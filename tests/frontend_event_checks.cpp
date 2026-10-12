#include "runtime/frontend_events.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace nes::ui;
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 SDL_SetMainReady();require(SDL_Init(SDL_INIT_EVENTS)==0,"SDL event initialization");
 FrontendUI ui;FrontendEvents events(ui);unsigned cases=0;
 const auto key=[&](SDL_Keycode code,bool repeat=false,bool chord=false){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;e.key.repeat=repeat;events.handle(e,960,720,1920,1440,chord);e.type=SDL_KEYUP;events.handle(e,960,720,1920,1440);};
 key(SDLK_ESCAPE);require(ui.modal()&&!ui.quit_requested(),"Escape must open");++cases;
 key(SDLK_ESCAPE,true);require(ui.modal(),"opening Escape repeat must not cancel");++cases;
 ui.observe_neutral(true,true);ui.begin_events();key(SDLK_RETURN);require(!ui.modal()&&!ui.quit_requested(),"initial Enter selects Cancel");++cases;
 ui.observe_neutral(true,true);ui.begin_events();SDL_Event close{};close.type=SDL_QUIT;events.handle(close,960,720,1920,1440);require(ui.modal(),"normal OS close ignored");++cases;
 ui.observe_neutral(true,true);ui.begin_events();key(SDLK_ESCAPE);require(!ui.modal(),"Escape cancels");++cases;
 ui.observe_neutral(true,true);ui.begin_events();SDL_Event pad{};pad.type=SDL_CONTROLLERBUTTONDOWN;pad.cbutton.button=SDL_CONTROLLER_BUTTON_GUIDE;events.handle(pad,960,720,1920,1440);require(ui.modal(),"gamepad menu");++cases;
 ui.observe_neutral(true,true);ui.begin_events();pad.cbutton.button=SDL_CONTROLLER_BUTTON_B;events.handle(pad,960,720,1920,1440);require(!ui.modal(),"gamepad cancel");++cases;
 ui.observe_neutral(true,true);ui.begin_events();key(SDLK_ESCAPE);ui.observe_neutral(true,true);ui.begin_events();key(SDLK_RIGHT);ui.begin_events();key(SDLK_RETURN,false,true);require(!ui.quit_requested(),"simultaneous keys confirmed");++cases;
 ui.begin_events();key(SDLK_LEFT);ui.begin_events();key(SDLK_RETURN);require(!ui.modal()&&!ui.quit_requested(),"keyboard choice");++cases;
 std::array<Uint8,SDL_NUM_SCANCODES> keys{};keys[SDL_SCANCODE_Z]=1;keys[SDL_SCANCODE_RIGHT]=1;require(game_buttons(keys.data(),nullptr)==129,"game mapping changed/combined input lost");++cases;
 ui.observe_neutral(true,true);ui.begin_events();key(SDLK_ESCAPE);ui.observe_neutral(true,true);ui.begin_events();
 SDL_Event mouse{};mouse.type=SDL_MOUSEBUTTONUP;mouse.button.button=SDL_BUTTON_LEFT;auto r=overlay_rect(1920,1440,true);mouse.button.x=(r.x+int(r.width*(440.0/640)))/2;mouse.button.y=(r.y+int(r.height*(134.0/180)))/2;events.handle(mouse,960,720,1920,1440);require(ui.quit_requested(),"Retina mouse explicit Exit");++cases;
 FrontendUI f;FrontendEvents fe(f);SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;fe.handle(focus,960,720,960,720);require(f.paused()&&!f.game_input_enabled(),"focus loss");focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;fe.handle(focus,960,720,960,720);f.observe_neutral(true,false);require(!f.game_input_enabled(),"focus restored held movement");f.observe_neutral(true,true);require(f.game_input_enabled(),"focus/input neutral resume");++cases;
 FrontendUI edge;FrontendEvents ee(edge);SDL_Event duplicate{};duplicate.type=SDL_KEYDOWN;duplicate.key.keysym.sym=SDLK_ESCAPE;
 ee.handle(duplicate,960,720,960,720);edge.observe_neutral(true,true);edge.begin_events();ee.handle(duplicate,960,720,960,720);
 require(edge.modal()&&!edge.quit_requested(),"duplicate key-down without repeat flag cancels menu");duplicate.type=SDL_KEYUP;ee.handle(duplicate,960,720,960,720);duplicate.type=SDL_KEYDOWN;ee.handle(duplicate,960,720,960,720);require(!edge.modal(),"new Escape edge must cancel");++cases;
 SDL_Quit();std::cout<<"Frontend SDL event cases="<<cases<<" PASS\n";return 0;
}catch(const std::exception& e){SDL_Quit();std::cerr<<e.what()<<'\n';return 1;}}
