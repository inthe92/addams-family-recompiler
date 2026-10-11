#include "game/gameplay_diagnostic.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

using nes::game::GameplayDiagnostic;
using nes::runtime::Bus;
namespace fs=std::filesystem;
extern "C" uint8_t game_bank0_program(CPUState*,const RuntimeServices*,uint32_t*);
extern "C" uint8_t game_bank0_effects(CPUState*,const RuntimeServices*,uint32_t*);
static void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
static unsigned count(const std::string& log,const char* event) {
    const std::string token=std::string("\"event\":\"")+event+'"';unsigned n=0;
    for(size_t p=0;(p=log.find(token,p))!=std::string::npos;p+=token.size())++n;
    return n;
}
struct Fixture {
    std::vector<uint8_t> prg=std::vector<uint8_t>(131072),chr=std::vector<uint8_t>(131072);
    std::unique_ptr<Bus> bus=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
    CPUState cpu=cpu_initial_state(0xC000);
    std::ostringstream log;GameplayDiagnostic diagnostic;
    RuntimeServices s=bus->services();
    Fixture(bool enabled=true) {
        if(enabled)diagnostic.begin(log,false,false,false);
        s=diagnostic.services(*bus,s,cpu);
    }
    void write(uint16_t a,uint8_t v,uint16_t pc=0xC000) {
        cpu.PC=pc;(void)s.boundary(s.context,&cpu,CPU_I);
        s.write8(s.context,a,v,1);s.advance(s.context,2);cpu.PC=uint16_t(pc+3);
        (void)s.boundary(s.context,&cpu,CPU_I);
    }
    void frame(bool god=false,bool items=false,bool mods=false) {
        bus->advance(33300);diagnostic.frame(cpu,god,items,mods);
    }
    void stable(uint8_t room) {
        write(0x045B,0);write(0x71,room);write(0x0471,0xFF);write(0xB1,0);write(0x80,0);write(0x70,0);
    }
    void bank(uint8_t b) {
        for(unsigned bit=0;bit<5;++bit)write(0xE000,uint8_t((b>>bit)&1));
    }
};
static std::vector<std::string> exported;
static void save(Fixture& f) {f.diagnostic.end("synthetic_end");exported.push_back(f.log.str());}
static void disabled() {
    Fixture f(false);auto original=f.bus->services();
    require(f.s.context==original.context&&f.s.read8==original.read8&&f.s.write8==original.write8&&
        f.s.advance==original.advance&&f.s.boundary==original.boundary&&f.s.bank_for_pc==original.bank_for_pc&&f.s.poll_irq==original.poll_irq,"Disabled observer must return original services verbatim");
    f.write(0x0633,1);f.frame();f.diagnostic.error("disabled");f.diagnostic.end("disabled");
    require(f.log.str().empty(),"Disabled logging produces no events");
}
static void rooms() {
    Fixture f;f.stable(2);f.frame();
    require(count(f.log.str(),"room_enter")==0,"One sample is not a confirmed room");
    f.frame();f.frame();require(count(f.log.str(),"room_enter")==1,"Stable room enters once");
    f.write(0x0471,3);f.write(0x71,3);f.write(0x80,1);f.frame();
    require(count(f.log.str(),"room_exit")==1&&count(f.log.str(),"room_enter")==1,"Pending/loading scene is not entered");
    f.stable(3);f.frame();f.frame();require(count(f.log.str(),"room_enter")==2,"Transition reaches next numeric room");
    f.write(0xB1,2);f.stable(3);f.frame();f.frame();
    require(count(f.log.str(),"room_enter")==3,"Same-ID reload is an observable new entry");
    f.diagnostic.end("user_quit");f.diagnostic.end("duplicate");
    require(count(f.log.str(),"session_start")==1&&count(f.log.str(),"session_end")==1,"Session lifecycle is singular");
    exported.push_back(f.log.str());
}
static void death_and_pickups() {
    Fixture f;f.stable(2);f.frame();f.frame();f.bank(1);
    f.cpu.X=0;f.bus->ram().write(0x0320,9);
    f.write(0x0633,1,0xA80A);f.write(0x0633,1,0xA80A);
    require(count(f.log.str(),"item_pickup")==1,"Confirmed native grant is recorded once");
    f.write(0x0633,0);f.write(0x0633,1,0xA700);
    require(count(f.log.str(),"item_pickup")==1,"Other native site is only a progress change");
    f.write(0x0633,0);f.cpu.X=12;f.write(0x0633,1,0xA80A);
    require(count(f.log.str(),"item_pickup")==1,"Invalid entity slot cannot imply pickup");
    f.cpu.X=0;f.write(0x0633,0);f.bus->ram().write(0x71,3);f.write(0x0633,1,0xA80A);
    require(count(f.log.str(),"item_pickup")==1,"Wrong scene cannot imply confirmed pickup");
    f.write(0x70,1);f.write(0x70,2);f.write(0x04ED,2);
    require(count(f.log.str(),"death")==1,"Death counter updates are not repeated deaths");
    f.write(0x0471,3);f.write(0x70,0);f.frame();
    require(count(f.log.str(),"respawn_observed")==0,"Cleared death during loading is not respawn");
    f.stable(3);f.frame();f.frame();
    require(count(f.log.str(),"respawn_observed")==1,"Respawn requires stable room after death");
    f.write(0x063A,1);f.write(0x063A,2);
    require(count(f.log.str(),"progress_activation")==1,"Ending timer is not logged every tick");
    require(count(f.log.str(),"progress_changed")>=4,"Numeric progression changes recorded");save(f);
}
static void banks_and_errors() {
    Fixture f;f.bank(1);f.bank(1);f.bank(2);
    require(count(f.log.str(),"bank_mapping_changed")==2,"Only effective PRG mapping changes logged");
    f.cpu.PC=0x8123;f.diagnostic.missing_address(f.cpu);f.diagnostic.error("synthetic\"error\n");
    for(unsigned i=0;i<20;++i)f.diagnostic.slice(f.cpu);
    require(count(f.log.str(),"no_cpu_progress")==1,"No-cycle warning is bounded");
    // Feed a deliberately stale frame sample to exercise the frame watchdog.
    f.bus->advance(1662607);f.diagnostic.slice(f.cpu);
    require(count(f.log.str(),"no_video_frame_progress")==0,"Actual frame advancement is not a stall");
    require(count(f.log.str(),"uncovered_native_address")==1&&count(f.log.str(),"error")==1,"Missing entry/error remain explicit");save(f);
}
static void assists_and_ending() {
    Fixture f;f.frame(true,false,false);f.frame(false,false,false);
    require(f.diagnostic.assisted(),"GOD use remains sticky when disabled again");
    f.frame(false,true,false);f.frame(false,false,true);f.frame(false,false,false);
    f.write(0x045B,4,0xC000);require(count(f.log.str(),"ending_screen_observed")==0,"Screen numeric value alone is not completion");
    f.write(0x045B,0);f.write(0x045B,4,0xC405);
    require(count(f.log.str(),"ending_screen_observed")==1,"Confirmed native ending selector recorded");save(f);
    Fixture normal;normal.stable(2);normal.frame();normal.frame();normal.write(0x045B,4,0xC405);
    require(!normal.diagnostic.assisted(),"Unaided synthetic ending remains unaided");save(normal);
}
struct Trace {
    Bus* bus;std::vector<std::array<uint64_t,4>> accesses;
    RuntimeServices services() {
        return {this,
            [](void* p,uint16_t a,uint8_t t)->uint8_t{auto& s=*static_cast<Trace*>(p);auto v=s.bus->read8(a,t);s.accesses.push_back({'R',a,v,s.bus->cycles()+t});return v;},
            [](void* p,uint16_t a,uint8_t v,uint8_t t){auto& s=*static_cast<Trace*>(p);s.bus->write8(a,v,t);s.accesses.push_back({'W',a,v,s.bus->cycles()+t});},
            [](void*,uint16_t)->uint8_t{return 0;},
            [](void* p,uint8_t t){static_cast<Trace*>(p)->bus->advance(t);},
            [](void* p,CPUState* c,uint8_t m)->uint8_t{return static_cast<Trace*>(p)->bus->boundary(*c,m);},
            [](void* p,uint8_t total,uint8_t offsets){static_cast<Trace*>(p)->bus->poll_irq(total,offsets);}};
    }
};
static void same(const Fixture& a,const Fixture& b,const Trace& ta,const Trace& tb) {
    const auto& x=a.cpu;const auto& y=b.cpu;
    require(x.A==y.A&&x.X==y.X&&x.Y==y.Y&&x.SP==y.SP&&x.P==y.P&&x.PC==y.PC,"Observer preserves every CPU register/flag");
    require(ta.accesses==tb.accesses,"Observer preserves access order, values and exact read/write cycles");
    for(unsigned i=0;i<2048;++i)require(a.bus->ram().read(uint16_t(i))==b.bus->ram().read(uint16_t(i)),"Observer preserves all RAM bytes");
    require(a.bus->cycles()==b.bus->cycles()&&a.bus->video().dots()==b.bus->video().dots()&&
        a.bus->video().frames()==b.bus->video().frames()&&a.bus->video().status()==b.bus->video().status()&&
        a.bus->video().address()==b.bus->video().address()&&a.bus->audio().cycles()==b.bus->audio().cycles(),"Observer preserves device cycles/PPU effects");
    require(a.bus->video().oam()==b.bus->video().oam()&&a.bus->video().frame()==b.bus->video().frame()&&
        a.bus->nmi_count()==b.bus->nmi_count()&&a.bus->irq_count()==b.bus->irq_count()&&
        a.bus->dma_count()==b.bus->dma_count(),"Observer preserves raster/OAM/interrupt/DMA state");
}
static void transparency() {
    for(unsigned which=0;which<4;++which) {
        Fixture plain(false),logged(false);Trace ta{plain.bus.get(),{}},tb{logged.bus.get(),{}};
        plain.s=ta.services();logged.diagnostic.begin(logged.log,false,false,false);
        plain.cpu=cpu_initial_state(which==0?0x8000:which==1?0x8100:which==2?0xB100:0xB170);
        plain.cpu.A=0x80;plain.cpu.SP=0;plain.cpu.P=0x6D;logged.cpu=plain.cpu;
        for(auto* f:{&plain,&logged}) {
            f->prg[7*16384+0x3FFE]=0;f->prg[7*16384+0x3FFF]=0x83;
            if(which>=2)f->bus->advance(58927);
        }
        logged.s=logged.diagnostic.services(*logged.bus,tb.services(),logged.cpu);
        for(auto* f:{&plain,&logged}) {
            uint32_t remaining=which==0?4:which==1?2:1;
            while(remaining) {
                const auto before=remaining;
                const auto ok=which<2?game_bank0_program(&f->cpu,&f->s,&remaining):game_bank0_effects(&f->cpu,&f->s,&remaining);
                require(ok&&remaining<before,"Real generated synthetic program progresses");
            }
        }
        same(plain,logged,ta,tb);
        for(auto* f:{&plain,&logged}) {
            f->s.write8(f->s.context,0x4016,1,0);f->s.advance(f->s.context,1);
            f->s.write8(f->s.context,0x4016,0,0);f->s.advance(f->s.context,1);
            (void)f->s.read8(f->s.context,0x4016,0);f->s.advance(f->s.context,1);
        }
        same(plain,logged,ta,tb);save(logged);
    }
    // Hardware NMI delivery must preserve the delegated return and CPU mutation.
    Fixture plain(false),logged(false);Trace ta{plain.bus.get(),{}},tb{logged.bus.get(),{}};
    for(auto* f:{&plain,&logged}) {f->bus->advance(33300);f->bus->video().write_register(0x2000,0x80);f->bus->advance(58931-f->bus->cycles());}
    plain.s=ta.services();logged.diagnostic.begin(logged.log,false,false,false);logged.s=logged.diagnostic.services(*logged.bus,tb.services(),logged.cpu);
    const auto ra=plain.s.boundary(plain.s.context,&plain.cpu,CPU_I),rb=logged.s.boundary(logged.s.context,&logged.cpu,CPU_I);
    require(ra==1&&rb==ra,"Diagnostic delegates interrupt return unchanged");same(plain,logged,ta,tb);save(logged);
}
static void sink_failure() {
    Fixture f;f.log.setstate(std::ios::badbit);f.write(0x0633,1);
    require(f.diagnostic.failed()&&f.bus->ram().read(0x0633)==1&&f.bus->cycles()==2,"Failed output cannot cancel a game write");
    f.write(0x0633,2);require(f.bus->ram().read(0x0633)==2,"Game continues after diagnostic I/O failure");
    Fixture thrown;thrown.log.exceptions(std::ios::badbit);try{thrown.log.setstate(std::ios::badbit);}catch(const std::ios_base::failure&){}
    thrown.write(0x0633,1);require(thrown.diagnostic.failed()&&thrown.bus->ram().read(0x0633)==1,"Throwing diagnostic sink cannot affect production callback");
    // Error reporting after runtime stack unwinding uses cached stamps only.
    GameplayDiagnostic diagnostic;std::ostringstream output;
    {Fixture local(false);diagnostic.begin(output,false,false,false);diagnostic.services(*local.bus,local.s,local.cpu);}
    diagnostic.error("after_runtime_unwind");diagnostic.end("exception");exported.push_back(output.str());
}
static void paths(const fs::path& root) {
    for(const auto& suffix:{"repo/.git","App.app/Contents","App.AppDir/usr","portable/tools/recompiler"})fs::create_directories(root/suffix);
    for(const auto& name:{"repo/logs","App.app/Contents/logs","App.AppDir/logs","portable/logs"}) {
        bool rejected=false;try{GameplayDiagnostic::local_log_path(root/name);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"Logs must reject repository/package trees");
    }
    auto path=GameplayDiagnostic::local_log_path(root/"local-diagnostics");
    std::ofstream(path)<<"synthetic sentinel\n";
    auto other=GameplayDiagnostic::local_log_path(root/"local-diagnostics");
    require(path!=other&&fs::is_regular_file(path),"Distinct session paths preserve earlier logs");
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    try {
        disabled();rooms();death_and_pickups();banks_and_errors();assists_and_ending();transparency();sink_failure();paths(fs::path(argv[1]).parent_path());
        std::ofstream output{fs::path(argv[1])};for(const auto& log:exported)output<<log;
        require(bool(output),"Write synthetic JSONL evidence");
        std::cout<<"8 synthetic gameplay diagnostic cases: PASS; generated programs and bus transparency checked\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
