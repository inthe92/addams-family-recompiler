#include "frontend_timing_fixture.hpp"
#include "runtime/mod_lua.hpp"
#include "runtime/presentation_clock.hpp"
#include "runtime/bus.hpp"
#include "game/mod_observation.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
static std::vector<uint64_t> run(unsigned hz){
    std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};for(auto b:timing_bytes)prg[b.offset]=b.value;
    nes::runtime::Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});auto cpu=cpu_initial_state(0x8000);auto services=bus.services();
    nes::mods::LuaMod mod("local n=0; function on_update(dt) n=n+1; assert(math.abs(dt-1/50.007)<1e-10); assert(game.get_player()==nil); local s=game.get_status(); s.frame=123456; log.info(tostring(n)) end");
    mod.load({});std::vector<uint64_t> result;nes::frontend::PresentationClock clock(1000000,hz?hz:60,0);uint64_t now=0,units=0,draws=0;
    nes::frontend::FrameSnapshot image;nes::runtime::OriginalRGBA rgba{};
    while(units<12){auto due=clock.poll(now);if(due.simulate){
        bus.set_buttons(0,uint8_t(units*17));if(hz)mod.update(nes::game::mod_observation(bus));
        auto before=bus.video().frames();while(bus.video().frames()==before){uint32_t budget=1000;require(game_bank0_frontend_timing(&cpu,&services,&budget)!=0,"synthetic address missing");}
        bus.video().consume_frame();auto pcm=bus.audio().take_samples();++units;clock.simulation_complete(now);
        result.insert(result.end(),{cpu.A,cpu.X,cpu.Y,cpu.SP,cpu.P,cpu.PC,bus.cycles(),bus.video().frames(),bus.audio().cycles(),bus.audio().produced_samples(),bus.dma_count(),bus.nmi_count()});
        for(unsigned i=0;i<2048;++i)result.push_back(bus.ram().read(uint16_t(i)));for(auto p:bus.video().frame())result.push_back(p);for(auto x:pcm){uint32_t bits;std::memcpy(&bits,&x,4);result.push_back(bits);}
        image.complete(bus.video().frame(),units,bus.video().frames(),bus.cycles(),now);
    }
    if(due.present&&image.valid()){const auto cycle=bus.cycles();nes::runtime::original_rgba(image.pixels().data(),256,240,rgba);require(bus.cycles()==cycle,"draw advances CPU");++draws;clock.presentation_complete(now);}
    now=uint64_t(std::ceil(clock.next_deadline()));}
    if(hz)require(mod.updates()==12&&mod.logs().size()==12,"callback count follows presentation rate");else require(mod.updates()==0,"safe-mode oracle executed mod");
    require(draws>0&&bus.nmi_count()>0&&bus.dma_count()>0,"fixture does not exercise presentation/NMI/DMA");return result;
}
int main(){try{auto reference=run(0);for(auto hz:{60u,90u,120u})require(run(hz)==reference,"managed mod alters generated native execution, RAM, framebuffer or PCM");std::cout<<"Managed native rates cases=3 PASS\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
