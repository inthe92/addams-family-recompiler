// Display-dependent integration probe. Run explicitly on a graphical desktop;
// the mandatory portable CTest contracts do not require an OpenGL display.
#include "runtime/gl_presenter.hpp"
#include "runtime/original_presentation.hpp"
#include "runtime/frontend_timing.hpp"
#include "runtime/presentation_clock.hpp"
#include "runtime/palette.hpp"
#include "runtime/bus.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <array>
#include <iostream>
#include <iomanip>
#include <algorithm>
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
        context=SDL_GL_CreateContext(window);if(!context)throw std::runtime_error(std::string("OpenGL context: ")+SDL_GetError());
        const int swap_result=SDL_GL_SetSwapInterval(0);SDL_DisplayMode display{};
        const int display_result=SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window),&display);
        std::cout<<"Display diagnostic driver="<<SDL_GetCurrentVideoDriver()<<" mode_result="<<display_result<<" reported_hz="<<display.refresh_rate
            <<" swap_request_result="<<swap_result<<" swap_interval="<<SDL_GL_GetSwapInterval()<<'\n';
        unsigned cases=0;std::cout<<std::setprecision(12);
        {
            nes::frontend::FrontendTiming timing;
            timing.start(SDL_GetPerformanceFrequency(),[](void*) noexcept {return static_cast<uint64_t>(SDL_GetPerformanceCounter());},nullptr,true);
            GLPresenter presenter(window,&timing);
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
            nes::ui::FrontendUI ui;ui.start(0);
            int w=0,h=0;SDL_GL_GetDrawableSize(window,&w,&h);auto r=nes::ui::overlay_rect(w,h,false);
            GLint before[4];glGetIntegerv(GL_VIEWPORT,before);presenter.present_overlay(ui,1);glFinish();
            GLint after[4];glGetIntegerv(GL_VIEWPORT,after);for(int i=0;i<4;++i)require(before[i]==after[i],"overlay changed NES viewport");
            require(bus.video().frame()==source&&bus.cycles()==cycles&&bus.audio().cycles()==apu,"overlay changed framebuffer or clock");++cases;
            ui.handle(nes::ui::Action::request_exit);presenter.present(bus.video());presenter.present_overlay(ui,2);glFinish();
            const auto modal=nes::ui::overlay_rect(w,h,true);std::array<uint8_t,4> shade{};
            glReadPixels(modal.x+modal.width/2,h-modal.y-modal.height/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,shade.data());
            require(shade[0]<100&&shade[1]<100&&shade[2]<100,"dialog background not drawn");++cases;
            using nes::frontend::TimingCount;using nes::frontend::TimingStage;
            const auto draws=timing.count(TimingCount::nes_draws),overlays=timing.count(TimingCount::ui_draws);
            for(unsigned i=0;i<4;++i){
                {nes::frontend::FrontendTiming::Scope measure(timing,TimingStage::render);presenter.present(bus.video());presenter.present_overlay(ui,2);timing.record(TimingCount::render_passes);}
                {nes::frontend::FrontendTiming::Scope measure(timing,TimingStage::swap);SDL_GL_SwapWindow(window);timing.record(TimingCount::swaps);}
            }
            require(timing.count(TimingCount::nes_draws)==draws+4&&timing.count(TimingCount::ui_draws)==overlays+4&&timing.count(TimingCount::swaps)==4,"actual GL calls vs swaps counted incorrectly");
            require(timing.count(TimingCount::nes_frames)==0&&timing.count(TimingCount::audio_batches)==0&&timing.count(TimingCount::god_hooks)==0&&bus.cycles()==cycles&&bus.video().frame()==source&&bus.audio().cycles()==apu,"instrumented GL rendering invoked or changed NES/audio/hooks");++cases;
            // Cache correctness is judged by actual uploads and readback, not
            // only a cache-key test. All pixels are authored synthetic data.
            nes::frontend::FrameSnapshot cached,other,empty;
            nes::frontend::FrameSnapshot::Pixels pixels{};pixels.fill(0x16);cached.complete(pixels,1,1,0,0);
            auto uploaded=timing.count(TimingCount::nes_uploads),converted=timing.count(TimingCount::nes_conversions);
            const auto cached_hits=timing.count(TimingCount::cached_draws);
            const auto check_color=[&](uint8_t index){
                glFinish();GLint v[4];glGetIntegerv(GL_VIEWPORT,v);std::array<uint8_t,4> actual{};
                glReadPixels(v[0]+v[2]/2,v[1]+v[3]/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());auto rgb=rgb_palette[index];
                require(actual[0]==((rgb>>16)&255)&&actual[1]==((rgb>>8)&255)&&actual[2]==(rgb&255),"cached texture readback stale");
            };
            for(unsigned n=0;n<10;++n)presenter.present(cached);check_color(0x16);
            require(timing.count(TimingCount::nes_uploads)==uploaded+1&&timing.count(TimingCount::nes_conversions)==converted+1&&timing.count(TimingCount::cached_draws)==cached_hits+9,"repeat snapshot cache miss or missing draw");++cases;
            pixels.fill(0x2a);cached.complete(pixels,1,1,0,0);presenter.present(cached);check_color(0x2a);
            require(timing.count(TimingCount::nes_uploads)==uploaded+2,"same metadata republished but not uploaded");++cases;
            pixels.fill(0x12);other.complete(pixels,1,1,0,0);presenter.present(other);check_color(0x12);
            require(timing.count(TimingCount::nes_uploads)==uploaded+3,"second stream sequence collision");++cases;
            presenter.present(image.data(),256,240);uploaded=timing.count(TimingCount::nes_uploads);presenter.present(other);check_color(0x12);
            require(timing.count(TimingCount::nes_uploads)==uploaded+1,"raw path did not invalidate snapshot cache");++cases;
            uploaded=timing.count(TimingCount::nes_uploads);SDL_SetWindowSize(window,800,600);SDL_PumpEvents();SDL_Delay(100);
            presenter.present(other);check_color(0x12);
            if(SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)throw std::runtime_error(SDL_GetError());SDL_PumpEvents();SDL_Delay(100);
            presenter.present(other);check_color(0x12);
            if(SDL_SetWindowFullscreen(window,0)!=0)throw std::runtime_error(SDL_GetError());SDL_PumpEvents();SDL_Delay(100);
            presenter.present(other);check_color(0x12);require(timing.count(TimingCount::nes_uploads)==uploaded,"resize/fullscreen needlessly reuploaded or invalidated texture");++cases;
            presenter.present_overlay(ui,2);presenter.present(other);check_color(0x12);
            require(timing.count(TimingCount::nes_uploads)==uploaded,"overlay corrupts retained NES texture");++cases;
            {GLPresenter recreated(window,&timing);recreated.present(other);check_color(0x12);}
            require(timing.count(TimingCount::nes_uploads)==uploaded+1,"new presenter/context resources silently accepted old texture state");
            presenter.present(empty);check_color(0);presenter.present(other);check_color(0x12);++cases;
            require(bus.cycles()==cycles&&bus.audio().cycles()==apu&&bus.video().frame()==source,"cache tests changed NES state");
            SDL_SetWindowSize(window,1280,720);SDL_PumpEvents();SDL_Delay(100);
            if(argc==2 && std::string(argv[1])!="--benchmark")presenter.capture(argv[1]);
            if(argc==2 && std::string(argv[1])=="--benchmark") {
                nes::frontend::FrameSnapshot stable;stable.complete(image,100,100,0,0);
                for(bool retained:{false,true}){
                    std::array<double,5> samples{};
                    const auto draw=[&]{if(retained)presenter.present(stable);else presenter.present(image.data(),256,240);};
                    for(unsigned n=0;n<32;++n)draw();glFinish();
                    for(auto& sample:samples){const auto begin=SDL_GetPerformanceCounter();
                        for(unsigned n=0;n<200;++n)draw();
                        glFinish();sample=1000.0*(SDL_GetPerformanceCounter()-begin)/SDL_GetPerformanceFrequency();}
                    std::sort(samples.begin(),samples.end());
                    std::cout<<"Repeat benchmark path="<<(retained?"cached":"raw")<<" batches=5 draws_per_batch=200 median_batch_ms="<<samples[2]<<'\n';
                }
            }
            // Real SDL/GL host pacing with own pixels; no NES simulation here.
            // Report swaps/draws, not physical display scanout or new images.
            for(unsigned hz:{60u,90u,120u}) {
                const auto frequency=SDL_GetPerformanceFrequency(),begin=SDL_GetPerformanceCounter();
                nes::frontend::PresentationClock clock(frequency,hz,begin);
                nes::frontend::FrameSnapshot frozen;
                nes::frontend::FrameSnapshot::Pixels live{};
                const auto initial_draws=timing.count(TimingCount::nes_draws);
                const auto conversions=timing.count(TimingCount::nes_conversions),uploads=timing.count(TimingCount::nes_uploads);
                const auto conversion_ticks=timing.stats(TimingStage::conversion).ticks,upload_ticks=timing.stats(TimingStage::upload).ticks;
                const auto render_start=timing.stats(TimingStage::render).ticks,swap_start=timing.stats(TimingStage::swap).ticks,wait_start=timing.stats(TimingStage::wait).ticks;
                uint64_t produced=0,presentations=0;
                while(SDL_GetPerformanceCounter()-begin<frequency*2) {
                    SDL_PumpEvents();auto now=SDL_GetPerformanceCounter();const auto due=clock.poll(now);
                    if(due.simulate) {
                        live.fill(static_cast<uint8_t>(colors[produced%4]));++produced;
                        frozen.complete(live,produced,produced,0,now);live.fill(0);
                        clock.simulation_complete(SDL_GetPerformanceCounter());
                    }
                    if(due.present && frozen.valid()) {
                        {nes::frontend::FrontendTiming::Scope measure(timing,TimingStage::render);presenter.present(frozen);timing.record(TimingCount::render_passes);}
                        {nes::frontend::FrontendTiming::Scope measure(timing,TimingStage::swap);SDL_GL_SwapWindow(window);}++presentations;
                        timing.record(TimingCount::swaps);
                        clock.presentation_complete(SDL_GetPerformanceCounter());
                    }
                    if(static_cast<double>(SDL_GetPerformanceCounter())<clock.next_deadline()){nes::frontend::FrontendTiming::Scope measure(timing,TimingStage::wait);SDL_Delay(1);}
                }
                const double elapsed=double(SDL_GetPerformanceCounter()-begin)/frequency;
                require(presentations>0&&produced>0&&timing.count(TimingCount::nes_draws)-initial_draws==presentations,"scheduled GL draws/swaps not observed");
                require(timing.count(TimingCount::nes_uploads)-uploads<=produced && timing.count(TimingCount::nes_conversions)-conversions==timing.count(TimingCount::nes_uploads)-uploads,"retained rate path reconverted/reuploaded unchanged snapshots");
                require(bus.cycles()==cycles&&bus.video().frames()==frames&&bus.audio().cycles()==apu&&bus.video().frame()==source,"scheduled GL touches NES runtime");
                std::cout<<"GL stage target="<<hz<<" conversions="<<timing.count(TimingCount::nes_conversions)-conversions
                    <<" uploads="<<timing.count(TimingCount::nes_uploads)-uploads
                    <<" conversion_ms="<<1000.0*(timing.stats(TimingStage::conversion).ticks-conversion_ticks)/frequency
                    <<" upload_host_ms="<<1000.0*(timing.stats(TimingStage::upload).ticks-upload_ticks)/frequency
                    <<" render_host_ms="<<1000.0*(timing.stats(TimingStage::render).ticks-render_start)/frequency
                    <<" swap_ms="<<1000.0*(timing.stats(TimingStage::swap).ticks-swap_start)/frequency
                    <<" wait_ms="<<1000.0*(timing.stats(TimingStage::wait).ticks-wait_start)/frequency<<'\n';
                presenter.present(frozen);glFinish();
                GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);std::array<uint8_t,4> pixel{};
                glReadPixels(viewport[0]+viewport[2]/2,viewport[1]+viewport[3]/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
                const auto expected=rgb_palette[frozen.pixels()[0]];
                require(pixel[0]==((expected>>16)&255)&&pixel[1]==((expected>>8)&255)&&pixel[2]==(expected&255),"snapshot readback differs from completed own image");
                std::cout<<"Scheduled GL target="<<hz<<" draws="<<presentations<<" elapsed="<<elapsed<<" host_draws_per_second="<<presentations/elapsed<<" own_snapshot_updates="<<produced<<'\n';++cases;
            }
            std::cout<<cases<<" real SDL/OpenGL presentation cases: PASS; OpenGL "<<presenter.version()<<'\n';
            timing.report(std::cout,"synthetic_probe",0);
        }
        SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';if(context)SDL_GL_DeleteContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();return 1;}
}
