#include "utf8_test_args.hpp"
#include "runtime/mod_frontend.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
static unsigned n{};static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);++n;}
int main(int argc,char**argv){try{nes::test::Utf8Args args(argc,argv);argc=args.size();argv=args.data();if(argc!=7)throw std::invalid_argument("store/helper/catalog/two fixtures required");SDL_SetMainReady();SDL_setenv("SDL_VIDEODRIVER","dummy",1);if(SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());
 std::array<uint8_t,32768>p{};std::array<uint8_t,8192>c{};p[2048]=65;p[2049]=66;p[2050]=67;nes::runtime::Cartridge cartridge({p.data(),p.size(),c.data(),c.size()});
 nes::mods::Frontend f(false,argv[1],{"0.9.0","own-fixture",std::string(64,'a'),argv[4]});
 auto key=[&](SDL_Keycode k){f.begin_events();SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;f.handle(e,960,720,960,720);e.type=SDL_KEYUP;f.handle(e,960,720,960,720);};
 auto drop=[&](const char*path){SDL_Event e{};e.type=SDL_DROPFILE;e.drop.file=SDL_strdup(path);f.handle(e,960,720,960,720);};
 f.show();drop(argv[5]);check(!f.active(),"installed visual resource auto-enabled");key(SDLK_RIGHT);key(SDLK_RETURN);check(f.active(),"UI Enable not reached");check(!cartridge.has_compositions(),"UI changed in-progress simulation");
 f.apply_resources(cartridge);check(cartridge.has_compositions()&&cartridge.cpu_read(0x8800)==65,"UI visual chain alters PRG or fails to publish");
 auto revision=cartridge.text_resource_revision();for(unsigned i=0;i<120;++i){f.apply_resources(cartridge);f.complete({});}check(cartridge.text_resource_revision()==revision,"repeated presentation republishes glyph");
 key(SDLK_RETURN);f.apply_resources(cartridge);check(!cartridge.has_compositions(),"UI Disable retains resource");
 key(SDLK_ESCAPE);check(!f.visible(),"Back does not return");key(SDLK_F2);drop(argv[6]);key(SDLK_DOWN);key(SDLK_RETURN);f.apply_resources(cartridge);check(cartridge.has_compositions(),"second ZIP requires different host");
 nes::mods::Frontend safe(true,argv[1]);nes::runtime::Cartridge untouched({p.data(),p.size(),c.data(),c.size()});safe.apply_resources(untouched);safe.complete({});check(!safe.active()&&!untouched.has_compositions()&&f.active(),"safe mode activates visual resource or alters preference");
 std::cout<<"Composition frontend checks="<<n<<" PASS\n";SDL_Quit();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
