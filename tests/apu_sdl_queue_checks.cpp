#include "runtime/audio_state.hpp"
#include "runtime/sdl_audio_output.hpp"
#include <SDL.h>
#include <iostream>
#include <stdexcept>
#include <vector>

// Reuses the legacy SDL queue check with CTest's dummy device. This tests
// delivery/backpressure, not speakers, host-driver quality or song fidelity.
static void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
int main() {
    try {
        require(SDL_Init(SDL_INIT_TIMER)==0,"SDL timer initialization");
        {
            nes::runtime::AudioState audio;
            audio.write_register(0x4017,0x40); audio.write_register(0x4015,1);
            audio.write_register(0x4000,0x7f); audio.write_register(0x4002,99); audio.write_register(0x4003,0);
            audio.advance_cpu_cycles(166261); auto pcm=audio.take_samples();
            nes::runtime::SDLAudioOutput device(true);
            require(device.available(),"Dummy audio device must open; no skip exemption");
            device.submit(pcm); const auto before=device.queued_bytes();
            require(device.submitted_samples()==pcm.size() && before>0,"PCM submitted to SDL queue");
            const auto deadline=SDL_GetTicks64()+1000;
            while(device.queued_bytes()>=before && SDL_GetTicks64()<deadline) SDL_Delay(10);
            require(device.queued_bytes()<before,"Dummy device consumes queued PCM");
            device.submit(std::vector<float>(20000,0));
            require(device.skipped_samples()==20000,"Oversized batch is reported, not queued");
            nes::runtime::SDLAudioOutput disabled(false); disabled.submit(pcm);
            require(!disabled.available() && disabled.submitted_samples()==0,"Muted output creates no audio device");
        }
        SDL_Quit(); std::cout<<"1 synthetic SDL dummy audio queue case: PASS\n";return 0;
    } catch(const std::exception& e) {SDL_Quit();std::cerr<<"SDL audio case: FAIL: "<<e.what()<<'\n';return 1;}
}
