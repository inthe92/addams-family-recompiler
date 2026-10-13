#include "game/compiled_program.h"
#include "game/recovered_resources.h"
#include "game/god_mode.hpp"
#include "game/gameplay_diagnostic.hpp"
#include "runtime/bus.hpp"
#include "runtime/gl_presenter.hpp"
#include "runtime/original_presentation.hpp"
#include "runtime/frontend_events.hpp"
#include "runtime/frontend_timing.hpp"
#include "runtime/presentation_clock.hpp"
#include "runtime/ui_text.hpp"
#include "runtime/sdl_audio_output.hpp"
#include "runtime/wav_writer.hpp"
#include "runtime/application_icon.hpp"
#include "native_build_metadata.hpp"
#ifdef NES_USER_ROM
#include "runtime/user_rom.hpp"
#endif
#ifdef NES_ENABLE_MODS
#include "runtime/mod_engine.hpp"
#include "runtime/mod_frontend.hpp"
#include "game/mod_observation.hpp"
#endif
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

struct SDLQuit { ~SDLQuit(){SDL_Quit();} };
struct GLContext {SDL_GLContext value{};~GLContext(){if(value)SDL_GL_DeleteContext(value);} };
struct Controller {SDL_GameController* value{};~Controller(){if(value)SDL_GameControllerClose(value);} };
static void show_error(const std::string& message) {
    std::cerr << message << '\n';
#ifdef NES_BETA_BUILD
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, nes::ui::text::name,
        (std::string(nes::ui::text::load_error)+"\n\n"+message+"\n\n"+nes::ui::text::experimental).c_str(), nullptr);
#endif
}
using nes::frontend::FrontendTiming;
using nes::frontend::TimingCount;
using nes::frontend::TimingStage;
int main(int argc,char** argv) {
 FrontendTiming timing;uint64_t nes_frames=0;
 std::ofstream diagnostic_output;
 nes::game::GameplayDiagnostic diagnostic;
 try {
    if(argc==2 && std::string(argv[1])=="--build-info") {
        std::cout<<nes::build::identity<<'\n';return 0;
    }
    SDL_SetMainReady();
#ifdef __APPLE__
    // Desktop-sized borderless fullscreen avoids Cocoa Space transitions.
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,"0");
#endif
    unsigned experimental_refresh=0;bool no_mods=false,mods_only=false;std::string mods_directory;
    uint64_t limit=0;bool fast=false,mute=false,fullscreen=false;std::string mod_path,capture,audio_path;
    bool god_enabled=false,diagnostic_requested=false,timing_requested=false;
    for(int i=1;i<argc;++i){const std::string option=argv[i];if(option=="--unthrottled"){fast=true;continue;}
#ifdef NES_USER_ROM
        if(option=="--import-only")continue;
        if(option=="--rom"||option=="--import-cache"||option=="--import-tools"){++i;continue;}
        if(option.rfind("-psn_",0)==0)continue;
#endif
        if(option=="--no-mods"){no_mods=true;continue;}
        if(option=="--mods-only"){mods_only=true;continue;}
        if(option=="--fullscreen"){fullscreen=true;continue;}
        if(option=="--mute"){mute=true;continue;}
        if(option=="--diagnostic"){diagnostic_requested=true;continue;}
        if(option=="--timing-diagnostic"){timing_requested=true;continue;}
        if(option=="--god"){god_enabled=true;continue;}
        if(option=="--no-god"){god_enabled=false;continue;}
        if(i+1>=argc)throw std::invalid_argument("Option requires a value");
        if(option=="--mods-dir")mods_directory=argv[++i];
        else if(option=="--experimental-present-hz")experimental_refresh=nes::frontend::parse_experimental_refresh(argv[++i]);
        else if(option=="--frames")limit=std::stoull(argv[++i]);else if(option=="--mod")mod_path=argv[++i];
        else if(option=="--capture")capture=argv[++i];else if(option=="--audio-output")audio_path=argv[++i];else throw std::invalid_argument("Unknown desktop option");}
    if(mods_only){
#ifdef NES_ENABLE_MODS
        return nes::mods::manager_window(no_mods,mods_directory);
#else
        throw std::runtime_error("Mod Manager disabled in this build");
#endif
    }
    if(no_mods)mod_path.clear();
    if(diagnostic_requested) {
        try {
            const auto preferences=std::unique_ptr<char,decltype(&SDL_free)>(SDL_GetPrefPath("nes-native-lab","diagnostics"),SDL_free);
            if(!preferences)throw std::runtime_error(SDL_GetError());
            const auto path=nes::game::GameplayDiagnostic::local_log_path(std::filesystem::u8path(preferences.get()));
            diagnostic_output.open(path,std::ios::out|std::ios::app);
            if(!diagnostic_output)throw std::runtime_error("Cannot open local diagnostic file");
            diagnostic.begin(diagnostic_output,god_enabled,false,!mod_path.empty());
            if(diagnostic.failed())throw std::runtime_error("Cannot write local diagnostic session header");
            std::cout<<"Gameplay diagnostic: "<<path.u8string()<<'\n';
        } catch(const std::exception& e) {std::cerr<<"Gameplay diagnostic unavailable: "<<e.what()<<'\n';}
    }
#ifdef NES_USER_ROM
    nes::runtime::UserRomModule imported;
    if(!imported.prepare(argc,argv)){diagnostic.end("import_not_started");return 0;}
#endif
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0)throw std::runtime_error(SDL_GetError());SDLQuit quit;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    const std::string title=std::string(nes::ui::text::name)+" "+nes::app::version;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow(title.c_str(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
    if(!window)throw std::runtime_error(SDL_GetError());
    nes::runtime::set_application_icon(window.get());
    GLContext context{SDL_GL_CreateContext(window.get())};
    if(!context.value)throw std::runtime_error(SDL_GetError());
    // PAL pacing below owns the clock. A Cocoa display-link wait can stop
    // receiving callbacks for a background/offscreen window and block input.
    const int swap_request_result=SDL_GL_SetSwapInterval(0);
    if(timing_requested) {
        SDL_DisplayMode display{};const int mode_result=SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window.get()),&display);
        std::cout<<"Presentation display: reported_hz="<<display.refresh_rate<<" mode_result="<<mode_result
            <<" swap_request_result="<<swap_request_result<<" swap_interval="<<SDL_GL_GetSwapInterval()<<'\n';
    }
    nes::runtime::GLPresenter presenter(window.get(),timing_requested?&timing:nullptr);
    std::cout<<"OpenGL "<<presenter.version()<<"; statically recompiled C game; PAL runtime\n";
    nes::runtime::Bus bus({&game_prg_data[0][0],131072u,game_chr_data,131072u});
    CPUState cpu=cpu_initial_state(game_reset_vector);RuntimeServices services=bus.services();
    nes::game::GodMode god;services=god.services(bus,services);god.set_enabled(god_enabled);
    services=diagnostic.services(bus,services,cpu);
    const bool debug_controls=god_enabled||diagnostic_requested;
    const auto update_title=[&]{
        const std::string caption=title+(god.enabled()?" - GOD enabled":"")+(god.all_items()?" - All items enabled":"");
        SDL_SetWindowTitle(window.get(),caption.c_str());
    };
    update_title();
    if(fullscreen && SDL_SetWindowFullscreen(window.get(),SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)throw std::runtime_error(SDL_GetError());
    nes::runtime::SDLAudioOutput audio_output(!mute,fast);
    std::unique_ptr<nes::runtime::WavWriter> wav;
    if(!audio_path.empty())wav=std::make_unique<nes::runtime::WavWriter>(audio_path,nes::runtime::AudioState::sample_rate);
#ifdef NES_ENABLE_MODS
    nes::runtime::ModEngine mods(bus);if(!no_mods&&!mod_path.empty())mods.load_file(mod_path);
    nes::mods::Frontend managed_mods(no_mods,mods_directory);
#else
    if(!mod_path.empty())throw std::runtime_error("Lua support is disabled in this build");
#endif
    Controller controller;for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){controller.value=SDL_GameControllerOpen(i);break;}
    bool running=true;const Uint64 frequency=SDL_GetPerformanceFrequency();
    double next_deadline=static_cast<double>(SDL_GetPerformanceCounter());
    bool frontend_paused=false;
    // --unthrottled retains the original one-NES-unit/one-draw fast path.
    std::optional<nes::frontend::PresentationClock> presentation_clock;
    nes::frontend::FrameSnapshot snapshot;
    if(experimental_refresh && !fast)presentation_clock.emplace(frequency,experimental_refresh,static_cast<uint64_t>(next_deadline));
    if(experimental_refresh)std::cout<<"Experimental presentation: "<<experimental_refresh<<" Hz; repeat complete PAL frames; "
        <<(fast?"bypassed by --unthrottled":"independent host deadlines")<<'\n';
    if(timing_requested)timing.start(frequency,[](void*) noexcept {return static_cast<uint64_t>(SDL_GetPerformanceCounter());},nullptr,fast);
    nes::ui::FrontendUI ui;nes::ui::FrontendEvents ui_events(ui);
    const auto seconds=[&]{return static_cast<double>(SDL_GetPerformanceCounter())/frequency;};

    while(running&&(!limit||nes_frames<limit)) {
        ui.begin_events();
#ifdef NES_ENABLE_MODS
        managed_mods.begin_events();
#endif
        int ww=0,wh=0,dw=0,dh=0;SDL_GetWindowSize(window.get(),&ww,&wh);SDL_GL_GetDrawableSize(window.get(),&dw,&dh);
        SDL_Event event;while(SDL_PollEvent(&event)) {
            SDL_GetWindowSize(window.get(),&ww,&wh);SDL_GL_GetDrawableSize(window.get(),&dw,&dh);
            const auto modal_keys=nes::ui::modal_pressed_count(SDL_GetKeyboardState(nullptr),controller.value);
#ifdef NES_ENABLE_MODS
            if(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_F2&&!event.key.repeat&&!ui.modal())audio_output.set_paused(true);
            if(event.type==SDL_CONTROLLERBUTTONDOWN&&event.cbutton.button==SDL_CONTROLLER_BUTTON_RIGHTSTICK&&!ui.modal())audio_output.set_paused(true);
            if(!ui.modal()&&managed_mods.handle(event,ww,wh,dw,dh)){ui.external_pause(managed_mods.visible());continue;}
            ui.external_pause(managed_mods.visible());
#endif
            ui_events.handle(event,ww,wh,dw,dh,modal_keys>1||(event.type==SDL_MOUSEBUTTONUP&&modal_keys>0));
            if(debug_controls&&!ui.paused()&&ui.game_input_enabled()&&event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_g){god.set_enabled(!god.enabled());update_title();diagnostic.frame(cpu,god.enabled(),god.all_items(),!mod_path.empty());timing.record(TimingCount::diagnostic_frames);}
            if(debug_controls&&!ui.paused()&&ui.game_input_enabled()&&event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_h){god.grant_all_items();update_title();diagnostic.frame(cpu,god.enabled(),god.all_items(),!mod_path.empty());timing.record(TimingCount::diagnostic_frames);}
            if(event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_F11){
                const Uint32 flag=(SDL_GetWindowFlags(window.get())&SDL_WINDOW_FULLSCREEN)?0:SDL_WINDOW_FULLSCREEN_DESKTOP;
                if(SDL_SetWindowFullscreen(window.get(),flag)!=0)throw std::runtime_error(SDL_GetError());
            }
            if(event.type==SDL_CONTROLLERDEVICEADDED&&!controller.value)controller.value=SDL_GameControllerOpen(event.cdevice.which);
            if(event.type==SDL_CONTROLLERDEVICEREMOVED&&controller.value&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.value))==event.cdevice.which){SDL_GameControllerClose(controller.value);controller.value=nullptr;}
        }
        ui.observe_neutral(nes::ui::modal_controls_idle(SDL_GetKeyboardState(nullptr),controller.value),nes::ui::all_controls_idle(SDL_GetKeyboardState(nullptr),controller.value));
        if(ui.quit_requested())break;
        audio_output.set_paused(ui.paused());
        if(frontend_paused!=ui.paused()) {
            frontend_paused=ui.paused();std::cout<<(frontend_paused?"Gameplay paused":"Gameplay resumed")<<"; CPU cycle="<<bus.cycles()<<"; PPU frame="<<bus.video().frames()<<"; audio_paused="<<audio_output.paused()<<'\n'<<std::flush;
        }
        if(presentation_clock)presentation_clock->set_paused(ui.paused(),SDL_GetPerformanceCounter());
        if(ui.paused()) {
            if(presentation_clock) {
                bus.set_buttons(0,0);
                timing.record(TimingCount::paused_iterations);
                if(presentation_clock->poll(SDL_GetPerformanceCounter()).present) {
                    { FrontendTiming::Scope measure(timing,TimingStage::render);
                      presenter.present(snapshot);presenter.present_overlay(ui,seconds());
#ifdef NES_ENABLE_MODS
                      managed_mods.draw(presenter,dw,dh);
#endif
                      timing.record(TimingCount::render_passes);timing.record(TimingCount::paused_render_passes); }
                    { FrontendTiming::Scope measure(timing,TimingStage::swap);
                      SDL_GL_SwapWindow(window.get());timing.record(TimingCount::swaps); }
                    presentation_clock->presentation_complete(SDL_GetPerformanceCounter());
                }
                { FrontendTiming::Scope measure(timing,TimingStage::wait);SDL_Delay(1); }
                continue;
            }
            bus.set_buttons(0,0);
            timing.record(TimingCount::paused_iterations);
            { FrontendTiming::Scope measure(timing,TimingStage::render);
              presenter.present(bus.video());presenter.present_overlay(ui,seconds());
#ifdef NES_ENABLE_MODS
                      managed_mods.draw(presenter,dw,dh);
#endif
              timing.record(TimingCount::render_passes);timing.record(TimingCount::paused_render_passes); }
            { FrontendTiming::Scope measure(timing,TimingStage::swap);
              SDL_GL_SwapWindow(window.get());timing.record(TimingCount::swaps); }
            { FrontendTiming::Scope measure(timing,TimingStage::wait);
              next_deadline=static_cast<double>(SDL_GetPerformanceCounter());SDL_Delay(10); }continue;
        }
        const bool simulate=!presentation_clock || presentation_clock->poll(SDL_GetPerformanceCounter()).simulate;
        if(simulate) {
        FrontendTiming::Scope simulation_measure(timing,TimingStage::simulation);
        bus.set_buttons(0,ui.game_input_enabled()?nes::ui::game_buttons(SDL_GetKeyboardState(nullptr),controller.value):0);
#ifdef NES_ENABLE_MODS
        if(!no_mods){mods.frame(cpu);timing.record(TimingCount::mod_hooks);}
        managed_mods.update(nes::game::mod_observation(bus));
#endif
        god.begin_frame();timing.record(TimingCount::god_hooks);
        const uint64_t before=bus.video().frames();
        while(bus.video().frames()==before) {
            timing.record(TimingCount::native_slices);
            uint32_t budget=1000;if(!game_execute_slice(&cpu,&services,&budget)){
                diagnostic.missing_address(cpu);diagnostic.end("uncovered_address");simulation_measure.stop();
                timing.report(std::cout,"uncovered_address",nes_frames);
                show_error("Uncovered native routine: bank "+std::to_string(bus.cartridge().bank_for_pc(cpu.PC))+", PC "+std::to_string(cpu.PC));return 2;}
            diagnostic.slice(cpu);timing.record(TimingCount::diagnostic_slices);
        }
        diagnostic.frame(cpu,god.enabled(),god.all_items(),!mod_path.empty());timing.record(TimingCount::diagnostic_frames);
        if(diagnostic.failed()&&diagnostic_requested){std::cerr<<"Gameplay diagnostic stopped: output write failed; session evidence is incomplete\n";diagnostic_requested=false;}
        bus.video().consume_frame();
        timing.record(TimingCount::nes_frames);timing.record(TimingCount::ppu_frames,bus.video().frames()-before);
        simulation_measure.stop();
        FrontendTiming::Scope audio_measure(timing,TimingStage::audio);
        auto samples=bus.audio().take_samples();audio_output.submit(samples);
        timing.record(TimingCount::audio_batches);timing.record(TimingCount::pcm_samples,samples.size());
        if(wav){wav->append(samples);timing.record(TimingCount::wav_batches);}
        audio_measure.stop();
        if(!nes_frames)ui.start(seconds());
        if(presentation_clock) {
            // Publish only after the unchanged NES/hooks/diagnostic/audio unit.
            snapshot.complete(bus.video().frame(),nes_frames+1,bus.video().frames(),bus.cycles(),SDL_GetPerformanceCounter());
            presentation_clock->simulation_complete(SDL_GetPerformanceCounter());
        }
        }
        const bool final_frame=simulate && limit && nes_frames+1==limit;
        const bool draw=!presentation_clock || (snapshot.valid() && (final_frame || presentation_clock->poll(SDL_GetPerformanceCounter()).present));
        if(draw) {
        { FrontendTiming::Scope measure(timing,TimingStage::render);
          if(presentation_clock)presenter.present(snapshot);
          else presenter.present(bus.video());
          presenter.present_overlay(ui,seconds());
#ifdef NES_ENABLE_MODS
                      managed_mods.draw(presenter,dw,dh);
#endif
          timing.record(TimingCount::render_passes); }
        }
        if(simulate)++nes_frames;
        if(limit&&nes_frames==limit&&!capture.empty()){
            FrontendTiming::Scope measure(timing,TimingStage::capture);
            presenter.capture(capture);timing.record(TimingCount::captures);
        }
        if(draw) {
        { FrontendTiming::Scope measure(timing,TimingStage::swap);
          SDL_GL_SwapWindow(window.get());timing.record(TimingCount::swaps); }
        if(presentation_clock)presentation_clock->presentation_complete(SDL_GetPerformanceCounter());
        }
        if(presentation_clock) {
            FrontendTiming::Scope measure(timing,TimingStage::wait);
            if(final_frame) {
                // Preserve the final PAL wait after capture/swap for --frames.
                while(static_cast<double>(SDL_GetPerformanceCounter())<presentation_clock->simulation_deadline())SDL_Delay(1);
            } else if(static_cast<double>(SDL_GetPerformanceCounter())<presentation_clock->next_deadline())SDL_Delay(1);
        } else if(!fast){
            FrontendTiming::Scope measure(timing,TimingStage::wait);
            next_deadline=nes::runtime::next_pal_deadline(next_deadline,static_cast<double>(frequency),static_cast<double>(SDL_GetPerformanceCounter()));
            while(static_cast<double>(SDL_GetPerformanceCounter())<next_deadline)SDL_Delay(1);}
    }
    if(wav)wav->finish();
    diagnostic.end(limit&&nes_frames==limit?"frame_limit":"user_quit");
    if(diagnostic.failed()&&diagnostic_requested)std::cerr<<"Gameplay diagnostic incomplete: output write failed\n";
    std::cout<<"Presented "<<nes_frames<<" frames; screen="<<static_cast<unsigned>(bus.ram().read(0x045B))<<"; NMI="<<bus.nmi_count()<<"; DMA="<<bus.dma_count()<<"; DMC="<<bus.dmc_dma_count()<<"; audio_samples="<<bus.audio().produced_samples()<<"; device_submitted="<<audio_output.submitted_samples()<<"; device_skipped="<<audio_output.skipped_samples()<<"; queued_bytes="<<audio_output.queued_bytes()<<'\n';
    std::cout<<"God mode="<<god.enabled()<<"; blocked_damage="<<god.blocked_damage()<<"; rescues="<<god.rescues()<<'\n';
    timing.report(std::cout,limit&&nes_frames==limit?"frame_limit":"user_quit",nes_frames);
    return 0;
 } catch(const std::exception& error){diagnostic.error("runtime_exception");diagnostic.end("exception");timing.report(std::cout,"exception",nes_frames);show_error(error.what());return 1;}
}
