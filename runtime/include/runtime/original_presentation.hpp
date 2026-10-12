#pragma once
#include <array>
#include <cstdint>

namespace nes::runtime {
inline constexpr unsigned original_width = 256, original_height = 240;
// 320:240 display geometry gives the requested 4:3 picture (5:4 pixel aspect).
struct PictureViewport { int x{}, y{}, width{}, height{}; };
PictureViewport original_viewport(int drawable_width, int drawable_height, bool fullscreen);
using OriginalRGBA = std::array<uint8_t, original_width * original_height * 4>;
void original_rgba(const uint8_t* indices, unsigned width, unsigned height, OriginalRGBA& output);
// Existing desktop PAL host pacing, independent of display refresh/viewport.
double next_pal_deadline(double deadline, double frequency, double now);
}
