#include "runtime/mod_panel.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace nes::mods;
static unsigned n;
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);++n;}
static Command act(Panel&p,PanelAction a){p.begin_events();return p.handle(a);}
int main(int argc,char**argv){try{
 Panel p;p.main();check(p.page()==Page::main&&p.visible(),"main menu");act(p,PanelAction::right);act(p,PanelAction::activate);check(p.page()==Page::installed,"Mods route");
 Record r{"test.example","Example","1.0.0","Tests","Own description","",false,2,32,false};r.category="Gameplay";r.dependencies="test.helper 1.0.0 to <2.0.0";r.conflicts="test.other";r.settings={{"amount","Jump amount","integer","6",0,12,1,{}}};p.records({r});p.profiles({{"Original","No mods",false},{"Gameplay","test.example",true}});
 p.begin_events();p.shortcut('d');check(p.page()==Page::details&&!p.bitmap().rgba.empty(),"details");act(p,PanelAction::down);check(p.bitmap().height==430,"details scroll");act(p,PanelAction::back);check(p.page()==Page::installed&&p.visible(),"details Back");
 p.begin_events();p.shortcut('c');check(p.page()==Page::configure&&p.settings()[0].value=="6","configure");act(p,PanelAction::right);act(p,PanelAction::activate);check(p.settings()[0].value=="7","integer editing");act(p,PanelAction::right);check(act(p,PanelAction::activate)==Command::configure,"Apply settings command");act(p,PanelAction::back);check(p.page()==Page::installed,"configuration Back");
 p.begin_events();p.shortcut('p');act(p,PanelAction::down);check(p.selected_profile()->name=="Gameplay","profile navigation");check(act(p,PanelAction::activate)==Command::none,"profile confirmation required");check(p.handle(PanelAction::activate)==Command::none,"same batch no apply");check(act(p,PanelAction::activate)==Command::profile_apply,"profile explicit apply");act(p,PanelAction::right);act(p,PanelAction::activate);check(p.page()==Page::name,"save profile name");p.text(" Test");check(act(p,PanelAction::activate)==Command::profile_save,"save profile command");act(p,PanelAction::back);act(p,PanelAction::back);check(!p.visible(),"manager-only Back closes");
 p.main();act(p,PanelAction::right);act(p,PanelAction::right);act(p,PanelAction::activate);check(p.page()==Page::settings,"settings menu");act(p,PanelAction::activate);check(p.fullscreen,"launch fullscreen");act(p,PanelAction::right);act(p,PanelAction::activate);check(p.muted,"launch mute");act(p,PanelAction::back);check(p.page()==Page::main,"settings Back");check(act(p,PanelAction::activate)==Command::play&&!p.visible(),"Play");
 check(p.handle(PanelAction::activate,true)==Command::none,"closed ignores repeats");
 LuaMod typed("assert(config.get('integer')==6 and math.type(config.get('integer'))=='integer');assert(config.get('flag')==true);assert(config.get('ratio')==0.5);assert(config.get('choice')=='top')",2*1024*1024,2,0,{},{{"integer","Integer","integer","6",0,12,1,{}},{"flag","Flag","boolean","true",0,0,1,{}},{"ratio","Ratio","float","0.5",0,1,0.1,{}},{"choice","Choice","enum","top",0,0,1,{"top","bottom"}}});check(typed.memory_used()>0,"typed config getter");
 for(auto value:{"13","nan","true","1.5"}){bool rejected=false;try{LuaMod invalid("",2*1024*1024,2,0,{},{{"amount","Amount","integer",value,0,12,1,{}}});}catch(...){rejected=true;}check(rejected,"invalid native config accepted");}
 bool unknown=false;try{LuaMod bad("config.get('undeclared')",2*1024*1024,2);}catch(...){unknown=true;}check(unknown,"undeclared config accepted");
 LuaMod legacy("assert(config==nil)");check(legacy.memory_used()>0,"v1 config compatibility");
 if(argc==6){
  std::filesystem::path root=argv[1];std::string python=argv[2];std::filesystem::path helper=argv[3],zips=argv[4];Target target{"0.9.0","addams-family-pal-eu-enfrde"};
  {Manager m(root,python,helper,target);for(const char*zip:{"Super-Gomez-Gameplay-v1.0.zip","Speedrun-HUD-v1.0.zip","Mansion-Parallax-Demo-v1.0.zip"})m.install(zips/zip);
   check(m.records().size()==3&&!m.active(),"three original ZIPs disabled");auto rows=m.records();for(auto r:rows)m.enable(r.id,true);Observation o;o.frame=1;o.simulation_seconds=1.0/50.007;m.complete(o);check(m.jump_extension()==12&&m.visual(0).width==320,"three simultaneous capabilities");m.profile("save","Gameplay and Visual");m.profile("apply","Original");check(!m.active()&&!m.jump_extension()&&m.records().size()==3,"Original without deleting");m.profile("apply","Gameplay and Visual");m.complete(o);check(m.jump_extension()==12&&m.visual(0).width==320,"restore profile");m.rename_profile("Gameplay and Visual","My Profile");check(m.profiles().size()==2,"rename");m.profile("apply","Original");m.install(argv[5]);auto id=std::string("nes-native-lab.configurable-jump");m.enable(id,true);check(m.jump_extension()==6,"declared Lua default used");
   auto r=*std::find_if(m.records().begin(),m.records().end(),[&](auto&r){return r.id==id;});for(auto&s:r.settings)if(s.key=="extension")s.value="9";m.configure(id,r.settings);check(m.jump_extension()==9,"configuration actual modifier changed");m.profile("save","Configured");m.profile("apply","Original");m.profile("apply","Configured");check(m.jump_extension()==9,"profile settings restored");bool bad=false;try{m.enable("phase5.super-gomez-gameplay",true);}catch(...){bad=true;}check(bad&&m.jump_extension()==9,"conflict leaves owner intact");m.profile("apply","Original");}
  {Manager m(root,python,helper,target);check(!m.active()&&m.records().size()==4,"normal persistence");m.profile("apply","Configured");check(m.jump_extension()==9,"restart settings/profile persistence");m.profile("apply","Original");m.install(zips/"bad-unload.zip");m.enable("test.bad-unload",true);m.configure("test.bad-unload",{{"flag","Flag","boolean","true",0,0,1,{}}});check(!m.active()&&!m.error().empty(),"configuration unload failure concealed");m.enable("test.bad-unload",true);m.profile("apply","Original");check(!m.active()&&!m.error().empty(),"profile unload failure concealed");}
  {std::ofstream marker(root/"session-active");marker<<"Synthetic interrupted session";}
  {Manager m(root,python,helper,target);check(!m.active()&&m.error().find("unclean")!=std::string::npos,"unclean session recovery");}
 }
 std::cout<<"Completion native checks="<<n<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
