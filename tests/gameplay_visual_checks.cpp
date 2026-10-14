#include "runtime/mod_manager.hpp"
#include "runtime/mod_clock.hpp"
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace nes::mods;
static unsigned n=0;
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);++n;}
template<class F>static void reject(F f){bool bad=false;try{f();}catch(...){bad=true;}check(bad,"invalid call accepted");}
int main(int argc,char**argv){try{
 GameplayAssistance history;check(!history.observe(false,0),"fresh no-mod run labelled assisted");check(history.observe(false,12)&&history.observe(false,0),"gameplay assistance lost after Disable");
 Observation o;o.simulation_seconds=2;o.real_seconds=4;o.paused_seconds=1;
 {LuaMod m("assert(game.player==nil and ui==nil and graphics==nil and game.get_clocks==nil and game.get_input==nil)");check(m.visual().empty(),"v1 changed");}
 {LuaMod m("function on_load() game.player.set_modifier('jump_extension',12) end;function on_update() ui.rect('viewport',0,0,30,10,10,20,30,128);ui.text('window',0,0,'TIMER');local c=game.get_clocks();assert(c.real_seconds==4 and c.paused_seconds==1 and c.simulation_seconds==2) end",2097152,2,gameplay_jump|ui_draw);m.load(o);check(m.jump_extension()==12,"lease missing");m.update(o);check(m.visual().size()==2&&m.visual()[1].space==1,"UI commands missing");m.unload();check(m.visual().empty()&&!m.jump_extension(),"cleanup");}
 for(auto source:{"game.player.set_modifier('jump_extension',13)","game.player.set_modifier('jump_extension',-1)","game.player.set_modifier('jump_extension',1.5)","game.player.set_modifier('speed',1)","ui.rect('viewport',319,0,2,1,0,0,0,0)","ui.text('viewport',319,239,'BAD')","ui.text('viewport',0,0,'ñ')","ui.rect('viewport',0,0,1,1,0,0,0,256)","ui.rect('world',0,0,1,1,0,0,0,255)","ui.rect('viewport',0,0,320,240,0,0,0,0);ui.rect('viewport',0,0,320,240,0,0,0,0)","for i=1,17 do ui.rect('viewport',0,0,1,1,0,0,0,0) end"})reject([&]{LuaMod m(std::string("function on_load() ")+source+" end",2097152,2,gameplay_jump|ui_draw);m.load(o);});
 reject([&]{LuaMod m("game.player.set_modifier('jump_extension',1)",2097152,2,gameplay_jump);});
 {LuaMod m("function on_load() game.player.set_modifier('jump_extension',12) end;function on_update() error('test') end",2097152,2,gameplay_jump);m.load(o);reject([&]{m.update(o);});check(!m.jump_extension()&&m.visual().empty(),"error retained lease");}
 ImageAsset asset{"own",{2,2,std::vector<uint8_t>(16,255)}};
 {LuaMod m("function on_update() graphics.layer('viewport','own',20,30,2,2,1,128,10,4,0.5) end",2097152,2,graphics_layers,{asset});m.load(o);m.update(o);check(m.visual()[0].x==15&&m.visual()[0].y==28,"parallax wrong");nes::ui::Bitmap b{320,240,std::vector<uint8_t>(320*240*4)};paint_visual(b,m.visual()[0],m.assets());check(b.rgba[(28*320+15)*4+3]==128,"alpha missing");m.unload();check(m.visual().empty(),"layer cleanup");}
 for(auto code:{"graphics.layer('viewport','missing',0,0,1,1,0,255,0,0,0)","graphics.layer('viewport','own',0,0,1,1,0,255,0,0,2)","graphics.layer('viewport','own',0,0,1,1,0,255,1/0,0,0)","graphics.layer('viewport','own',0,0,1,1,0,255,0,0,0);graphics.layer('viewport','own',0,0,1,1,0,255,0,0,0)"})reject([&]{LuaMod m(std::string("function on_load() ")+code+" end",2097152,2,graphics_layers,{asset});m.load(o);});
 reject([&]{auto bad=asset;bad.bitmap.rgba.clear();LuaMod m("",2097152,2,graphics_layers,{bad});});
 reject([&]{LuaMod m("",2097152,2,graphics_layers,{asset,asset});});
 for(auto wh:{std::pair<int,int>{960,720},{1280,720},{1920,1080},{300,200}}){auto r=visual_rect(wh.first,wh.second,false,0);check(r.width*3==r.height*4&&r.x>=0&&r.y>=0,"viewport aspect");auto f=visual_rect(wh.first,wh.second,true,0);check(f.width<=wh.first&&f.height<=wh.second,"fullscreen bounds");}
 SessionClock clock(10);clock.sample(12,false);clock.sample(13,true);clock.sample(16,true);clock.sample(17,false);clock.sample(19,false);check(clock.real_seconds()==9&&clock.paused_seconds()==4,"clock pause accounting");clock.sample(18,false);check(clock.real_seconds()==9,"clock reversal");
 if(argc==5){const auto dir=std::filesystem::path(argv[4]);{
  Manager m(argv[1],argv[2],argv[3],{"0.9.0","addams-family-pal-eu-enfrde"});
  for(auto filename:{"Super-Gomez-Gameplay-v1.0.zip","Speedrun-HUD-v1.0.zip","Mansion-Parallax-Demo-v1.0.zip"})m.install(dir/filename);check(!m.active(),"install executed/enabled mods");
  for(auto id:{"phase5.super-gomez-gameplay","phase5.speedrun-hud","phase5.mansion-parallax-demo"})m.enable(id,true);
  check(m.jump_extension()==12,"Manager lease forwarding");o.simulation_seconds=5;m.complete(o);check(m.visual(0).width==320,"Manager visual publication");auto saved=m.visual(0).rgba;auto rev=m.visual_revision();check(saved==m.visual(0).rgba&&rev==m.visual_revision(),"draw mutated capture");
  m.install(dir/"conflict.zip");reject([&]{m.enable("phase5.conflict",true);});check(m.jump_extension()==12,"conflict changed existing owner");m.uninstall("phase5.conflict");
  m.enable("phase5.super-gomez-gameplay",false);m.install(dir/"failed.zip");m.enable("phase5.failed",true);check(m.jump_extension()==12,"failed fixture did not acquire lease");m.complete(o);check(!m.jump_extension()&&m.active()&&m.visual(0).width==320,"one failing mod corrupted others/retained request");
  bool failed=false;for(const auto&r:m.records())if(r.id=="phase5.failed")failed=!r.enabled&&!r.error.empty();check(failed,"callback failure not persisted");m.uninstall("phase5.failed");m.enable("phase5.super-gomez-gameplay",true);
  o.toggle_ui=true;o.toggle_layers=true;m.complete(o);check(m.jump_extension()==12,"UI affected gameplay owner");o={};m.complete(o);
  for(auto id:{"phase5.super-gomez-gameplay","phase5.speedrun-hud","phase5.mansion-parallax-demo"})m.enable(id,false);check(!m.active()&&!m.jump_extension()&&!m.visual(0).width,"Disable cleanup");
  for(auto id:{"phase5.super-gomez-gameplay","phase5.speedrun-hud","phase5.mansion-parallax-demo"})m.uninstall(id);check(m.records().empty(),"Uninstall cleanup");
 }
 }
 // Isolate actual Lua and compositing microcosts, not monitor FPS.
 LuaMod bench("function on_update() ui.rect('viewport',0,0,100,40,0,0,0,100);ui.text('viewport',4,4,'OWN TIMER') end",2097152,2,ui_draw);bench.load(o);auto t=std::chrono::steady_clock::now();for(int i=0;i<10000;++i)bench.update(o);auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count()/10000;
 nes::ui::Bitmap b{320,240,std::vector<uint8_t>(320*240*4)};t=std::chrono::steady_clock::now();for(int i=0;i<1000;++i){std::fill(b.rgba.begin(),b.rgba.end(),0);for(const auto&c:bench.visual())paint_visual(b,c,{});}auto paint=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count()/1000;
 std::cout<<"Phase5 gameplay/UI/visual cases="<<n<<" PASS; callback_us="<<us<<" paint_us="<<paint<<" Lua_bytes="<<bench.memory_used()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
