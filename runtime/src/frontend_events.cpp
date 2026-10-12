#include "runtime/frontend_events.hpp"
#include <algorithm>
#include <cstdlib>
namespace nes::ui {
namespace { bool pressed(SDL_GameController* p,SDL_GameControllerButton b){return p&&SDL_GameControllerGetButton(p,b);} }
uint8_t game_buttons(const Uint8* keys,SDL_GameController* pad){
 uint8_t value=0;
 const struct {SDL_Scancode key;SDL_GameControllerButton button;uint8_t mask;} map[]={
 {SDL_SCANCODE_Z,SDL_CONTROLLER_BUTTON_A,1},{SDL_SCANCODE_X,SDL_CONTROLLER_BUTTON_B,2},
 {SDL_SCANCODE_RSHIFT,SDL_CONTROLLER_BUTTON_BACK,4},{SDL_SCANCODE_RETURN,SDL_CONTROLLER_BUTTON_START,8},
 {SDL_SCANCODE_UP,SDL_CONTROLLER_BUTTON_DPAD_UP,16},{SDL_SCANCODE_DOWN,SDL_CONTROLLER_BUTTON_DPAD_DOWN,32},
 {SDL_SCANCODE_LEFT,SDL_CONTROLLER_BUTTON_DPAD_LEFT,64},{SDL_SCANCODE_RIGHT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,128}};
 for(const auto& b:map)if(keys[b.key]||pressed(pad,b.button))value=static_cast<uint8_t>(value|b.mask);
 if(pad){const auto x=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),y=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);if(x<-16000)value|=64;if(x>16000)value|=128;if(y<-16000)value|=16;if(y>16000)value|=32;}
 return value;
}
unsigned modal_pressed_count(const Uint8* keys,SDL_GameController* pad){
 unsigned count=0;for(auto k:{SDL_SCANCODE_ESCAPE,SDL_SCANCODE_RETURN,SDL_SCANCODE_SPACE,SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,SDL_SCANCODE_TAB})count+=keys[k]!=0;
 for(auto b:{SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_GUIDE,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN})count+=pressed(pad,b);
 if(pad)count+=std::abs(int(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX)))>16000;
 return count;
}
bool modal_controls_idle(const Uint8* keys,SDL_GameController* pad){return modal_pressed_count(keys,pad)==0&&SDL_GetMouseState(nullptr,nullptr)==0;}
bool all_controls_idle(const Uint8* keys,SDL_GameController* pad){return game_buttons(keys,pad)==0&&modal_controls_idle(keys,pad)&&!keys[SDL_SCANCODE_F1]&&!pressed(pad,SDL_CONTROLLER_BUTTON_Y);}
void FrontendEvents::handle(const SDL_Event& e,int ww,int wh,int dw,int dh,bool simultaneous){
 if(e.type==SDL_QUIT){if(!ui_.modal())ui_.handle(Action::request_exit);return;}
 if(e.type==SDL_WINDOWEVENT){if(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST){ui_.handle(Action::focus_lost);held_keys_.clear();held_buttons_.clear();axis_direction_=0;}if(e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)ui_.handle(Action::focus_gained);return;}
 if(e.type==SDL_KEYUP){held_keys_.erase(e.key.keysym.sym);return;}
 if(e.type==SDL_CONTROLLERBUTTONUP){held_buttons_.erase({e.cbutton.which,e.cbutton.button});return;}
 if(e.type==SDL_KEYDOWN){auto key=e.key.keysym.sym;bool repeat=e.key.repeat!=0||!held_keys_.insert(key).second;
  if(key==SDLK_ESCAPE)ui_.handle(ui_.modal()?Action::cancel:Action::request_exit,repeat);
  else if(key==SDLK_q&&(e.key.keysym.mod&KMOD_GUI))ui_.handle(Action::request_exit,repeat);
  else if(key==SDLK_F1)ui_.handle(Action::dismiss_help,repeat);
  else if(ui_.modal()){
   if(key==SDLK_RETURN||key==SDLK_SPACE)ui_.handle(Action::confirm,repeat,simultaneous);
   if(key==SDLK_LEFT||key==SDLK_UP)ui_.handle(Action::previous,repeat,simultaneous);
   if(key==SDLK_RIGHT||key==SDLK_DOWN)ui_.handle(Action::next,repeat,simultaneous);
   if(key==SDLK_TAB)ui_.handle(ui_.exit_selected()?Action::previous:Action::next,repeat,simultaneous);
  }
 }
 if(e.type==SDL_CONTROLLERBUTTONDOWN){
  if(!held_buttons_.insert({e.cbutton.which,e.cbutton.button}).second)return;
  if(e.cbutton.button==SDL_CONTROLLER_BUTTON_GUIDE)ui_.handle(ui_.modal()?Action::cancel:Action::request_exit);
  else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_Y)ui_.handle(Action::dismiss_help);
  else if(ui_.modal()){
   if(e.cbutton.button==SDL_CONTROLLER_BUTTON_A)ui_.handle(Action::confirm,false,simultaneous);
   if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B)ui_.handle(Action::cancel,false,simultaneous);
   if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT||e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)ui_.handle(Action::previous,false,simultaneous);
   if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT||e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)ui_.handle(Action::next,false,simultaneous);
  }
 }
 if(e.type==SDL_CONTROLLERAXISMOTION&&e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX){const int direction=e.caxis.value<-16000?-1:e.caxis.value>16000?1:0;if(ui_.modal()&&direction&&axis_direction_==0)ui_.handle(direction<0?Action::previous:Action::next,false,simultaneous);axis_direction_=direction;}
 if(!simultaneous&&ui_.modal()&&e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT&&ww>0&&wh>0){
  const auto r=overlay_rect(dw,dh,true);if(!r.width||!r.height)return;
  const int x=int((double(e.button.x)*dw/ww-r.x)*640/r.width),y=int((double(e.button.y)*dh/wh-r.y)*180/r.height);
  if(cancel_button().contains(x,y))ui_.click_button(false);else if(exit_button().contains(x,y))ui_.click_button(true);
 }
}
}
