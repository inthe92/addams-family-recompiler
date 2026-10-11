#pragma once
#include "recomp/runtime_services.h"
#include "runtime/cartridge.hpp"
#include "runtime/ram.hpp"
#include "runtime/video_state.hpp"
#include "runtime/audio_state.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace nes::runtime {
struct AudioWrite { uint64_t cycle; uint16_t address; uint8_t value; };
class Bus {
public:
    explicit Bus(CartridgeData data) : cartridge_(data), video_(cartridge_) {}
    RuntimeServices services();
    uint8_t read8(uint16_t address);
    uint8_t read8(uint16_t address, uint8_t cycle_offset);
    void write8(uint16_t address, uint8_t value, uint8_t cycle_offset = 0);
    void advance(uint64_t cycles);
    uint8_t boundary(CPUState& cpu, uint8_t irq_mask = 0xFF);
    void set_buttons(unsigned port, uint8_t buttons);
    MainRAM& ram() noexcept { return ram_; }
    const MainRAM& ram() const noexcept { return ram_; }
    Cartridge& cartridge() noexcept { return cartridge_; }
    const Cartridge& cartridge() const noexcept { return cartridge_; }
    VideoState& video() noexcept { return video_; }
    const VideoState& video() const noexcept { return video_; }
    AudioState& audio() noexcept { return audio_; }
    uint64_t cycles() const noexcept { return cycles_; }
    uint64_t nmi_count() const noexcept { return nmi_count_; }
    uint64_t dma_count() const noexcept { return dma_count_; }
    uint64_t dmc_dma_count() const noexcept { return dmc_dma_count_; }
    uint64_t irq_count() const noexcept { return irq_count_; }
    const std::vector<AudioWrite>& audio_writes() const noexcept { return audio_writes_; }

private:
    MainRAM ram_;
    Cartridge cartridge_;
    VideoState video_;
    AudioState audio_;
    uint64_t cycles_{}, device_cycles_{}, nmi_count_{}, dma_count_{}, dmc_dma_count_{}, irq_count_{};
    uint8_t open_bus_{};
    std::array<uint8_t, 2> buttons_{}, controller_shift_{};
    bool strobe_{}, dma_pending_{};
    uint8_t dma_page_{};
    // Keep timed writes for diagnostics alongside native channel state.
    std::vector<AudioWrite> audio_writes_;
    void sync_devices(uint64_t target);
    void perform_dma();
    void perform_dmc_dma(bool during_oam = false);
    void deliver_interrupt(CPUState& cpu, uint16_t vector);
};
}
