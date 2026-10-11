// Authored native programs, actual Bus/APU, no interpreter or game data.
// PAL 08 calibrates onset separately. Edge controls below test the documented
// CPU poll contract in our integer clock, not unmeasured PAL master phases.
#include "runtime/bus.hpp"
#include "irq_edge_fixtures.hpp"
#include "game/actor_view.hpp"
#include "game/god_mode.hpp"
#include "game/gameplay_diagnostic.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>

using namespace nes::runtime;
// ActorView's optional pose decoder links this import symbol. No synthetic
// instruction reaches its game-specific pose hooks, and no ROM is loaded.
extern "C" { const uint8_t (*imported_prg_data)[16384]=nullptr; }
static unsigned passed{}, failed{};
static void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }

struct Machine {
    const EdgeFixture& f;
    std::array<uint8_t, 32768> prg{};
    std::array<uint8_t, 8192> chr{};
    std::unique_ptr<Bus> bus;
    CPUState cpu;
    explicit Machine(const char* name) : f(find(name)), cpu(cpu_initial_state(f.pc)) {
        for (unsigned i=0; i<f.count; ++i) prg[f.bytes[i].address-0x8000]=f.bytes[i].value;
        bus=std::make_unique<Bus>(CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
        cpu.A=0x40; cpu.X=0x17; cpu.Y=0x29; cpu.P=CPU_U;
    }
    static const EdgeFixture& find(const char* name) {
        for (const auto& f:edge_fixtures) if (std::string(f.name)==name) return f;
        throw std::runtime_error("Unknown edge fixture");
    }
    void step(const RuntimeServices* wrapped=nullptr) {
        auto s=wrapped?*wrapped:bus->services(); uint32_t left=1;
        require(f.execute(&cpu,&s,&left) && !left,"Native instruction dispatch");
        require(bus->cycles()==bus->audio().cycles(),"CPU/APU clock equality");
    }
    void entered(uint16_t return_pc, uint8_t saved_p, uint8_t saved_a, uint64_t cycle, bool nmi=false) {
        if(cpu.PC!=(nmi?0x9100:0x9000) || cpu.SP!=0xfa)
            throw std::runtime_error("Vector/SP observed PC="+std::to_string(cpu.PC)+" SP="+
                std::to_string(cpu.SP)+" IRQ="+std::to_string(bus->irq_count())+" NMI="+std::to_string(bus->nmi_count()));
        require(cpu.A==saved_a && cpu.X==0x17 && cpu.Y==0x29,"Entry A/X/Y");
        require(cpu.P==(saved_p|CPU_I),"Entry flags");
        require(bus->ram().read(0x1fb)==saved_p && bus->ram().read(0x1fc)==uint8_t(return_pc) &&
                bus->ram().read(0x1fd)==uint8_t(return_pc>>8),"Saved P/PC, B=0 U=1");
        require(bus->cycles()==cycle,"Interrupt duration and absolute cycle");
        require(bus->irq_count()==(nmi?0u:1u) && bus->nmi_count()==(nmi?1u:0u),"Interrupt counts/priority");
    }
};

template<class F> static void test(const std::string& name,F run) {
    try {run();++passed;std::cout<<name<<": PASS\n";}
    catch(const std::exception& e){++failed;std::cerr<<name<<": FAIL: "<<e.what()<<'\n';}
}

static void masks() {
    // Stable source well outside the assertion/acknowledgement collision.
    struct Case {const char* op; uint8_t before,pulled,after; unsigned cycles; bool irq, next;};
    const Case cases[]={
        {"cli",0x24,0,0x20,2,false,true}, {"sei",0x20,0,0x24,2,true,false},
        {"plp",0x24,0x20,0x20,4,false,true}, {"plp",0x20,0x24,0x24,4,true,false},
        {"rti",0x24,0x20,0x20,6,true,false}, {"rti",0x20,0x24,0x24,6,false,false},
        {"clear",0x20,0,0x20,4,true,false}, {"clear",0x24,0,0x24,4,false,false},
        {"inhibit",0x20,0,0x20,4,true,false}, {"rmw_clear",0x20,0,0x20,6,false,false},
    };
    for (const auto& c:cases) test(std::string("mask/ack ")+c.op+" I="+std::to_string(c.before&4),[&]{
        Machine m(c.op);m.bus->advance(33300);m.cpu.P=c.before;
        require(m.bus->audio().irq_pending(),"Held IRQ setup");
        if(std::string(c.op)=="plp") {m.cpu.SP=0xfc;m.bus->ram().write(0x1fd,c.pulled);}
        if(std::string(c.op)=="rti") {
            m.cpu.SP=0xfa;m.bus->ram().write(0x1fb,c.pulled);
            m.bus->ram().write(0x1fc,0);m.bus->ram().write(0x1fd,0x82);
        }
        m.step();
        if(c.irq) m.entered(m.f.next,c.after,0x40,33300+c.cycles+7);
        else {
            require(m.bus->irq_count()==0 && m.cpu.PC==m.f.next && m.cpu.SP==0xfd &&
                    m.cpu.P==c.after && m.cpu.A==0x40 && m.cpu.X==0x17 && m.cpu.Y==0x29,
                    "No early IRQ; completed registers/flags/PC/SP");
            require(m.bus->cycles()==33300+c.cycles,"Instruction cycles");
            m.step();
            if(c.next) m.entered(m.f.next+1,c.after,0x40,33300+c.cycles+2+7);
            else require(m.bus->irq_count()==0 && m.bus->cycles()==33300+c.cycles+2,"No stale/masked IRQ on next instruction");
        }
    });
}

static void edges() {
    // Two $4017 reset parities; onset is the existing APU event, not a forced
    // private state. Expectations are CPU detector/poll controls (not a new
    // certification of the 2A07's get/put or phi-phase collision).
    for(unsigned parity=0;parity<2;++parity) for(const char* op:{"nop","branch","cross","jmp"})
        for(int alignment=-2;alignment<=0;++alignment) for(unsigned taken=0;taken<2;++taken) {
            if((std::string(op)=="nop" || std::string(op)=="jmp") && taken) continue;
            const bool branch=std::string(op)=="branch" || std::string(op)=="cross";
            const bool crossing=std::string(op)=="cross" && taken;
            // Ordinary JMP has a later poll than a taken noncrossing branch.
            const bool irq=alignment==0 || ((crossing || std::string(op)=="jmp") && alignment>=-1) ||
                           (crossing && alignment==-2);
            test(std::string("edge ")+op+" parity="+std::to_string(parity)+" alignment="+
                 std::to_string(alignment)+" taken="+std::to_string(taken),[&]{
                Machine m(op);m.cpu.P=uint8_t(0x20|((branch&&!taken)?2:0));
                m.bus->advance(10+parity);m.bus->write8(0x4017,0);
                const uint64_t event=10+parity+(parity?4:3)+33252;
                const uint64_t start=event+alignment;m.bus->advance(start-m.bus->cycles());
                const unsigned duration=branch?(taken?m.f.cycles:2):m.f.cycles;
                const uint16_t next=branch&&!taken?uint16_t(m.f.pc+2):m.f.next;
                const uint8_t p=m.cpu.P;m.step();
                if(irq) m.entered(next,p,0x40,start+duration+7);
                else {
                    require(m.bus->irq_count()==0 && m.cpu.PC==next && m.cpu.SP==0xfd &&
                            m.cpu.P==p && m.cpu.A==0x40 && m.cpu.X==0x17 && m.cpu.Y==0x29,
                            "Assertion after poll must defer IRQ");
                    require(m.bus->cycles()==start+duration,"Branch/ordinary duration");
                    m.step();m.entered(next+1,p,0x40,start+duration+2+7);
                }
            });
        }
}

static void priority_dma() {
    test("NMI overrides sampled IRQ cleared by $4015",[]{
        Machine m("clear");m.bus->advance(33300);m.bus->write8(0x2000,0x80);
        m.bus->advance(58931-m.bus->cycles());
        require(m.bus->audio().irq_pending() && (m.bus->video().status()&0x80),"Both sources pending");
        m.step();m.entered(0x8003,0x20,0x40,58931+4+7,true);
        m.step(); // NMI RTI must not revive the discarded IRQ decision.
        require(m.bus->irq_count()==0 && m.cpu.PC==0x8003 && m.cpu.SP==0xfd && m.cpu.P==0x20,
                "NMI consumes decision; RTI cannot revive cleared source");
    });
    for(unsigned parity=0;parity<2;++parity) for(bool held:{false,true})
        test("OAM decision held="+std::to_string(held)+" parity="+std::to_string(parity),[&]{
            Machine m("oam");m.cpu.A=2;
            for(unsigned i=0;i<256;++i)m.bus->ram().write(uint16_t(0x200+i),uint8_t(i^0x5a));
            const uint64_t start=(held?33300u:33000u)+parity;m.bus->advance(start);
            m.step();const uint64_t finish=start+4+513+parity;
            require(m.bus->dma_count()==1 && m.bus->dmc_dma_count()==0,"Single OAM DMA");
            for(unsigned i=0;i<256;++i)require(m.bus->video().oam()[i]==uint8_t(i^0x5a),"OAM copy");
            if(held)m.entered(0x8003,0x20,2,finish+7);
            else {
                require(m.bus->irq_count()==0 && m.cpu.PC==0x8003 && m.bus->cycles()==finish,
                        "IRQ raised during stall is not an earlier instruction poll");
                m.step();m.entered(0x8004,0x20,2,finish+2+7);
            }
        });
    for(unsigned parity=0;parity<2;++parity)
        test("DMC IRQ appears during boundary DMA parity="+std::to_string(parity),[&]{
            Machine m("nop");m.bus->advance(parity);
            m.bus->write8(0x4010,0x80);m.bus->advance(2);
            m.bus->write8(0x4012,0);m.bus->advance(2);
            m.bus->write8(0x4013,0);m.bus->advance(2);
            m.bus->write8(0x4015,0x10);
            // Start delay is 4 on even writes and 3 on odd writes. A two-cycle
            // native NOP ends exactly when the request becomes ready.
            m.bus->advance(parity?1:2);const uint64_t start=m.bus->cycles();m.step();
            const uint64_t finish=start+2+3; // Both requests land at CPU cycle 4.
            require(m.bus->dmc_dma_count()==1 && m.bus->audio().irq_pending(),"One-byte DMC DMA asserts IRQ");
            require(m.bus->irq_count()==0 && m.cpu.PC==0x8001 && m.bus->cycles()==finish,
                    "DMC completion cannot retroactively change the instruction poll");
            m.step();m.entered(0x8002,0x20,0x40,finish+2+7);
        });
}

static void adapters() {
    // Exact service chain used by the desktop; no room, actor or GOD mechanics
    // are exercised or altered. A last-cycle clear must preserve the poll.
    for(unsigned kind=0;kind<5;++kind) test("ABI service adapter="+std::to_string(kind),[&]{
        Machine m("clear");m.bus->advance(33300);
        nes::game::ActorView actors;nes::game::GodMode god;
        nes::game::GameplayDiagnostic diagnostic;std::ostringstream log;
        auto services=m.bus->services();
        if(kind==0 || kind==4)services=actors.services(*m.bus);
        if(kind==1 || kind==2 || kind==4) {
            god.set_enabled(kind!=1);services=god.services(*m.bus,services);
        }
        if(kind==3 || kind==4) {
            diagnostic.begin(log,false,false,false);
            services=diagnostic.services(*m.bus,services,m.cpu);
        }
        require(services.poll_irq!=nullptr,"Adapter must forward ABI v3 poll service");
        m.step(&services);m.entered(0x8003,0x20,0x40,33311);
        require(!m.bus->audio().irq_pending(),"Native status read acknowledges live source");
        m.step(&services); // RTI, decision consumed, source remains cleared.
        require(m.bus->irq_count()==1 && m.cpu.PC==0x8003 && m.cpu.SP==0xfd && m.cpu.P==0x20,
                "Adapter must not retain an IRQ after delivery");
    });
}

int main(int argc,char** argv) {
    const std::string group=argc==2?argv[1]:"";
    if(group=="masks")masks();else if(group=="edges")edges();else if(group=="priority_dma")priority_dma();
    else if(group=="adapters")adapters();
    else {std::cerr<<"Expected masks, edges, priority_dma or adapters\n";return 2;}
    std::cout<<"IRQ edge controls executed="<<passed+failed<<" passed="<<passed<<" failed="<<failed<<'\n';
    return failed?1:0;
}
