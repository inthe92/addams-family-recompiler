#pragma once
#include "runtime/bus.hpp"
#include <array>
#include <cstdint>

namespace nes::game {
class ActorView;
// This layer knows the game's camera/map fields; the generic bus does not.
class WorldView {
public:
    static constexpr unsigned width=427, height=240, original_left=85;
    void compose(const runtime::Bus& bus,const ActorView* actors=nullptr);
    const std::array<uint8_t,width*height>& frame() const noexcept {return frame_;}
    const std::array<uint8_t,width*192>& base_background() const noexcept {return base_background_;}
    bool extended_background() const noexcept {return extended_;}
    uint16_t camera_x() const noexcept {return camera_x_;}
    unsigned lateral_sprite_pixels() const noexcept {return lateral_sprite_pixels_;}
private:
    std::array<uint8_t,width*height> frame_{};
    std::array<uint8_t,width*192> base_background_{};
    std::array<uint8_t,width*192> background_opacity_{};
    bool extended_{};
    uint16_t camera_x_{};
    unsigned lateral_sprite_pixels_{};
};
}
