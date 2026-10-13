#include "runtime/mod_frontend.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(int argc,char**argv){SDL_SetMainReady();try{
    if(argc!=2)throw std::runtime_error("Private non-existing test storage required");
    nes::mods::Frontend f(true,argv[1]);unsigned n=0;SDL_Event e{};
    const auto key=[&](SDL_Keycode code,int repeat=0){e={};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;e.key.repeat=Uint8(repeat);return f.handle(e,960,720,960,720);};
    const auto release=[&](SDL_Keycode code){e={};e.type=SDL_KEYUP;e.key.keysym.sym=code;f.handle(e,960,720,960,720);};
    require(!f.visible()&&!f.active(),"safe startup loaded mods");++n;
    require(key(SDLK_F2)&&f.visible(),"F2 open");++n;
    require(key(SDLK_F2,1)&&f.visible(),"repeat unexpectedly closes");++n;
    key(SDLK_RETURN);require(!f.active()&&!std::filesystem::exists(argv[1]),"safe mode installed or loaded external files");++n;
    release(SDLK_RETURN);f.begin_events();key(SDLK_ESCAPE);require(!f.visible(),"Escape did not dismiss mod manager");++n;
    release(SDLK_ESCAPE);release(SDLK_F2);e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    require(f.handle(e,960,720,960,720)&&f.visible(),"controller open");++n;
    require(f.handle(e,960,720,960,720)&&f.visible(),"duplicate controller press");++n;
    f.begin_events();e.cbutton.button=SDL_CONTROLLER_BUTTON_B;f.handle(e,960,720,960,720);require(!f.visible(),"controller Back");++n;
    f.show();e={};e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;require(!f.handle(e,960,720,960,720),"focus must reach original frontend");
    e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=SDL_CONTROLLER_BUTTON_B;f.handle(e,960,720,960,720);require(f.visible(),"background controller changed mod state");++n;
    e={};e.type=SDL_QUIT;require(!f.handle(e,960,720,960,720)&&!f.visible(),"normal close must reach exit confirmation");++n;
    require(!std::filesystem::exists(argv[1]),"safe frontend wrote storage");++n;
    std::cout<<"Managed frontend cases="<<n<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
