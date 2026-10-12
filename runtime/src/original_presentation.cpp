#include "runtime/original_presentation.hpp"
#include "runtime/palette.hpp"
#include <algorithm>
#include <stdexcept>

namespace nes::runtime {
PictureViewport original_viewport(int width, int height, bool fullscreen) {
    if (width < 4 || height < 3) return {};
    int picture_width, picture_height;
    const int scale = std::min(width / 320, height / 240);
    if (!fullscreen && scale >= 1) {
        picture_width = scale * 320;
        picture_height = scale * 240;
    } else {
        const int scale4by3 = std::min(width / 4, height / 3);
        picture_width = scale4by3 * 4;
        picture_height = scale4by3 * 3;
    }
    return {(width - picture_width) / 2, (height - picture_height) / 2,
            picture_width, picture_height};
}
void original_rgba(const uint8_t* indices, unsigned width, unsigned height, OriginalRGBA& output) {
    if (!indices || width != original_width || height != original_height)
        throw std::invalid_argument("Presentation requires the original 256 x 240 NES frame");
    for (std::size_t i = 0; i < original_width * original_height; ++i) {
        const uint32_t color = rgb_palette[indices[i] & 63u];
        output[i * 4] = static_cast<uint8_t>(color >> 16);
        output[i * 4 + 1] = static_cast<uint8_t>(color >> 8);
        output[i * 4 + 2] = static_cast<uint8_t>(color);
        output[i * 4 + 3] = 255;
    }
}
double next_pal_deadline(double deadline, double frequency, double now) {
    deadline += frequency / 50.007;
    if (now > deadline + frequency / 10.0) deadline = now;
    return deadline;
}
}
