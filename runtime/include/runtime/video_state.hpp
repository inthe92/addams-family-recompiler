#pragma once
#include "runtime/cartridge.hpp"
#include <array>
#include <cstdint>

namespace nes::runtime {
struct VideoLineStyle {
    uint32_t background_chr_offset{};
    std::array<uint32_t,2> chr_offsets{};
    std::array<uint8_t,32> palette{};
    uint8_t mask{},control{};
    uint16_t scroll_address{};
    uint8_t fine_x{};
};
// Logical tile/sprite state consumed by the native renderer. PAL is the target
// cartridge's timing. This does not fetch or execute CPU instructions.
class VideoState {
public:
    static constexpr unsigned width = 256, height = 240;
    explicit VideoState(Cartridge& cartridge) : cartridge_(cartridge) {}
    uint8_t read_register(uint16_t address);
    void write_register(uint16_t address, uint8_t value);
    void write_oam(uint8_t value);
    void advance_cpu_cycles(uint64_t cycles);
    bool consume_nmi() noexcept;
    bool consume_frame() noexcept;
    bool pending_frame() const noexcept { return frame_ready_; }
    uint64_t frames() const noexcept { return frames_; }
    uint64_t dots() const noexcept { return total_dots_; }
    uint16_t scanline() const noexcept { return scanline_; }
    uint16_t dot() const noexcept { return dot_; }
    uint8_t control() const noexcept { return control_; }
    uint8_t mask() const noexcept { return mask_; }
    uint8_t status() const noexcept { return status_; }
    uint16_t address() const noexcept { return v_; }
    uint16_t temporary_address() const noexcept { return t_; }
    const std::array<uint8_t, width * height>& frame() const noexcept { return frame_; }
    const std::array<uint8_t, width * height>& background_frame() const noexcept { return background_frame_; }
    const std::array<VideoLineStyle,height>& line_styles() const noexcept { return frame_lines_; }
    const std::array<uint8_t, 256>& oam() const noexcept { return oam_; }
    uint8_t read_memory(uint16_t address) const;
    void write_memory(uint16_t address, uint8_t value);
    uint64_t data_writes() const noexcept { return data_writes_; }

private:
    Cartridge& cartridge_;
    std::array<uint8_t, 2048> nametables_{};
    std::array<uint8_t, 32> palette_{};
    std::array<uint8_t, 256> oam_{};
    std::array<uint8_t, width * height> drawing_{};
    std::array<uint8_t, width * height> frame_{};
    std::array<uint8_t, width * height> background_drawing_{}, background_frame_{};
    std::array<VideoLineStyle,height> drawing_lines_{}, frame_lines_{};
    std::array<uint8_t, 8> line_sprites_{};
    unsigned line_sprite_count_{};
    uint8_t control_{}, mask_{}, status_{}, oam_address_{};
    uint8_t fine_x_{}, read_buffer_{}, io_latch_{};
    uint16_t v_{}, t_{}, dot_{}, scanline_{};
    unsigned pal_remainder_{};
    uint64_t total_dots_{}, frames_{}, data_writes_{};
    bool second_write_{}, nmi_line_{}, nmi_pending_{}, frame_ready_{};
    bool rendering() const noexcept { return (mask_ & 0x18u) != 0; }
    bool visible_or_prerender() const noexcept { return scanline_ < 240 || scanline_ == 311; }
    void update_nmi();
    void tick_dot();
    void select_sprites();
    void draw_pixel(unsigned x);
    void increment_data_address();
    static uint16_t increment_x(uint16_t address);
    static uint16_t increment_y(uint16_t address);
    static uint8_t palette_offset(uint16_t address);
};
}
