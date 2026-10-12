#include "runtime/sdl_audio_output.hpp"
#include "runtime/audio_state.hpp"
#include <iostream>
#include <stdexcept>

namespace nes::runtime {
SDLAudioOutput::SDLAudioOutput(bool enabled, bool unthrottled) : unthrottled_(unthrottled) {
    if (!enabled) return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::cerr << "Audio device unavailable: " << SDL_GetError() << '\n'; return;
    }
    SDL_AudioSpec requested{}, obtained{};
    requested.freq = AudioState::sample_rate; requested.format = AUDIO_F32SYS;
    requested.channels = 1; requested.samples = 1024;
    device_ = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, 0);
    if (!device_) {std::cerr << "Audio device unavailable: " << SDL_GetError() << '\n'; return;}
    std::cout << "SDL audio " << SDL_GetCurrentAudioDriver() << ": " << obtained.freq << "Hz mono float32; buffer=" << obtained.samples << '\n';
}
SDLAudioOutput::~SDLAudioOutput() {if (device_) SDL_CloseAudioDevice(device_);}
uint32_t SDLAudioOutput::queued_bytes() const noexcept {return device_ ? SDL_GetQueuedAudioSize(device_) : 0;}
void SDLAudioOutput::set_paused(bool value) {
    if(value==paused_)return;paused_=value;if(device_)SDL_PauseAudioDevice(device_,paused_||!started_?1:0);
}
void SDLAudioOutput::submit(const std::vector<float>& samples) {
    if (!device_ || samples.empty()) return;
    // A fast diagnostic can generate many seconds per wall-clock second.
    // Bound latency without changing the game's APU timing or sample sequence.
    constexpr Uint32 capacity = AudioState::sample_rate * sizeof(float) / 4u;
    if (samples.size() > capacity / sizeof(float)) {skipped_ += samples.size(); return;}
    const auto bytes = static_cast<Uint32>(samples.size() * sizeof(float));
    if (!unthrottled_) {
        const Uint64 deadline = SDL_GetTicks64() + 150;
        while (queued_bytes() + bytes > capacity && SDL_GetTicks64() < deadline) SDL_Delay(1);
    }
    if (queued_bytes() + bytes > capacity) {skipped_ += samples.size(); return;}
    if (SDL_QueueAudio(device_, samples.data(), bytes) != 0) throw std::runtime_error(SDL_GetError());
    submitted_ += samples.size();
    // Start with two PAL frames queued so the device has room for host jitter.
    if (!started_ && queued_bytes() >= AudioState::sample_rate * sizeof(float) / 25u) {
        started_ = true; SDL_PauseAudioDevice(device_, paused_?1:0);
    }
}
}
