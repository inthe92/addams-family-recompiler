#pragma once
#include <SDL.h>
#include <cstdint>
#include <vector>

namespace nes::runtime {
// The device receives copies of PCM; no SDL thread reads game or APU state.
class SDLAudioOutput {
public:
    explicit SDLAudioOutput(bool enabled, bool unthrottled = false);
    ~SDLAudioOutput();
    SDLAudioOutput(const SDLAudioOutput&) = delete;
    SDLAudioOutput& operator=(const SDLAudioOutput&) = delete;
    void submit(const std::vector<float>& samples);
    bool available() const noexcept { return device_ != 0; }
    uint64_t submitted_samples() const noexcept { return submitted_; }
    uint64_t skipped_samples() const noexcept { return skipped_; }
    uint32_t queued_bytes() const noexcept;
private:
    SDL_AudioDeviceID device_{};
    uint64_t submitted_{}, skipped_{};
    bool started_{};
    bool unthrottled_{};
};
}
