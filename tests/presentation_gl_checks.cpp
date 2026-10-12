// Display-dependent integration probe. Run explicitly on a graphical desktop;
// the mandatory portable CTest contracts do not require an OpenGL display.
#include "runtime/gl_presenter.hpp"
#include "runtime/original_presentation.hpp"
#include "runtime/palette.hpp"
#include "runtime/bus.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <utility>
using namespace nes::runtime;
static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv) {
    SDL_SetMainReady();
#ifdef __APPLE__
    // Desktop-sized borderless fullscreen avoids Cocoa Space transitions.
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,"0");
#endif
    SDL_Window* window=nullptr;SDL_GLContext context=nullptr;
    try {
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(std::string("SDL video initialization: ")+SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
        window=SDL_CreateWindow("NES Native Lab — synthetic presentation verification",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
        if(!window)throw std::runtime_error(std::string("SDL window: ")+SDL_GetError());
        context=SDL_GL_CreateContext(window);if(!context)throw std::runtime_error(std::string("OpenGL context: ")+SDL_GetError());SDL_GL_SetSwapInterval(0);
        unsigned cases=0;
        {
            GLPresenter presenter(window);
            std::array<uint8_t,256*240> image{};
            const unsigned colors[]={0x16,0x2A,0x12,0x20};
            for(unsigned y=0;y<240;++y)for(unsigned x=0;x<256;++x)image[y*256+x]=static_cast<uint8_t>(colors[(y>=120?2:0)+(x>=128?1:0)]);
            const auto saved=image;
            const auto verify=[&](bool full) {
                SDL_PumpEvents();SDL_Delay(100);SDL_PumpEvents();
                presenter.present(image.data(),256,240);glFinish();
                int w=0,h=0;SDL_GL_GetDrawableSize(window,&w,&h);const auto v=original_viewport(w,h,full);
                GLint actual[4]{};glGetIntegerv(GL_VIEWPORT,actual);
                require(actual[0]==v.x&&actual[1]==v.y&&actual[2]==v.width&&actual[3]==v.height,"OpenGL viewport mismatch");
                const auto pixel=[&](int x,int y,uint32_t expected) {
                    std::array<uint8_t,4> value{};glReadPixels(x,y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,value.data());
                    require(value[0]==((expected>>16)&255)&&value[1]==((expected>>8)&255)&&value[2]==(expected&255),"rendered pixel/letterbox/orientation mismatch");
                };
                for(int qy=0;qy<2;++qy)for(int qx=0;qx<2;++qx)
                    pixel(v.x+v.width*(qx*2+1)/4,v.y+v.height*(qy*2+1)/4,rgb_palette[colors[(1-qy)*2+qx]]);
                if(v.x>0){pixel(0,h/2,0);pixel(w-1,h/2,0);}
                if(v.y>0){pixel(w/2,0,0);pixel(w/2,h-1,0);}
                require(image==saved,"source modified by GL");++cases;
            };
            verify(false);
            for(const auto size:{std::pair<int,int>{1280,720},{800,600},{600,900},{319,239}}){SDL_SetWindowSize(window,size.first,size.second);verify(false);}
            if(SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)throw std::runtime_error(std::string("Enter fullscreen: ")+SDL_GetError());verify(true);
            if(SDL_SetWindowFullscreen(window,0)!=0)throw std::runtime_error(std::string("Leave fullscreen: ")+SDL_GetError());verify(false);
            bool rejected=false;try{presenter.present(image.data(),427,240);}catch(const std::invalid_argument&){rejected=true;}
            require(rejected,"GL accepted an extended frame");++cases;
            std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};
            Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});bus.advance(100);
            const auto cycles=bus.cycles(),frames=bus.video().frames(),apu=bus.audio().cycles();
            const auto source=bus.video().frame();
            for(unsigned refresh:{60u,120u})for(unsigned i=0;i<refresh;++i)presenter.present(bus.video());
            require(bus.cycles()==cycles&&bus.video().frames()==frames&&bus.audio().cycles()==apu&&bus.video().frame()==source,"presentation advanced CPU/PPU/APU or mutated frame");++cases;
            SDL_SetWindowSize(window,1280,720);verify(false);
            if(argc==2)presenter.capture(argv[1]);
            std::cout<<cases<<" real SDL/OpenGL presentation cases: PASS; OpenGL "<<presenter.version()<<'\n';
        }
        SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';if(context)SDL_GL_DeleteContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();return 1;}
}
