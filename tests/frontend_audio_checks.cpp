#include "runtime/sdl_audio_output.hpp"
#include <SDL.h>
#include <iostream>
#include <stdexcept>
using nes::runtime::SDLAudioOutput;
int main(){try{
 SDLAudioOutput output(true);if(!output.available())throw std::runtime_error("mandatory dummy audio device");
 output.set_paused(true);output.submit(std::vector<float>(2400,0.25f));const auto queued=output.queued_bytes();SDL_Delay(50);
 if(!output.paused()||!queued||output.queued_bytes()!=queued)throw std::runtime_error("modal pause consumed queued samples");
 output.set_paused(false);const auto deadline=SDL_GetTicks64()+1000;while(output.queued_bytes()==queued&&SDL_GetTicks64()<deadline)SDL_Delay(10);
 if(output.paused()||output.queued_bytes()>=queued)throw std::runtime_error("cancel did not resume audio");
 std::cout<<"Frontend SDL audio cases=2 PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
