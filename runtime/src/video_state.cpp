#include "runtime/video_state.hpp"

namespace nes::runtime {
uint8_t VideoState::palette_offset(uint16_t address) {
    uint8_t offset = static_cast<uint8_t>(address & 0x1Fu);
    if ((offset & 0x13u) == 0x10u) offset = static_cast<uint8_t>(offset & 0x0Fu);
    return offset;
}
uint8_t VideoState::read_memory(uint16_t address) const {
    address = static_cast<uint16_t>(address & 0x3FFFu);
    if (address < 0x2000) return cartridge_.chr_read(address);
    if (address < 0x3F00) return nametables_[cartridge_.nametable_offset(address)];
    return palette_[palette_offset(address)];
}
void VideoState::write_memory(uint16_t address, uint8_t value) {
    address = static_cast<uint16_t>(address & 0x3FFFu);
    // This cartridge contains read-only CHR. Native mods use separate assets.
    if (address < 0x2000) return;
    if (address < 0x3F00) nametables_[cartridge_.nametable_offset(address)] = value;
    else palette_[palette_offset(address)] = static_cast<uint8_t>(value & 0x3Fu);
}
void VideoState::update_nmi() {
    const bool active = (control_ & status_ & 0x80u) != 0;
    if (active && !nmi_line_) nmi_pending_ = true;
    nmi_line_ = active;
}
bool VideoState::consume_nmi() noexcept {
    const bool pending = nmi_pending_; nmi_pending_ = false; return pending;
}
bool VideoState::consume_frame() noexcept {
    const bool ready = frame_ready_; frame_ready_ = false; return ready;
}
uint16_t VideoState::increment_x(uint16_t address) {
    if ((address & 0x001Fu) == 31) return static_cast<uint16_t>((address & 0x7FE0u) ^ 0x0400u);
    return static_cast<uint16_t>(address + 1u);
}
uint16_t VideoState::increment_y(uint16_t address) {
    if ((address & 0x7000u) != 0x7000u) return static_cast<uint16_t>(address + 0x1000u);
    address = static_cast<uint16_t>(address & 0x0FFFu);
    unsigned y = (address >> 5) & 31u;
    if (y == 29) { y = 0; address = static_cast<uint16_t>(address ^ 0x0800u); }
    else if (y == 31) y = 0;
    else ++y;
    return static_cast<uint16_t>((address & 0x7C1Fu) | (y << 5));
}
void VideoState::increment_data_address() {
    if (rendering() && visible_or_prerender()) v_ = increment_y(increment_x(v_));
    else v_ = static_cast<uint16_t>((v_ + ((control_ & 4u) ? 32u : 1u)) & 0x7FFFu);
}
uint8_t VideoState::read_register(uint16_t address) {
    const unsigned index = address & 7u;
    if (index == 2) {
        io_latch_ = static_cast<uint8_t>((status_ & 0xE0u) | (io_latch_ & 0x1Fu));
        status_ = static_cast<uint8_t>(status_ & 0x7Fu); second_write_ = false; update_nmi();
    } else if (index == 4) io_latch_ = oam_[oam_address_];
    else if (index == 7) {
        const uint16_t location = static_cast<uint16_t>(v_ & 0x3FFFu);
        const uint8_t value = read_memory(location);
        if (location >= 0x3F00) {
            io_latch_ = static_cast<uint8_t>((io_latch_ & 0xC0u) | (value & ((mask_ & 1u) ? 0x30u : 0x3Fu)));
            read_buffer_ = read_memory(static_cast<uint16_t>(location - 0x1000u));
        } else { io_latch_ = read_buffer_; read_buffer_ = value; }
        increment_data_address();
    }
    return io_latch_;
}
void VideoState::write_oam(uint8_t value) { oam_[oam_address_++] = value; }
void VideoState::write_register(uint16_t address, uint8_t value) {
    io_latch_ = value;
    const unsigned index = address & 7u;
    // PAL power-on policy: control/mask/scroll/address writes are gated until
    // the first pre-render line. OAM and the data latch remain available.
    if (total_dots_ < 106024 && (index == 0 || index == 1 || index == 5 || index == 6)) return;
    switch (index) {
        case 0:
            control_ = value;
            t_ = static_cast<uint16_t>((t_ & 0x73FFu) | ((value & 3u) << 10));
            update_nmi(); break;
        case 1: mask_ = value; break;
        case 3: oam_address_ = value; break;
        case 4: write_oam(value); break;
        case 5:
            if (!second_write_) {
                fine_x_ = static_cast<uint8_t>(value & 7u);
                t_ = static_cast<uint16_t>((t_ & 0x7FE0u) | (value >> 3));
            } else t_ = static_cast<uint16_t>((t_ & 0x0C1Fu) | ((value & 7u) << 12) | ((value & 0xF8u) << 2));
            second_write_ = !second_write_; break;
        case 6:
            if (!second_write_) t_ = static_cast<uint16_t>((t_ & 0x00FFu) | ((value & 0x3Fu) << 8));
            else { t_ = static_cast<uint16_t>((t_ & 0x7F00u) | value); v_ = t_; }
            second_write_ = !second_write_; break;
        case 7: write_memory(v_, value); ++data_writes_; increment_data_address(); break;
        default: break;
    }
}
void VideoState::select_sprites() {
    line_sprite_count_ = 0;
    if (!rendering()) return;
    const unsigned sprite_height = (control_ & 0x20u) ? 16 : 8;
    for (unsigned i = 0; i < 64; ++i) {
        const unsigned first_y = static_cast<unsigned>(oam_[i * 4]) + 1;
        if (scanline_ < first_y || scanline_ >= first_y + sprite_height) continue;
        if (line_sprite_count_ == 8) { status_ = static_cast<uint8_t>(status_ | 0x20u); break; }
        line_sprites_[line_sprite_count_++] = static_cast<uint8_t>(i);
    }
}
void VideoState::draw_pixel(unsigned x) {
    if (x == 0) {
        auto& line = drawing_lines_[scanline_];
        line.background_chr_offset = cartridge_.chr_offset((control_ & 0x10u) ? 0x1000 : 0);
        line.chr_offsets={cartridge_.chr_offset(0),cartridge_.chr_offset(0x1000)};
        line.palette=palette_;
        line.mask = mask_;line.control=control_;
        line.scroll_address = v_; line.fine_x = fine_x_;
    }
    uint8_t background_pixel = 0, color = palette_[0];
    if ((mask_ & 8u) && (x >= 8 || (mask_ & 2u))) {
        const unsigned pixel_x = fine_x_ + (x & 7u);
        const uint16_t tile_v = pixel_x >= 8 ? increment_x(v_) : v_;
        const uint16_t name_address = static_cast<uint16_t>(0x2000u | (tile_v & 0x0FFFu));
        const uint8_t tile = read_memory(name_address);
        const uint16_t pattern = static_cast<uint16_t>(((control_ & 0x10u) ? 0x1000u : 0u) + tile * 16u + ((tile_v >> 12) & 7u));
        const unsigned bit = 7 - (pixel_x & 7u);
        background_pixel = static_cast<uint8_t>(((cartridge_.chr_read(pattern) >> bit) & 1u)
            | (((cartridge_.chr_read(static_cast<uint16_t>(pattern + 8u)) >> bit) & 1u) << 1));
        if (background_pixel) {
            const uint16_t attr_address = static_cast<uint16_t>(0x23C0u | (tile_v & 0x0C00u) | ((tile_v >> 4) & 0x38u) | ((tile_v >> 2) & 7u));
            const unsigned shift = ((tile_v >> 4) & 4u) | (tile_v & 2u);
            const uint8_t pal = static_cast<uint8_t>((read_memory(attr_address) >> shift) & 3u);
            color = palette_[pal * 4u + background_pixel];
        }
    }
    background_drawing_[scanline_ * width + x] = static_cast<uint8_t>(color & ((mask_ & 1u) ? 0x30u : 0x3Fu));
    if ((mask_ & 0x10u) && (x >= 8 || (mask_ & 4u))) {
        const unsigned sprite_height = (control_ & 0x20u) ? 16 : 8;
        for (unsigned n = 0; n < line_sprite_count_; ++n) {
            const unsigned i = line_sprites_[n], first_x = oam_[i * 4u + 3u];
            if (x < first_x || x >= first_x + 8) continue;
            const uint8_t tile = oam_[i * 4u + 1u], attributes = oam_[i * 4u + 2u];
            unsigned row = scanline_ - (static_cast<unsigned>(oam_[i * 4u]) + 1u);
            if (attributes & 0x80u) row = sprite_height - 1 - row;
            uint16_t pattern;
            if (sprite_height == 16) pattern = static_cast<uint16_t>((tile & 1u) * 4096u + ((tile & 0xFEu) + row / 8u) * 16u + (row & 7u));
            else pattern = static_cast<uint16_t>(((control_ & 8u) ? 4096u : 0u) + tile * 16u + row);
            const unsigned bit = (attributes & 0x40u) ? x - first_x : 7 - (x - first_x);
            const uint8_t pixel = static_cast<uint8_t>(((cartridge_.chr_read(pattern) >> bit) & 1u)
                | (((cartridge_.chr_read(static_cast<uint16_t>(pattern + 8u)) >> bit) & 1u) << 1));
            if (!pixel) continue;
            if (i == 0 && background_pixel && x != 255) status_ = static_cast<uint8_t>(status_ | 0x40u);
            if (!(attributes & 0x20u) || !background_pixel) color = palette_[0x10u + (attributes & 3u) * 4u + pixel];
            break;
        }
    }
    drawing_[scanline_ * width + x] = static_cast<uint8_t>(color & ((mask_ & 1u) ? 0x30u : 0x3Fu));
}
void VideoState::tick_dot() {
    if (scanline_ < 240) {
        if (dot_ == 0) select_sprites();
        if (dot_ >= 1 && dot_ <= 256) draw_pixel(dot_ - 1);
    }
    if (rendering() && visible_or_prerender()) {
        if (dot_ >= 8 && dot_ <= 256 && (dot_ & 7u) == 0) v_ = increment_x(v_);
        if (dot_ == 256) v_ = increment_y(v_);
        if (dot_ == 257) v_ = static_cast<uint16_t>((v_ & 0x7BE0u) | (t_ & 0x041Fu));
        if (scanline_ == 311 && dot_ >= 280 && dot_ <= 304) v_ = static_cast<uint16_t>((v_ & 0x041Fu) | (t_ & 0x7BE0u));
    }
    if (scanline_ == 241 && dot_ == 1) {
        frame_ = drawing_; background_frame_ = background_drawing_; frame_lines_ = drawing_lines_;
        frame_ready_ = true; ++frames_;
        status_ = static_cast<uint8_t>(status_ | 0x80u); update_nmi();
    }
    if (scanline_ == 311 && dot_ == 1) { status_ = 0; update_nmi(); }
    ++total_dots_;
    if (++dot_ == 341) { dot_ = 0; if (++scanline_ == 312) scanline_ = 0; }
}
void VideoState::advance_cpu_cycles(uint64_t cycles) {
    // 16 master ticks per PAL CPU cycle, 5 per PPU dot: retain the remainder.
    const uint64_t ticks = cycles * 16u + pal_remainder_;
    pal_remainder_ = static_cast<unsigned>(ticks % 5u);
    for (uint64_t dots = ticks / 5u; dots != 0; --dots) tick_dot();
}
}
