#include "runtime/mod_frontend.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{check(argc==7,"synthetic store/catalog/two ZIPs required");SDL_SetMainReady();SDL_setenv("SDL_VIDEODRIVER","dummy",1);check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());unsigned n=0;
 std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};prg[0x800]=65;prg[0x801]=66;prg[0x802]=67;nes::runtime::Cartridge c({prg.data(),prg.size(),chr.data(),chr.size()});
 nes::mods::Frontend f(false,argv[1],{"0.9.0","own-fixture",std::string(64,'a'),argv[4]});
 auto key=[&](SDL_Keycode k){f.begin_events();SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;f.handle(e,960,720,960,720);e.type=SDL_KEYUP;f.handle(e,960,720,960,720);};
 auto drop=[&](const char* path){SDL_Event e{};e.type=SDL_DROPFILE;e.drop.file=SDL_strdup(path);f.handle(e,960,720,960,720);};
 f.show();drop(argv[5]);check(!f.active(),"install automatically enables resource");++n;key(SDLK_RIGHT);key(SDLK_RETURN);check(f.active(),"frontend Enable missing");++n;check(c.cpu_read(0x8800,0)==65,"UI event changes in-progress simulation");++n;f.apply_resources(c);check(c.cpu_read(0x8800,0)==67,"frontend backend not reached");++n;key(SDLK_ESCAPE);check(!f.visible(),"Back blocked");++n;
 auto revision=c.text_resources().data();for(unsigned i=0;i<120;++i){f.apply_resources(c);f.complete({});}check(c.text_resources().data()==revision,"presentation updates republish resource");++n;
 key(SDLK_F2);key(SDLK_RETURN);f.apply_resources(c);check(!f.active()&&c.cpu_read(0x8800,0)==65,"disable fallback/lifetime");++n;drop(argv[6]);key(SDLK_DOWN);key(SDLK_RETURN);f.apply_resources(c);check(c.cpu_read(0x8800,0)==66,"second package needs different C++");++n;
 nes::mods::Frontend safe(true,argv[1]);nes::runtime::Cartridge original({prg.data(),prg.size(),chr.data(),chr.size()});safe.apply_resources(original);safe.complete({});check(!safe.active()&&f.active()&&original.cpu_read(0x8800,0)==65,"safe mode activates translations or changes enabled preference");++n;
 std::cout<<"Resource frontend checks="<<n<<" PASS\n";SDL_Quit();return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
