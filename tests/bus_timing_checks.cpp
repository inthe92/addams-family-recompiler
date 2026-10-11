#include "runtime/bus.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

extern "C" uint8_t game_bank0_effects(CPUState*, const RuntimeServices*, uint32_t*);
using nes::runtime::Bus;
static void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
struct Access { char kind; uint16_t address; uint8_t offset; };
struct Event { Access access; uint8_t value; };
struct Case { const char* name; char operation; uint8_t x,y,size,cycles; bool data; std::vector<Access> plan; };
// Independent, manually authored schedules: no emitter tables or CPU helpers
// calculate expected accesses, flags, results or instruction durations.
static const Case cases[]={
    {"LDA abs PPUSTATUS",'L',0,0,3,4,false,{{'R',0x2002,3}}},
    {"LDA abx noncross",'L',1,0,3,4,false,{{'R',0x2002,3}}},
    {"LDA abx crossing status dummy",'L',3,0,3,5,false,{{'R',0x2002,3},{'R',0x2102,4}}},
    {"LDA aby crossing status dummy",'L',0,3,3,5,false,{{'R',0x2002,3},{'R',0x2102,4}}},
    {"LDA izx pointer wrap",'L',7,0,2,6,false,{{'R',0xf8,2},{'R',0xff,3},{'R',0,4},{'R',0x2002,5}}},
    {"LDA izy crossing pointer wrap",'L',0,3,2,6,false,{{'R',0xff,2},{'R',0,3},{'R',0x2002,4},{'R',0x2102,5}}},
    {"BIT abs status",'B',0,0,3,4,false,{{'R',0x2002,3}}},
    {"INC abs read old/new write",'I',0,0,3,6,false,{{'R',0x2002,3},{'W',0x2002,4},{'W',0x2002,5}}},
    {"INC abx dummy/read/old/new",'I',3,0,3,7,false,{{'R',0x2002,3},{'R',0x2102,4},{'W',0x2102,5},{'W',0x2102,6}}},
    {"STA abx dummy/read side effect",'S',3,0,3,5,false,{{'R',0x2002,3},{'W',0x2102,4}}},
    {"LDA abs PPUDATA",'L',0,0,3,4,true,{{'R',0x2007,3}}},
    {"LDA abx PPUDATA noncross",'L',1,0,3,4,true,{{'R',0x2007,3}}},
    {"LDA abx PPUDATA dummy buffer",'L',8,0,3,5,true,{{'R',0x2007,3},{'R',0x2107,4}}},
    {"LDA izy PPUDATA dummy buffer",'L',0,8,2,6,true,{{'R',0xff,2},{'R',0,3},{'R',0x2007,4},{'R',0x2107,5}}}
};
static void set_address(Bus& bus,uint16_t address) {
    auto& v=bus.video();(void)v.read_register(0x2002);
    v.write_register(0x2006,uint8_t(address>>8));v.write_register(0x2006,uint8_t(address));
}
static void setup(Bus& bus,const Case& c,unsigned index) {
    bus.advance(33300);
    bus.ram().write(0xf8,0x71);bus.ram().write(0xff,index==4?0x02:0xff);bus.ram().write(0,0x20);
    if(c.data) {
        bus.video().write_memory(0x2100,0x5a);bus.video().write_memory(0x2101,0xc3);bus.video().write_memory(0x2102,0x17);
        set_address(bus,0x2100);(void)bus.video().read_register(0x2007);set_address(bus,0x2101);
    } else {
        (void)bus.video().read_register(0x2002);bus.video().write_register(0x2006,0x21);
    }
    bus.video().write_register(0x2003,0);
    bus.advance(58927-bus.cycles());
}
struct Trace { Bus* bus; uint64_t start; std::vector<Event> events; };
static void observe(Trace& t,char kind,uint16_t address,uint8_t offset,uint8_t value) {
    require(t.bus->cycles()==t.start,"An access must not commit CPU cycles");
    require(t.bus->video().dots()==((t.start+offset)*16)/5,"Exact PAL device clock at access");
    require(t.bus->audio().cycles()==t.start+offset,"Exact APU clock at access");
    if(!t.events.empty())require(t.events.back().access.offset<offset,"Access order must increase");
    t.events.push_back({{kind,address,offset},value});
}
static void check(unsigned index) {
    const auto& c=cases[index];std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};
    auto native=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
    auto reference=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
    setup(*native,c,index);setup(*reference,c,index);
    Trace t{native.get(),native->cycles(),{}};
    RuntimeServices s{&t,
        [](void* p,uint16_t a,uint8_t o)->uint8_t {auto& t=*static_cast<Trace*>(p);auto v=t.bus->read8(a,o);observe(t,'R',a,o,v);return v;},
        [](void* p,uint16_t a,uint8_t v,uint8_t o){auto& t=*static_cast<Trace*>(p);t.bus->write8(a,v,o);observe(t,'W',a,o,v);},
        [](void*,uint16_t)->uint8_t{return 0;},
        [](void* p,uint8_t n){static_cast<Trace*>(p)->bus->advance(n);},nullptr};
    auto cpu=cpu_initial_state(uint16_t(0xb100+index*16));cpu.A=0x5a;cpu.X=c.x;cpu.Y=c.y;cpu.SP=0xfe;cpu.P=0x6d;
    const auto initial=cpu;uint32_t left=1;
    require(game_bank0_effects(&cpu,&s,&left)&&left==0,"Generated instruction dispatch");
    std::vector<Event> expected;unsigned elapsed=0,writes=0;uint8_t operand=0;
    for(const auto& a:c.plan) {
        reference->advance(a.offset-elapsed);elapsed=a.offset;
        uint8_t value;
        if(a.kind=='R')value=operand=reference->read8(a.address);
        else {value=c.operation=='S'?initial.A:uint8_t(operand+(writes?1:0));reference->write8(a.address,value);++writes;}
        expected.push_back({a,value});
    }
    reference->advance(c.cycles-elapsed);
    require(t.events.size()==expected.size(),"All represented accesses observed");
    for(unsigned j=0;j<expected.size();++j) {
        const auto& e=expected[j];const auto& a=t.events[j];
        require(a.access.kind==e.access.kind&&a.access.address==e.access.address&&a.access.offset==e.access.offset&&a.value==e.value,"Independent bus schedule/value mismatch");
    }
    uint8_t result=c.operation=='I'?uint8_t(operand+1):operand,p=initial.P;
    if(c.operation=='L'||c.operation=='I')p=uint8_t((p&0x7d)|(result==0?2:0)|(result&0x80));
    if(c.operation=='B')p=uint8_t((p&0x3d)|(operand&0xc0)|((initial.A&operand)==0?2:0));
    require(cpu.A==(c.operation=='L'?result:initial.A)&&cpu.X==initial.X&&cpu.Y==initial.Y&&cpu.SP==initial.SP&&cpu.P==p&&cpu.PC==initial.PC+c.size,"Independent final CPU registers/flags");
    require(native->cycles()==t.start+c.cycles&&reference->cycles()==native->cycles(),"Exact total instruction duration");
    require(native->audio().cycles()==native->cycles()&&native->video().dots()==native->cycles()*16/5,"Finish advances residual device time once");
    require(native->video().status()==reference->video().status()&&native->video().address()==reference->video().address(),"Final PPU status/address effects");
    if(c.data) {
        require(native->video().address()==0x2101+(index>=12?2:1),"Dummy PPUDATA reads increment address");
        const auto n=native->video().read_register(0x2007),r=reference->video().read_register(0x2007);
        require(n==r&&n==(index>=12?0x17:0xc3),"Final PPUDATA read buffer");
    } else {
        require(native->video().status()==0,"Dummy status reads clear vblank");
        for(auto* b:{native.get(),reference.get()}) {b->video().write_register(0x2006,0x23);b->video().write_register(0x2006,0x45);require(b->video().address()==0x2345,"Status access resets write latch");}
    }
    std::cout<<c.name<<": OK\n";
}
int main() {
    try {for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i)check(i);std::cout<<"14 native timed side-effect cases: PASS\n";}
    catch(const std::exception& e){std::cerr<<"Timed side-effect regression: "<<e.what()<<'\n';return 1;}
}
