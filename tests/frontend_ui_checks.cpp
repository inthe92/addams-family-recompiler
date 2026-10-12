#include "runtime/frontend_ui.hpp"
#include "runtime/ui_text.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace nes::ui;
static void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static void open(FrontendUI& u){u.begin_events();u.handle(Action::request_exit);u.observe_neutral(true,true);u.begin_events();}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("banner|quit|bitmap required");std::string mode=argv[1];unsigned count=0;
 if(mode=="banner"){
  FrontendUI u;require(!u.banner_visible(0),"help before first frame");++count;
  u.start(50);require(u.banner_visible(50)&&u.banner_visible(59.999),"real-time help interval");++count;
  require(!u.banner_visible(60)&&!u.banner_visible(100),"help must expire at ten seconds");++count;
  require(!u.paused()&&u.game_input_enabled(),"help blocks game");++count;
  u.start(100);u.handle(Action::dismiss_help);require(!u.banner_visible(100.1)&&u.game_input_enabled(),"dismissal consumes game input");++count;
  u.start(200);for(int i=0;i<100000;++i)require(u.banner_visible(201),"emulated frame count changed time");++count;
  require(u.banner_alpha(200)==0&&u.banner_alpha(200.2)==1&&u.banner_alpha(209.9)<1,"fade does not use monotonic interval");++count;
  u.handle(Action::focus_lost);require(!u.banner_visible(211),"real-time expiration stopped on focus loss");++count;
  u.handle(Action::focus_gained);u.observe_neutral(true,true);require(!u.paused()&&u.game_input_enabled(),"focus resume");++count;
 }else if(mode=="quit"){
  FrontendUI u;u.begin_events();u.handle(Action::request_exit);require(u.modal()&&u.paused()&&!u.exit_selected()&&!u.armed(),"default Cancel");++count;
  u.handle(Action::confirm);u.handle(Action::next);require(!u.quit_requested()&&!u.exit_selected(),"opening event leaked into dialog");++count;
  u.observe_neutral(false,false);u.begin_events();u.handle(Action::confirm);require(u.modal()&&!u.quit_requested(),"held controls armed confirmation");++count;
  u.observe_neutral(true,true);u.begin_events();u.handle(Action::confirm);require(!u.modal()&&!u.quit_requested()&&!u.game_input_enabled(),"Enter on Cancel must cancel and gate held input");++count;
  u.observe_neutral(true,false);require(!u.game_input_enabled(),"held movement restored");++count;
  u.observe_neutral(true,true);require(u.game_input_enabled(),"released input not restored");++count;
  open(u);u.handle(Action::cancel);require(!u.modal()&&!u.quit_requested(),"Escape/B cancellation");++count;
  u.begin_events();u.handle(Action::request_exit);require(!u.modal(),"duplicate closing/opening request");++count;
  u.observe_neutral(true,true);open(u);u.handle(Action::next);u.handle(Action::confirm);require(u.modal()&&!u.quit_requested(),"navigation and confirmation coalesced");++count;
  u.begin_events();u.handle(Action::confirm,true);require(!u.quit_requested(),"repeat confirmation");++count;
  u.handle(Action::confirm,false,true);require(!u.quit_requested(),"simultaneous confirmation");++count;
  u.begin_events();u.handle(Action::confirm);require(u.quit_requested(),"explicit Exit confirmation");++count;
  FrontendUI m;open(m);m.click_button(false);require(!m.modal()&&!m.quit_requested(),"mouse Cancel");++count;
  m.observe_neutral(true,true);open(m);m.click_button(true);require(m.quit_requested(),"mouse Exit");++count;
  FrontendUI f;open(f);f.handle(Action::next);f.handle(Action::focus_lost);require(f.paused()&&!f.game_input_enabled(),"focus loss leaves active input");++count;
  f.handle(Action::confirm);require(!f.quit_requested(),"background confirmation");++count;
  f.handle(Action::focus_gained);f.begin_events();f.handle(Action::confirm);require(!f.quit_requested(),"focus gain without release gate");++count;
  f.observe_neutral(true,true);f.begin_events();f.handle(Action::cancel);require(!f.paused(),"cancel after focus returns");++count;
 }else if(mode=="bitmap"){
  FrontendUI u;u.start(0);auto b=overlay_bitmap(u,1);require(b.width==640&&b.height==112&&b.rgba.size()==640*112*4,"help bitmap");++count;
  require(b.rgba[3]==0&&b.rgba[(30*640+620)*4+3]>0,"rounded/translucent panel");++count;
  require(overlay_bitmap(u,10).rgba.empty(),"expired bitmap remains");++count;
  open(u);auto d=overlay_bitmap(u,10);require(d.width==640&&d.height==180,"dialog bitmap");++count;
  for(auto size:{std::pair<int,int>{960,720},{1920,1080},{3840,2160},{600,900},{256,192}})for(bool modal:{true,false}){auto r=overlay_rect(size.first,size.second,modal);require(r.width>0&&r.height>0&&r.x>=0&&r.y>=0&&r.x+r.width<=size.first&&r.y+r.height<=size.second,"overlay resize/fullscreen bounds");}++count;
  require(overlay_rect(0,0,true).width==0,"minimized overlay");++count;
  require(cancel_button().contains(111,113)&&!cancel_button().contains(300,113)&&exit_button().contains(351,113),"mouse hit boundaries");++count;
  require(std::string(nes::app::version)=="0.9.0-beta","beta identity");++count;
 }else throw std::runtime_error("unknown contract");
 std::cout<<"Frontend "<<mode<<" cases="<<count<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
