// Own pixels only. Requires a real SDL/OpenGL context; run explicitly, not a skip.
#include "runtime/mod_frontend.hpp"
#include "runtime/original_presentation.hpp"
#include <SDL_opengl.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){SDL_SetMainReady();SDL_Window* w=nullptr;SDL_GLContext gl=nullptr;try{
 check(argc==3,"new private storage and authored HUD ZIP required");
#ifdef __APPLE__
 SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,"0");
#endif
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 w=SDL_CreateWindow("NES Native Lab - Phase5 own UI and visual integration probe",100,100,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);check(w,SDL_GetError());gl=SDL_GL_CreateContext(w);check(gl,SDL_GetError());SDL_GL_SetSwapInterval(0);unsigned n=0;
 {
 nes::runtime::GLPresenter presenter(w);nes::mods::Frontend f(false,argv[1],{"0.9.0","addams-family-pal-eu-enfrde"});
 auto key=[&](SDL_Keycode k){f.begin_events();SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;f.handle(e,960,720,960,720);e.type=SDL_KEYUP;f.handle(e,960,720,960,720);};
 f.show();
 for(auto name:{"Super-Gomez-Gameplay-v1.0.zip","Speedrun-HUD-v1.0.zip","Mansion-Parallax-Demo-v1.0.zip"}){SDL_Event e{};e.type=SDL_DROPFILE;auto path=std::filesystem::path(argv[2])/name;e.drop.file=SDL_strdup(path.string().c_str());f.handle(e,960,720,960,720);}
 check(!f.active(),"ZIP automatically ran");++n;key(SDLK_RIGHT);
 for(unsigned i=0;i<3;++i){key(SDLK_RETURN);if(i<2)key(SDLK_DOWN);}
 check(f.active()&&f.jump_extension()==12,"actual Frontend Enable/lease failed");key(SDLK_ESCAPE);check(!f.visible(),"Back did not return to game");++n;
 nes::mods::Observation o;o.frame=5;o.simulation_seconds=5;o.real_seconds=8;o.game=nes::mods::GameSnapshot{"own-fixture","playing",0,nes::mods::PlayerSnapshot{76,160}};f.complete(o);
 std::array<uint8_t,256*240> image{};image.fill(0x20);const auto saved=image;
 auto draw=[&](bool enabled,bool full){SDL_PumpEvents();SDL_Delay(100);int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);presenter.present(image.data(),256,240);GLint before[4],after[4];glGetIntegerv(GL_VIEWPORT,before);f.draw_hud(presenter,dw,dh,full);glFinish();glGetIntegerv(GL_VIEWPORT,after);for(int i=0;i<4;++i)check(before[i]==after[i],"HUD changed viewport");
  auto r=nes::mods::visual_rect(dw,dh,full,0);std::array<uint8_t,4> pixel{};glReadPixels(r.x+r.width/32,dh-r.y-r.height/24,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
  check(enabled?pixel[0]<240:pixel[0]>240,"HUD missing or remains after disable");check(image==saved,"HUD wrote NES pixels");++n;};
 draw(true,false);
 auto raster=[&]{int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);presenter.present(image.data(),256,240);f.draw_hud(presenter,dw,dh,false);glFinish();std::vector<uint8_t> pixels(size_t(dw)*dh*4);glReadPixels(0,0,dw,dh,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());return pixels;};
 auto first=raster();auto uploads=presenter.mod_plane_uploads();check(first==raster()&&presenter.mod_plane_uploads()==uploads,"repeated snapshot draw changed layers");++n;
 auto layer_region=[&](const std::vector<uint8_t>& pixels){int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);auto r=nes::mods::visual_rect(dw,dh,false,0);std::vector<uint8_t> region;for(int y=160;y<230;++y)for(int x=200;x<320;++x){auto offset=size_t(dh-r.y-1-y*r.height/240)*dw*4+(r.x+x*r.width/320)*4;region.insert(region.end(),pixels.begin()+offset,pixels.begin()+offset+4);}return region;};
 o.simulation_seconds=9;f.complete(o);check(layer_region(first)!=layer_region(raster())&&presenter.mod_plane_uploads()==uploads+1,"synthetic camera failed to move own PNG");++n;
 {nes::runtime::GLPresenter fresh(w);fresh.present(image.data(),256,240);f.draw_hud(fresh,960,720,false);glFinish();check(glGetError()==GL_NO_ERROR&&fresh.mod_plane_uploads()==1,"fresh renderer failed to recover resources");++n;}
 SDL_SetWindowSize(w,1280,720);draw(true,false);check(SDL_SetWindowFullscreen(w,SDL_WINDOW_FULLSCREEN_DESKTOP)==0,SDL_GetError());draw(true,true);check(SDL_SetWindowFullscreen(w,0)==0,SDL_GetError());draw(true,false);
 // Pause repeats this same completed capture, no complete/update callbacks.
 for(unsigned i=0;i<5;++i)draw(true,false);
 double cached_us=0,upload_us=0;
 for(unsigned i=0;i<100;++i){int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);auto start=std::chrono::steady_clock::now();f.draw_hud(presenter,dw,dh,false);glFinish();cached_us+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();f.complete(o);start=std::chrono::steady_clock::now();f.draw_hud(presenter,dw,dh,false);glFinish();upload_us+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();}
 std::cout<<"GL host+glFinish repeated_plane_us="<<cached_us/100<<" upload_and_draw_us="<<upload_us/100<<" iterations=100; not display FPS\n";
 key(SDLK_F2);for(unsigned i=0;i<3;++i){key(SDLK_RETURN);if(i<2)key(SDLK_UP);}check(!f.active(),"actual frontend Disable failed");key(SDLK_ESCAPE);draw(false,false);
 nes::mods::Frontend safe(true,std::string(argv[1])+"-safe");safe.complete(o);check(!safe.active(),"safe mode executes v2");safe.draw_hud(presenter,960,720,false);++n;
 std::cout<<"Phase5 real SDL/OpenGL cases="<<n<<" PASS; "<<presenter.version()<<'\n';
 }
 SDL_GL_DeleteContext(gl);SDL_DestroyWindow(w);SDL_Quit();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';if(gl)SDL_GL_DeleteContext(gl);if(w)SDL_DestroyWindow(w);SDL_Quit();return 1;}}
