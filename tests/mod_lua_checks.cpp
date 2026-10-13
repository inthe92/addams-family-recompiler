#include "runtime/mod_lua.hpp"
#include "runtime/mod_panel.hpp"
#include "runtime/frontend_ui.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace nes::mods;
static void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
int main(){try{
    unsigned cases=0;
    LuaMod lifecycle("local n=0; function on_load() log.info('load') end; function on_update(dt) assert(math.abs(dt-1/50.007)<1e-10); n=n+1; log.info(tostring(n)) end; function on_unload() log.info('unload') end");
    lifecycle.load({});lifecycle.update({1,20,7,Player{5,9}});lifecycle.unload();require(lifecycle.logs().size()==3&&lifecycle.logs()[1]=="1"&&lifecycle.updates()==1,"lifecycle and PAL dt");++cases;
    for(const auto* code:{"assert(os==nil)","assert(io==nil)","assert(package==nil and require==nil)","assert(debug==nil)","assert(load==nil and dofile==nil and loadfile==nil)","assert(pcall==nil and xpcall==nil)","assert(coroutine==nil)","assert(getmetatable==nil and setmetatable==nil)","assert(nes==nil)","assert(string.dump==nil and string.find==nil and table.sort==nil)","assert(math.random==nil)","assert(_G.os==nil and _G.debug==nil)"}){LuaMod m(code);++cases;}
    for(const auto* code:{"not Lua @","while true do end","local t={} while true do t[#t+1]=string.char(65)..tostring(#t) end","function on_update() while true do end end","function on_update() error('bad script') end","on_update=42"}){
        bool failed=false;try{LuaMod m(code);m.update({});}catch(const std::runtime_error&){failed=true;}require(failed,"syntax/budget/memory/callback error escaped containment");++cases;
    }
    LuaMod a("isolated=17"),b("assert(isolated==nil)");++cases;
    LuaMod copy("function on_update() local s=game.get_status(); s.frame=999; local p=game.get_player(); assert(p.x==5 and p.y==9); p.x=999 end");Observation o{3,9,7,Player{5,9}};copy.update(o);require(o.frame==3&&o.player->x==5,"API snapshot aliases live state");++cases;
    LuaMod missing("function on_load() assert(game.get_player()==nil) end");missing.load({});++cases;
    LuaMod logs("for i=1,1000 do log.info('own text') end");require(logs.logs().size()==128,"unbounded logs");++cases;
    bool failed=false;try{LuaMod binary(std::string("\x1bLua",4));}catch(...){failed=true;}require(failed,"binary chunk accepted");++cases;
    std::cout<<"Managed Lua cases="<<cases<<" PASS\n";
    Panel panel;panel.open();panel.records({{"test","Example","1.0.0","Tests","Desc","",false}});unsigned ui=0;
    require(panel.visible()&&panel.bitmap().width==640,"panel bitmap");++ui;
    require(panel.handle(PanelAction::activate)==Command::install,"install navigation");++ui;
    panel.begin_events();panel.handle(PanelAction::right);require(panel.handle(PanelAction::activate)==Command::toggle,"toggle navigation");++ui;
    require(panel.handle(PanelAction::activate)==Command::none,"repeated activation same batch");++ui;
    panel.begin_events();panel.handle(PanelAction::right);require(panel.handle(PanelAction::activate)==Command::none,"uninstall needs confirmation");++ui;
    panel.begin_events();require(panel.handle(PanelAction::activate)==Command::uninstall,"uninstall explicit confirmation");++ui;
    panel.begin_events();require(panel.handle(PanelAction::back)==Command::close&&!panel.visible(),"back");++ui;
    nes::ui::FrontendUI game;game.external_pause(true);require(game.paused()&&!game.game_input_enabled(),"pause/input barrier");game.external_pause(false);require(!game.game_input_enabled(),"resume input barrier");game.observe_neutral(true,true);require(game.game_input_enabled(),"neutral resume");++ui;
    for(auto size:{std::pair<int,int>{960,720},{1280,720},{319,239}}){auto r=Panel::rect(size.first,size.second);require(r.width>0&&r.height>0&&r.x>=0&&r.y>=0&&r.width<=size.first&&r.height<=size.second,"responsive panel");++ui;}
    std::cout<<"Mod panel cases="<<ui<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
