#include "frontend_timing_fixture.hpp"
#include "runtime/frontend_timing.hpp"
#include "runtime/presentation_clock.hpp"
#include <cmath>
#include <chrono>
#include "runtime/original_presentation.hpp"
#include "runtime/frontend_ui.hpp"
#include "runtime/bus.hpp"
#include "game/god_mode.hpp"
#include "game/gameplay_diagnostic.hpp"
#ifdef NES_ENABLE_MODS
#include "runtime/mod_engine.hpp"
#endif
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace nes::frontend;
using nes::runtime::Bus;
static void require(bool b,const char*m){if(!b)throw std::runtime_error(m);}
struct Clock {uint64_t now{};static uint64_t get(void*p) noexcept {return ++static_cast<Clock*>(p)->now;}};
using Record=std::array<uint64_t,8>;
struct Trace {
    Bus* bus;
    std::vector<Record> records;
    RuntimeServices services(){return {this,
        [](void*p,uint16_t a,uint8_t t)->uint8_t{auto&s=*static_cast<Trace*>(p);auto v=s.bus->read8(a,t);s.records.push_back({'R',a,v,t,s.bus->cycles(),0,0,0});return v;},
        [](void*p,uint16_t a,uint8_t v,uint8_t t){auto&s=*static_cast<Trace*>(p);s.bus->write8(a,v,t);s.records.push_back({'W',a,v,t,s.bus->cycles(),0,0,0});},
        [](void*p,uint16_t a)->uint8_t{return static_cast<Trace*>(p)->bus->cartridge().bank_for_pc(a);},
        [](void*p,uint8_t n){auto&s=*static_cast<Trace*>(p);s.bus->advance(n);s.records.push_back({'A',n,s.bus->cycles(),0,0,0,0,0});},
        [](void*p,CPUState*c,uint8_t m)->uint8_t{auto&s=*static_cast<Trace*>(p);auto v=s.bus->boundary(*c,m);s.records.push_back({'B',m,v,c->PC,c->SP,c->P,c->A,s.bus->cycles()});return v;},
        [](void*p,uint8_t n,uint8_t o){auto&s=*static_cast<Trace*>(p);s.bus->poll_irq(n,o);s.records.push_back({'P',n,o,s.bus->cycles(),0,0,0,0});}};}
};
struct State {
    std::vector<uint64_t> fields;
    std::array<uint8_t,2048> ram{};
    std::array<uint8_t,256*240> frame{},background{};
    std::array<uint8_t,256> oam{};
    std::vector<uint32_t> pcm;
    bool operator==(const State&o)const{return fields==o.fields&&ram==o.ram&&frame==o.frame&&background==o.background&&oam==o.oam&&pcm==o.pcm;}
};
static State snapshot(Bus&b,const CPUState&c,const std::vector<float>&pcm){
    const auto a=b.audio().counters();auto&v=b.video();auto&k=b.cartridge();State s;
    s.fields={c.A,c.X,c.Y,c.SP,c.P,c.PC,b.cycles(),v.frames(),v.dots(),v.scanline(),v.dot(),v.status(),v.control(),v.mask(),v.address(),v.temporary_address(),
        b.nmi_count(),b.irq_count(),b.dma_count(),b.dmc_dma_count(),b.audio().cycles(),b.audio().produced_samples(),b.audio().dropped_samples(),
        a.linear,a.noise_shift,a.dmc_address,a.dmc_remaining,a.frame_cycle,a.pulse_timer[0],a.pulse_timer[1],
        k.control(),k.chr0(),k.chr1(),k.prg_register(),k.shift_register()};
    for(auto x:a.length)s.fields.push_back(x);for(auto x:a.envelope)s.fields.push_back(x);
    for(unsigned i=0;i<s.ram.size();++i)s.ram[i]=b.ram().read(uint16_t(i));
    s.frame=v.frame();s.background=v.background_frame();s.oam=v.oam();
    for(float x:pcm){uint32_t bits;static_assert(sizeof(bits)==sizeof(x));std::memcpy(&bits,&x,sizeof(bits));s.pcm.push_back(bits);}
    return s;
}
static uint64_t steady_tick(void*) noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
struct Replay {std::vector<State> frames;std::vector<Record> trace;std::string measurement;};
static Replay run(bool enabled,unsigned presentations,unsigned hz=0,unsigned disturbance=0){
    std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};
    for(const auto&b:timing_bytes)prg[b.offset]=b.value;
    for(unsigned i=0;i<chr.size();++i)chr[i]=uint8_t(i%16<8?0xaa:0x55);
    Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});CPUState cpu=cpu_initial_state(0x8000);Trace trace{&bus,{}};
    nes::game::GodMode god;auto services=god.services(bus,trace.services());
    nes::game::GameplayDiagnostic diagnostic;services=diagnostic.services(bus,services,cpu);
#ifdef NES_ENABLE_MODS
    nes::runtime::ModEngine mods(bus);
    // An observable real on_frame hook, independent of a presentation count.
    mods.load_source("function on_frame(s) nes.write_ram(85,nes.read_ram(85)+1) end");
#endif
    FrontendTiming timing;Clock clock;
    if(enabled){if(hz)timing.start(1000000000,steady_tick,nullptr,false);else timing.start(1000000,Clock::get,&clock,true);}
    Replay out;uint64_t legacy=0,slices=0;uint64_t batches=0,sample_count=0;nes::runtime::OriginalRGBA rgba{};
    FrameSnapshot frozen;PresentationClock scheduler(1000000,hz?hz:60,0);
    uint64_t host=0,draws=0;bool disturbed=false;
    const auto draw=[&]{
        FrontendTiming::Scope render(timing,TimingStage::render);
        const auto saved=frozen.pixels();
        nes::runtime::original_rgba(frozen.pixels().data(),256,240,rgba);
        require(frozen.pixels()==saved,"scheduled draw changed immutable snapshot");
        timing.record(TimingCount::nes_draws);timing.record(TimingCount::render_passes);++draws;
    };
    unsigned step=0;
    while(step<12){
        if(hz) {
            if(disturbance && step==6 && !disturbed) {
                if(disturbance==2)scheduler.set_paused(true,host);
                const auto state=snapshot(bus,cpu,{});const auto trace_size=trace.records.size();
                host+=250000;
                if(disturbance==2){require(!scheduler.poll(host).simulate,"paused scheduler authorized native step");draw();scheduler.presentation_complete(host);scheduler.set_paused(false,host);}
                require(snapshot(bus,cpu,{})==state&&trace.records.size()==trace_size,"pause/suspension changed native state");disturbed=true;
            }
            const auto due=scheduler.poll(host);
            if(!due.simulate) {
                const auto state=snapshot(bus,cpu,{});const auto trace_size=trace.records.size();
                if(due.present&&frozen.valid()){draw();scheduler.presentation_complete(host);}
                require(snapshot(bus,cpu,{})==state&&trace.records.size()==trace_size,"extra presentation executed instructions/hooks/audio");
                host=static_cast<uint64_t>(std::ceil(scheduler.next_deadline()));continue;
            }
        }
        FrontendTiming::Scope simulation(timing,TimingStage::simulation);
        bus.set_buttons(0,uint8_t(step*17));
#ifdef NES_ENABLE_MODS
        mods.frame(cpu);timing.record(TimingCount::mod_hooks);
#endif
        god.begin_frame();timing.record(TimingCount::god_hooks);
        const auto before=bus.video().frames();
        while(bus.video().frames()==before){
            uint32_t budget=1000;timing.record(TimingCount::native_slices);++slices;
            require(game_bank0_frontend_timing(&cpu,&services,&budget)!=0,"synthetic native address missing");
            diagnostic.slice(cpu);timing.record(TimingCount::diagnostic_slices);
        }
        diagnostic.frame(cpu,false,false,false);timing.record(TimingCount::diagnostic_frames);
        bus.video().consume_frame();timing.record(TimingCount::nes_frames);timing.record(TimingCount::ppu_frames,bus.video().frames()-before);simulation.stop();
        FrontendTiming::Scope audio(timing,TimingStage::audio);auto pcm=bus.audio().take_samples();++batches;sample_count+=pcm.size();
        timing.record(TimingCount::audio_batches);timing.record(TimingCount::pcm_samples,pcm.size());audio.stop();
        const auto before_draw=snapshot(bus,cpu,pcm);
        frozen.complete(bus.video().frame(),step+1,bus.video().frames(),bus.cycles(),host);
        if(hz){
            scheduler.simulation_complete(host);
            if(step==11 || scheduler.poll(host).present){draw();scheduler.presentation_complete(host);}
        } else for(unsigned n=0;n<presentations;++n)draw();
        require(snapshot(bus,cpu,pcm)==before_draw,"presentation/metrics changed observed CPU, memory, PPU, mapper or PCM state");
        out.frames.push_back(before_draw);++legacy;
        if(legacy==12)timing.record(TimingCount::captures);
        ++step;
        if(hz)host=static_cast<uint64_t>(std::ceil(scheduler.next_deadline()));
    }
    require(legacy==12&&batches==12&&bus.nmi_count()>0&&bus.dma_count()>0&&sample_count>0,"test must exercise frame limit, audio, real NMI and DMA");
#ifdef NES_ENABLE_MODS
    require(bus.ram().read(85)==12,"mod hook tied to presentation count");
#endif
    // A render-only pause sequence must not call native code/hooks or drain APU.
    const auto paused=snapshot(bus,cpu,{});nes::ui::FrontendUI ui;ui.handle(nes::ui::Action::request_exit);require(ui.paused(),"pause model");
    for(unsigned n=0;n<5;++n){timing.record(TimingCount::paused_iterations);nes::runtime::original_rgba(bus.video().frame().data(),256,240,rgba);timing.record(TimingCount::nes_draws);}
    require(snapshot(bus,cpu,{})==paused&&bus.audio().take_samples().empty(),"paused presentation advanced/drained NES audio or state");
    if(enabled){require(timing.count(TimingCount::nes_frames)==12&&timing.count(TimingCount::audio_batches)==12&&timing.count(TimingCount::god_hooks)==12&&
        timing.count(TimingCount::diagnostic_frames)==12&&timing.count(TimingCount::diagnostic_slices)==slices&&timing.count(TimingCount::captures)==1&&
        timing.count(TimingCount::native_slices)==slices&&timing.count(TimingCount::nes_draws)==draws+5&&timing.count(TimingCount::pcm_samples)==sample_count,"native observation counts not independent");}
    else require(timing.clock_reads()==0,"disabled native observation sampled clock");
    if(hz){std::ostringstream measured;timing.report(measured,"synthetic_virtual_schedule",legacy);out.measurement=measured.str();}
    out.trace=std::move(trace.records);return out;
}
int main(int argc,char**argv){try{
    const auto reference=run(false,0);unsigned cases=0;
    if(argc>1 && std::string(argv[1])=="scheduled") {
        for(unsigned hz:{60u,90u,120u})for(unsigned disturbance:{0u,1u,2u}) {
            auto result=run(true,0,hz,disturbance);
            require(result.frames==reference.frames&&result.trace==reference.trace,"scheduled native replay differs from original execution");++cases;
            std::cout<<"Native measurement target="<<hz<<" disturbance="<<disturbance<<" schedule=virtual graphics=conversion_only\n"<<result.measurement;
        }
        std::cout<<"Frontend scheduled native cases="<<cases<<" PASS (generated C, scheduler, snapshot, PCM, complete bus trace, pause/suspension)\n";return 0;
    }
    for(bool enabled:{false,true})for(unsigned presentations:{0u,1u,3u,7u}){
        auto result=run(enabled,presentations);require(result.frames==reference.frames&&result.trace==reference.trace,"per-frame native replay/bus trace differs from uninstrumented reference");++cases;
    }
    std::cout<<"Frontend timing native cases="<<cases<<" PASS (generated C, per-frame CPU/RAM/PPU/PCM, bus order, hooks, NMI/DMA)\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
