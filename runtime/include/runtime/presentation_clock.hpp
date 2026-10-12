#pragma once
#include "runtime/original_presentation.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace nes::frontend {
// Host scheduling only: no CPU services, SDL, callbacks or NES clock changes.
class PresentationClock {
public:
    struct Due { bool simulate, present; };
    PresentationClock(uint64_t frequency, unsigned refresh, uint64_t now);
    Due poll(uint64_t now) noexcept;
    void simulation_complete(uint64_t now) noexcept;
    void presentation_complete(uint64_t now) noexcept;
    void set_paused(bool paused, uint64_t now) noexcept;
    double next_deadline() const noexcept;
    double simulation_deadline() const noexcept { return nes_deadline_; }
    double presentation_deadline() const noexcept { return draw_deadline_; }
    uint64_t suspension_reanchors() const noexcept { return reanchors_; }
private:
    double frequency_, draw_period_, nes_deadline_, draw_deadline_;
    uint64_t last_now_, reanchors_{};
    bool paused_{};
    double monotonic(uint64_t now) noexcept;
};
unsigned parse_experimental_refresh(const char* value);
// Owning copy. Readers cannot change pixels or reach live VideoState/Bus.
class FrameSnapshot {
public:
    using Pixels = std::array<uint8_t,nes::runtime::original_width*nes::runtime::original_height>;
    void complete(const Pixels& pixels, uint64_t sequence, uint64_t ppu_frame,
                  uint64_t cpu_cycle, uint64_t host_tick) noexcept {
        pixels_=pixels;publication_=next_publication();sequence_=sequence;ppu_frame_=ppu_frame;cpu_cycle_=cpu_cycle;host_tick_=host_tick;valid_=true;
    }
    const Pixels& pixels() const noexcept { return pixels_; }
    bool valid() const noexcept { return valid_; }
    uint64_t publication() const noexcept { return publication_; }
    uint64_t sequence() const noexcept { return sequence_; }
    uint64_t ppu_frame() const noexcept { return ppu_frame_; }
    uint64_t cpu_cycle() const noexcept { return cpu_cycle_; }
    uint64_t host_tick() const noexcept { return host_tick_; }
private:
    static uint64_t next_publication() noexcept;
    Pixels pixels_{};
    bool valid_{};
    uint64_t publication_{}, sequence_{}, ppu_frame_{}, cpu_cycle_{}, host_tick_{};
};
}
