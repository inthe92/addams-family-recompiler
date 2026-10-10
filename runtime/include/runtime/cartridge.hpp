#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace nes::runtime {
struct CartridgeData {
    const uint8_t* prg;
    std::size_t prg_size;
    const uint8_t* chr;
    std::size_t chr_size;
};

// Runtime services for the target's standard MMC1B PRG/CHR-ROM wiring.
// Source bytes are compiled constants; this class has no ROM loader or CPU.
class Cartridge {
public:
    explicit Cartridge(CartridgeData data);
    uint8_t cpu_read(uint16_t address, uint8_t open_bus = 0) const;
    // The bus notifies every CPU write so adjacent-cycle suppression is tracked.
    void cpu_write(uint16_t address, uint8_t value, uint64_t cycle);
    uint8_t bank_for_pc(uint16_t address) const noexcept;
    uint8_t chr_read(uint16_t address) const;
    uint32_t chr_offset(uint16_t address) const;
    uint16_t nametable_offset(uint16_t ppu_address) const;
    uint8_t control() const noexcept { return control_; }
    uint8_t chr0() const noexcept { return chr0_; }
    uint8_t chr1() const noexcept { return chr1_; }
    uint8_t prg_register() const noexcept { return prg_; }
    uint8_t shift_register() const noexcept { return shift_; }
    bool prg_ram_enabled() const noexcept { return (prg_ & 0x10u) == 0; }

private:
    CartridgeData data_;
    std::array<uint8_t, 8192> prg_ram_{};
    uint8_t prg_mask_{};
    uint8_t chr_mask_{};
    uint8_t shift_{0x10};
    uint8_t control_{0x0C};
    uint8_t chr0_{};
    uint8_t chr1_{};
    uint8_t prg_{};
    bool has_previous_write_{};
    uint64_t previous_write_cycle_{};
};
}
