// Public synthetic UI only. A real GL context is mandatory, never skipped.
#include "runtime/mod_frontend.hpp"
#include <SDL_opengl.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
static unsigned n;
static void check(bool ok,const char*s){if(!ok)throw std::runtime_error(s);++n;}
int main(int argc,char**argv){SDL_SetMainReady();SDL_Window*w=nullptr;SDL_GLContext gl=nullptr;try{
 check(argc==3,"Fresh private store and ZIP directory required");
#ifdef __APPLE__
 SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,"0");
#endif
 check(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 w=SDL_CreateWindow("NES Native Lab - Completion interface probe",100,100,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE);check(w,SDL_GetError());gl=SDL_GL_CreateContext(w);check(gl,SDL_GetError());SDL_GL_SetSwapInterval(0);
 {nes::runtime::GLPresenter presenter(w);nes::mods::Frontend f(false,argv[1],{"0.9.0","addams-family-pal-eu-enfrde"});
 auto key=[&](SDL_Keycode k){f.begin_events();SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;f.handle(e,960,720,960,720);e.type=SDL_KEYUP;f.handle(e,960,720,960,720);};
 auto pad=[&](Uint8 k){f.begin_events();SDL_Event e{};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=k;f.handle(e,960,720,960,720);e.type=SDL_CONTROLLERBUTTONUP;f.handle(e,960,720,960,720);};
 std::array<uint8_t,256*240>pixels{};pixels.fill(0x20);auto source=pixels;
 auto draw=[&]{SDL_PumpEvents();int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);presenter.present(pixels.data(),256,240);f.draw(presenter,dw,dh);glFinish();check(glGetError()==GL_NO_ERROR&&pixels==source,"UI corrupted render/input source");std::vector<uint8_t>out(size_t(dw)*dh*4);glReadPixels(0,0,dw,dh,GL_RGBA,GL_UNSIGNED_BYTE,out.data());return out;};
 f.main_menu();auto main=draw();pad(SDL_CONTROLLER_BUTTON_DPAD_DOWN);pad(SDL_CONTROLLER_BUTTON_A);auto mods=draw();check(main!=mods&&f.visible(),"gamepad main to Mods");pad(SDL_CONTROLLER_BUTTON_B);check(draw()==main&&f.visible(),"Back from main Mods returns main");key(SDLK_DOWN);key(SDLK_RETURN);
 for(auto file:{"Super-Gomez-Gameplay-v1.0.zip","Speedrun-HUD-v1.0.zip","Mansion-Parallax-Demo-v1.0.zip"}){SDL_Event e{};e.type=SDL_DROPFILE;e.drop.file=SDL_strdup((std::filesystem::path(argv[2])/file).string().c_str());f.handle(e,960,720,960,720);}check(!f.active(),"installation auto-enables");key(SDLK_RIGHT);for(int i=0;i<3;++i){key(SDLK_RETURN);if(i<2)key(SDLK_DOWN);}check(f.jump_extension()==12,"three mods enabled through UI");
 auto installed=draw();pad(SDL_CONTROLLER_BUTTON_X);check(draw()!=installed,"Details controller shortcut");pad(SDL_CONTROLLER_BUTTON_B);pad(SDL_CONTROLLER_BUTTON_Y);check(draw()!=installed,"Configure no-options page");pad(SDL_CONTROLLER_BUTTON_B);pad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);pad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);pad(SDL_CONTROLLER_BUTTON_A);pad(SDL_CONTROLLER_BUTTON_A); // save Profile1
 pad(SDL_CONTROLLER_BUTTON_A);pad(SDL_CONTROLLER_BUTTON_A);check(!f.active(),"Original profile disabled all");pad(SDL_CONTROLLER_BUTTON_DPAD_DOWN);pad(SDL_CONTROLLER_BUTTON_A);pad(SDL_CONTROLLER_BUTTON_A);check(f.jump_extension()==12,"restored combined profile");
 // Apply Original, install a fourth independent configuration example, exercise UI Apply.
 pad(SDL_CONTROLLER_BUTTON_DPAD_UP);pad(SDL_CONTROLLER_BUTTON_A);pad(SDL_CONTROLLER_BUTTON_A);
 pad(SDL_CONTROLLER_BUTTON_B);
 {SDL_Event e{};e.type=SDL_DROPFILE;e.drop.file=SDL_strdup((std::filesystem::path(argv[2])/"Configurable-Jump-Example-v1.0.zip").string().c_str());f.handle(e,960,720,960,720);}
 key(SDLK_UP);key(SDLK_UP);key(SDLK_RIGHT);key(SDLK_RETURN);check(f.jump_extension()==6,"config example Enable");pad(SDL_CONTROLLER_BUTTON_Y);key(SDLK_RIGHT);key(SDLK_RETURN);key(SDLK_RIGHT);key(SDLK_RETURN);check(f.jump_extension()==7,"Frontend Configure applied real Lua setting");pad(SDL_CONTROLLER_BUTTON_B);
 pad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);pad(SDL_CONTROLLER_BUTTON_A);pad(SDL_CONTROLLER_BUTTON_A);check(!f.active(),"profile Original after configuration");pad(SDL_CONTROLLER_BUTTON_B);
 pad(SDL_CONTROLLER_BUTTON_B);check(f.visible()&&!f.play_requested(),"main return preserves navigation");
 key(SDLK_DOWN);key(SDLK_DOWN);key(SDLK_RETURN);draw();key(SDLK_RETURN);key(SDLK_RIGHT);key(SDLK_RETURN);check(f.launch_fullscreen()&&f.launch_muted(),"launch Settings");key(SDLK_ESCAPE);key(SDLK_RETURN);check(f.play_requested()&&!f.visible(),"Play selected deliberately");
 nes::mods::Frontend game(true,std::string(argv[1])+"-safe");game.show();game.begin_events();SDL_Event e{};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=SDL_CONTROLLER_BUTTON_B;game.handle(e,960,720,960,720);check(!game.visible()&&!std::filesystem::exists(std::string(argv[1])+"-safe"),"game/safe Back closes without storage");
 f.show();SDL_SetWindowSize(w,1280,720);draw();check(SDL_SetWindowFullscreen(w,SDL_WINDOW_FULLSCREEN_DESKTOP)==0,SDL_GetError());draw();SDL_SetWindowFullscreen(w,0);draw();
 }
 SDL_GL_DeleteContext(gl);SDL_DestroyWindow(w);SDL_Quit();std::cout<<"Completion real SDL/OpenGL checks="<<n<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';if(gl)SDL_GL_DeleteContext(gl);if(w)SDL_DestroyWindow(w);SDL_Quit();return 1;}}
