#include "game/compiled_program.h"
#include "game/recovered_resources.h"
#include "game/world_view.hpp"
#include "game/actor_view.hpp"
#include "game/god_mode.hpp"
#include "runtime/bus.hpp"
#include "runtime/gl_presenter.hpp"
#include "runtime/sdl_audio_output.hpp"
#include "runtime/wav_writer.hpp"
#ifdef NES_USER_ROM
#include "runtime/user_rom.hpp"
#endif
#ifdef NES_ENABLE_MODS
#include "runtime/mod_engine.hpp"
#endif
#include <SDL.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

struct SDLQuit { ~SDLQuit(){SDL_Quit();} };
struct GLContext {SDL_GLContext value{};~GLContext(){if(value)SDL_GL_DeleteContext(value);} };
struct Controller {SDL_GameController* value{};~Controller(){if(value)SDL_GameControllerClose(value);} };
static void show_error(const std::string& message) {
    std::cerr << message << '\n';
#ifdef NES_ALPHA_BUILD
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Addams Family — Alpha 0.2.1",
        ("La alfa se ha detenido.\n\n" + message +
         "\n\nEsta version es parcial; el juego completo sigue en desarrollo.").c_str(), nullptr);
#endif
}
static uint8_t buttons(SDL_GameController* pad) {
    const auto* keys=SDL_GetKeyboardState(nullptr);uint8_t value=0;
    const struct { SDL_Scancode key; SDL_GameControllerButton button; uint8_t mask; } map[]={
        {SDL_SCANCODE_Z,SDL_CONTROLLER_BUTTON_A,1},{SDL_SCANCODE_X,SDL_CONTROLLER_BUTTON_B,2},
        {SDL_SCANCODE_RSHIFT,SDL_CONTROLLER_BUTTON_BACK,4},{SDL_SCANCODE_RETURN,SDL_CONTROLLER_BUTTON_START,8},
        {SDL_SCANCODE_UP,SDL_CONTROLLER_BUTTON_DPAD_UP,16},{SDL_SCANCODE_DOWN,SDL_CONTROLLER_BUTTON_DPAD_DOWN,32},
        {SDL_SCANCODE_LEFT,SDL_CONTROLLER_BUTTON_DPAD_LEFT,64},{SDL_SCANCODE_RIGHT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,128}};
    for(const auto& binding:map)if(keys[binding.key]||(pad&&SDL_GameControllerGetButton(pad,binding.button)))value=static_cast<uint8_t>(value|binding.mask);
    if(pad){const Sint16 x=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),y=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);
        if(x<-16000)value=static_cast<uint8_t>(value|64u);if(x>16000)value=static_cast<uint8_t>(value|128u);
        if(y<-16000)value=static_cast<uint8_t>(value|16u);if(y>16000)value=static_cast<uint8_t>(value|32u);}
    return value;
}
int main(int argc,char** argv) {
 try {
    SDL_SetMainReady();
#ifdef NES_USER_ROM
    nes::runtime::UserRomModule imported;
    if(!imported.prepare(argc,argv))return 0;
#endif
    uint64_t limit=0;bool fast=false,mute=false;std::string mod_path,capture,audio_path;
    bool god_enabled=false;
#ifdef NES_ALPHA_BUILD
    god_enabled=true;
#endif
    for(int i=1;i<argc;++i){const std::string option=argv[i];if(option=="--unthrottled"){fast=true;continue;}
#ifdef NES_USER_ROM
        if(option=="--import-only")continue;
        if(option=="--rom"||option=="--import-cache"||option=="--import-tools"){++i;continue;}
        if(option.rfind("-psn_",0)==0)continue;
#endif
        if(option=="--mute"){mute=true;continue;}
        if(option=="--god"){god_enabled=true;continue;}
        if(option=="--no-god"){god_enabled=false;continue;}
        if(i+1>=argc)throw std::invalid_argument("Option requires a value");
        if(option=="--frames")limit=std::stoull(argv[++i]);else if(option=="--mod")mod_path=argv[++i];
        else if(option=="--capture")capture=argv[++i];else if(option=="--audio-output")audio_path=argv[++i];else throw std::invalid_argument("Unknown desktop option");}
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0)throw std::runtime_error(SDL_GetError());SDLQuit quit;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    const char* title="The Addams Family — Native";
#ifdef NES_ALPHA_BUILD
    title="Addams Family — Alpha 0.2.1 · GOD activo · G: cambiar · Enter: empezar · Z/X";
#endif
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,540,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
    if(!window)throw std::runtime_error(SDL_GetError());GLContext context{SDL_GL_CreateContext(window.get())};
    if(!context.value)throw std::runtime_error(SDL_GetError());
    // PAL pacing below owns the clock. A Cocoa display-link wait can stop
    // receiving callbacks for a background/offscreen window and block input.
    SDL_GL_SetSwapInterval(0);
    nes::runtime::GLPresenter presenter(window.get());
    std::cout<<"OpenGL "<<presenter.version()<<"; statically recompiled C game; PAL runtime\n";
    nes::runtime::Bus bus({&game_prg_data[0][0],131072u,game_chr_data,131072u});
    nes::game::ActorView actors;
    CPUState cpu=cpu_initial_state(game_reset_vector);RuntimeServices services=actors.services(bus);
    nes::game::GodMode god;services=god.services(bus,services);god.set_enabled(god_enabled);
    const auto update_title=[&]{
        const std::string caption=std::string("Addams Family — ")+
#ifdef NES_ALPHA_BUILD
            "Alpha 0.2.1 · "+
#else
            "Native · "+
#endif
            (god.enabled()?"GOD activo":"GOD desactivado")+
            (god.all_items()?" · OBJETOS completos":" · H: todos los objetos")+" · G: GOD · Enter · Flechas · Z/X";
        SDL_SetWindowTitle(window.get(),caption.c_str());
    };
    update_title();
    nes::runtime::SDLAudioOutput audio_output(!mute,fast);
    std::unique_ptr<nes::runtime::WavWriter> wav;
    if(!audio_path.empty())wav=std::make_unique<nes::runtime::WavWriter>(audio_path,nes::runtime::AudioState::sample_rate);
#ifdef NES_ENABLE_MODS
    nes::runtime::ModEngine mods(bus);if(!mod_path.empty())mods.load_file(mod_path);
#else
    if(!mod_path.empty())throw std::runtime_error("Lua support is disabled in this build");
#endif
    Controller controller;for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){controller.value=SDL_GameControllerOpen(i);break;}
    bool running=true;const Uint64 frequency=SDL_GetPerformanceFrequency();
    double next_deadline=static_cast<double>(SDL_GetPerformanceCounter());
    uint64_t presented=0;nes::game::WorldView world_view;
    while(running&&(!limit||presented<limit)) {
        SDL_Event event;while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT||(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_ESCAPE))running=false;
            if(event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_g){god.set_enabled(!god.enabled());update_title();}
            if(event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_h){god.grant_all_items();update_title();}
            if(event.type==SDL_CONTROLLERDEVICEADDED&&!controller.value)controller.value=SDL_GameControllerOpen(event.cdevice.which);
            if(event.type==SDL_CONTROLLERDEVICEREMOVED&&controller.value&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.value))==event.cdevice.which){SDL_GameControllerClose(controller.value);controller.value=nullptr;}
        }
        if(!running)break;
        bus.set_buttons(0,buttons(controller.value));
#ifdef NES_ENABLE_MODS
        mods.frame(cpu);
#endif
        god.begin_frame();
        const uint64_t before=bus.video().frames();
        while(bus.video().frames()==before) {
            uint32_t budget=1000;if(!game_execute_slice(&cpu,&services,&budget)){
                show_error("Falta una rutina nativa: banco "+std::to_string(bus.cartridge().bank_for_pc(cpu.PC))+", PC decimal "+std::to_string(cpu.PC));return 2;}
        }
        bus.video().consume_frame();auto samples=bus.audio().take_samples();audio_output.submit(samples);if(wav)wav->append(samples);
        world_view.compose(bus,&actors);presenter.present(world_view.frame().data(),world_view.width,world_view.height);++presented;
        if(limit&&presented==limit&&!capture.empty())presenter.capture(capture);
        SDL_GL_SwapWindow(window.get());
        if(!fast){next_deadline+=static_cast<double>(frequency)/50.007;
            const auto now=SDL_GetPerformanceCounter();
            // After a long host pause, resume at normal speed instead of
            // generating a burst of catch-up frames that drops queued sound.
            if(static_cast<double>(now)>next_deadline+static_cast<double>(frequency)/10.0)next_deadline=static_cast<double>(now);
            while(static_cast<double>(SDL_GetPerformanceCounter())<next_deadline)SDL_Delay(1);}
    }
    if(wav)wav->finish();
    std::cout<<"Presented "<<presented<<" frames; screen="<<static_cast<unsigned>(bus.ram().read(0x045B))<<"; NMI="<<bus.nmi_count()<<"; DMA="<<bus.dma_count()<<"; DMC="<<bus.dmc_dma_count()<<"; audio_samples="<<bus.audio().produced_samples()<<"; device_submitted="<<audio_output.submitted_samples()<<"; device_skipped="<<audio_output.skipped_samples()<<"; queued_bytes="<<audio_output.queued_bytes()<<"; extended_background="<<world_view.extended_background()<<"; camera_x="<<world_view.camera_x()<<"; lateral_sprite_pixels="<<world_view.lateral_sprite_pixels()<<'\n';
    std::cout<<"God mode="<<god.enabled()<<"; blocked_damage="<<god.blocked_damage()<<"; rescues="<<god.rescues()<<'\n';
    return 0;
 } catch(const std::exception& error){show_error(error.what());return 1;}
}
