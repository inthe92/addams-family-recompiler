#include "frontend_timing_fixture.hpp"
#include "runtime/mod_lua.hpp"
#include "runtime/presentation_clock.hpp"
#include "runtime/bus.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace nes::mods;
static void require(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
static std::vector<uint64_t> run(unsigned hz,bool pause=false){
 std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};for(auto b:timing_bytes)prg[b.offset]=b.value;
 nes::runtime::Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});auto cpu=cpu_initial_state(0x8000);auto s=bus.services();
 std::unique_ptr<LuaMod> mod;if(hz)mod=std::make_unique<LuaMod>("local n=0;function on_load()nes.memory.observe(32,64,function(changes)n=n+#changes;log.info(tostring(n))end)end;function on_update(dt)assert(math.abs(dt-1/50.007)<1e-10);hud.text(4,2,'CHANGES '..tostring(n))end",2*1024*1024,2,14);if(mod)mod->load({});
 std::vector<uint64_t> trace;nes::frontend::PresentationClock clock(1000000,hz>50?hz:60,0);uint64_t now=0,units=0,draws=0;nes::frontend::FrameSnapshot snapshot;
 auto draw=[&]{const auto cycles=bus.cycles(),updates=(mod?mod->updates():0);nes::runtime::OriginalRGBA rgba{};nes::runtime::original_rgba(snapshot.pixels().data(),256,240,rgba);auto overlay=mod?nes::ui::text_overlay_bitmap(hud_width,hud_height,mod->overlay()):nes::ui::Bitmap{};(void)overlay;require(bus.cycles()==cycles&&(mod?mod->updates():0)==updates,"HUD draw advances simulation/Lua");++draws;};
 while(units<12){auto due=hz<=50?nes::frontend::PresentationClock::Due{true,true}:clock.poll(now);
  if(due.simulate){bus.set_buttons(0,uint8_t(units*17));auto before=bus.video().frames();while(bus.video().frames()==before){uint32_t left=1000;require(game_bank0_frontend_timing(&cpu,&s,&left),"authored C did not execute");}bus.video().consume_frame();auto pcm=bus.audio().take_samples();++units;
   if(hz){Observation o;o.frame=bus.video().frames();o.cpu_cycles=bus.cycles();o.game=GameSnapshot{"own-fixture","playing",0,std::nullopt};o.memory=capture_memory(bus.ram());mod->update(o);}
   trace.insert(trace.end(),{cpu.A,cpu.X,cpu.Y,cpu.SP,cpu.P,cpu.PC,bus.cycles(),bus.video().frames(),bus.video().dots(),bus.audio().cycles(),bus.dma_count(),bus.nmi_count()});
   for(unsigned i=0;i<2048;++i)trace.push_back(bus.ram().read(i));for(auto v:bus.video().frame())trace.push_back(v);for(auto f:pcm){uint32_t bits;std::memcpy(&bits,&f,sizeof(bits));trace.push_back(bits);}
   snapshot.complete(bus.video().frame(),units,bus.video().frames(),bus.cycles(),now);clock.simulation_complete(now);
   if(pause&&units==4){auto frozen=mod->overlay();auto before_calls=(mod?mod->updates():0);clock.set_paused(true,now);for(unsigned i=0;i<20;++i){now+=20000;auto d=clock.poll(now);require(!d.simulate,"pause executes NES");if(d.present){draw();clock.presentation_complete(now);}}require((mod?mod->updates():0)==before_calls&&mod->overlay()[0].text==frozen[0].text,"pause changed HUD capture");clock.set_paused(false,now);}
  }
  if(due.present&&snapshot.valid()){draw();clock.presentation_complete(now);}now=uint64_t(std::ceil(clock.next_deadline()));
 }
 require((mod?mod->updates():0)==(hz?12:0),"v2 callbacks follow presentation rate");require(draws>0&&bus.nmi_count()>0&&bus.dma_count()>0,"fixture lacks drawing/NMI/DMA");if(mod){require(!mod->logs().empty(),"native RAM watch never triggered");mod->unload();require(mod->overlay().empty(),"native run HUD cleanup");}return trace;
}
int main(){try{auto ref=run(0);unsigned n=0;for(auto hz:{50u,60u,90u,120u}){require(run(hz)==ref,"v2 changed CPU/RAM/raster/PCM");++n;}require(run(120,true)==ref,"pause/repeated HUD draws changed execution");++n;std::cout<<"SDK native rates cases="<<n<<" PASS\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
