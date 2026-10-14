#include "runtime/mod_manager.hpp"
#include "runtime/original_presentation.hpp"
#include "game/mod_observation_v2.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace nes::mods;
static unsigned checks{};
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);++checks;}
int main(int argc,char**argv){try{
 Observation o;o.frame=5;o.cpu_cycles=900;o.game=GameSnapshot{"own-synthetic","playing",0,PlayerSnapshot{76,160}};
 LuaMod old("assert(MOD_API_VERSION==1 and hud==nil); function on_update(dt) assert(game.get_status().room_id==7);assert(game.get_player()==nil) end");Observation legacy;legacy.room=7;old.update(legacy);require(old.updates()==1,"v1 contract changed");
 LuaMod v2("assert(MOD_API_VERSION==2 and nes==nil and game.write_ram==nil and os==nil);function on_update(dt) local s=game.get_status();assert(s.state=='playing' and s.room_id==0 and s.frame==5);local p=game.get_player();assert(p.x==76 and p.y==160 and p.space=='nes_screen');p.x=999;hud.text(4,2,'X '..tostring(p.y)) end",2*1024*1024,2,3);
 v2.update(o);require(v2.overlay().size()==1&&v2.overlay()[0].text=="X 160"&&o.game->player->x==76,"v2 observation aliases host or HUD not published");
 v2.unload();require(v2.overlay().empty(),"unload left overlay");
 LuaMod absent("assert(hud==nil and game.get_player==nil and game.write_ram==nil)",2*1024*1024,2,0);require(absent.overlay().empty(),"missing permission grants HUD");
 LuaMod unknown("function on_update()assert(game.get_player()==nil);assert(game.get_status().state=='unavailable') end",2*1024*1024,2,1);unknown.update({});require(unknown.updates()==1,"unknown player fabricated");
 for(auto source:{"function on_update() hud.text(-1,0,'A') end","function on_update() hud.text(0,34,'A') end","function on_update() hud.text(255,0,'A') end","function on_update() hud.text(0,0,string.char(0)) end","function on_update() hud.text(0,0,string.char(200)) end","function on_update() hud.text(0/0,0,'A') end","function on_update() hud.text(0,0,{}) end","function on_update() hud.text(0,0,'A',1) end","hud.text(0,0,'A')","function on_update()for i=1,9 do hud.text(0,0,'A') end end","function on_update()hud.text(0,0,'A');error('own error') end"}){
  bool failed=false;try{LuaMod m(source,2*1024*1024,2,2);m.update(o);}catch(...){failed=true;}require(failed,"invalid HUD accepted");
 }
 LuaMod transaction("local n=0;function on_update()n=n+1;hud.text(0,0,'A');if n==2 then error('own failure')end end",2*1024*1024,2,2);transaction.update(o);require(!transaction.overlay().empty(),"valid HUD absent");try{transaction.update(o);}catch(...){}require(transaction.overlay().empty(),"failed frame published partial/stale HUD");
 for(auto api:{0u,3u}){bool failed=false;try{LuaMod m("",2*1024*1024,api);}catch(...){failed=true;}require(failed,"unknown API constructor accepted");}
 std::array<uint8_t,131072> prg{},chr{};nes::runtime::Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});bus.ram().write(0x471,255);bus.ram().write(0x12,76);bus.ram().write(0x13,160);
 auto before=bus.cycles();auto observed=nes::game::mod_completed_observation(bus);require(observed.game->player&&observed.game->player->x==76&&observed.game->player->y==160&&bus.cycles()==before,"adapter unavailable or mutates clock");
 for(auto a:{0x45b,0xb1,0x80,0x70,0x600,0x72,0x7c,0x60d,0x61c}){bus.ram().write(a,1);require(!nes::game::mod_completed_observation(bus).game->player,"unsupported context exposed player");bus.ram().write(a,0);}
 bus.ram().write(0x71,2);observed=nes::game::mod_completed_observation(bus);require(!observed.game->player&&observed.game->room==2,"v2 level ID mixes screen selector");bus.ram().write(0x71,0);
 bus.ram().write(0x471,1);require(nes::game::mod_completed_observation(bus).game->state=="transition","transition not unknown");bus.ram().write(0x471,255);
 const auto image=nes::ui::text_overlay_bitmap(256,40,{{4,2,"OWN HUD"}});require(image.rgba.size()==256*40*4,"HUD raster bound");
 for(auto size:{std::pair<int,int>{960,720},{1920,1080},{319,239},{0,0}})for(bool fullscreen:{false,true}){auto v=nes::runtime::original_viewport(size.first,size.second,fullscreen);auto rect=hud_rect(size.first,size.second,fullscreen);require(rect.x==v.x&&rect.y==v.y&&rect.width==v.width&&rect.height<=v.height,"HUD escapes original 4:3 viewport");}
 if(argc==5){Manager manager(argv[1],argv[2],argv[3],{"0.9.0",nes::game::mod_game_id});manager.install(argv[4]);require(!manager.active()&&!manager.overlay().width,"ZIP auto enabled");manager.enable("nes-native-lab.gomez-debug-hud",true);require(manager.active()&&manager.overlay().width,"v2 ZIP not enabled");auto revision=manager.overlay_revision();manager.update({});require(manager.overlay_revision()==revision,"v1 update executes v2 HUD");o.game->game_id=nes::game::mod_game_id;manager.complete(o);require(manager.overlay_revision()>revision,"completed unit did not publish v2 HUD");manager.enable("nes-native-lab.gomez-debug-hud",false);require(!manager.overlay().width&&!manager.active(),"disable left HUD");manager.uninstall("nes-native-lab.gomez-debug-hud");require(manager.records().empty(),"uninstall failed");}
 std::cout<<"Mod API v2 checks="<<checks<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
