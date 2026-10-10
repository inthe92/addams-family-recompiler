#include "runtime/cartridge.hpp"
#include <stdexcept>

namespace nes::runtime {
static bool power_of_two(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}
Cartridge::Cartridge(CartridgeData data) : data_(data) {
    if (!data.prg || !power_of_two(data.prg_size) || data.prg_size < 32768
        || data.prg_size > 262144 || !data.chr || !power_of_two(data.chr_size)
        || data.chr_size < 8192 || data.chr_size > 131072) {
        throw std::invalid_argument("Standard MMC1B PRG/CHR-ROM resource layout required");
    }
    prg_mask_ = static_cast<uint8_t>(data.prg_size / 16384 - 1);
    chr_mask_ = static_cast<uint8_t>(data.chr_size / 4096 - 1);
}

uint8_t Cartridge::bank_for_pc(uint16_t address) const noexcept {
    if (address < 0x8000) return 0xFF;
    const uint8_t window = address >= 0xC000 ? 1 : 0;
    const uint8_t mode = static_cast<uint8_t>((control_ >> 2) & 3u);
    uint8_t bank;
    if (mode < 2) bank = static_cast<uint8_t>((prg_ & 0x0Eu) + window);
    else if (mode == 2) bank = window ? static_cast<uint8_t>(prg_ & 0x0Fu) : 0;
    else bank = window ? prg_mask_ : static_cast<uint8_t>(prg_ & 0x0Fu);
    return static_cast<uint8_t>(bank & prg_mask_);
}

uint8_t Cartridge::cpu_read(uint16_t address, uint8_t open_bus) const {
    if (address >= 0x8000) {
        return data_.prg[static_cast<std::size_t>(bank_for_pc(address)) * 16384
                         + (address & 0x3FFFu)];
    }
    if (address >= 0x6000 && prg_ram_enabled()) return prg_ram_[address & 0x1FFFu];
    return open_bus;
}

void Cartridge::cpu_write(uint16_t address, uint8_t value, uint64_t cycle) {
    if (has_previous_write_ && cycle <= previous_write_cycle_) {
        throw std::logic_error("CPU write timestamps must increase");
    }
    const bool consecutive = has_previous_write_ && cycle == previous_write_cycle_ + 1;
    previous_write_cycle_ = cycle;
    has_previous_write_ = true;
    if (address < 0x8000) {
        if (address >= 0x6000 && prg_ram_enabled()) prg_ram_[address & 0x1FFFu] = value;
        return;
    }
    // D7 reset is never ignored, including the second RMW write.
    if ((value & 0x80u) != 0) {
        shift_ = 0x10;
        control_ = static_cast<uint8_t>(control_ | 0x0Cu);
        return;
    }
    if (consecutive) return;
    const bool full = (shift_ & 1u) != 0;
    shift_ = static_cast<uint8_t>((shift_ >> 1) | ((value & 1u) << 4));
    if (!full) return;
    switch ((address >> 13) & 3u) {
        case 0: control_ = shift_; break;
        case 1: chr0_ = shift_; break;
        case 2: chr1_ = shift_; break;
        case 3: prg_ = shift_; break;
    }
    shift_ = 0x10;
}

uint32_t Cartridge::chr_offset(uint16_t address) const {
    if (address >= 0x2000) throw std::out_of_range("CHR address outside pattern tables");
    uint8_t bank;
    if ((control_ & 0x10u) != 0) bank = address < 0x1000 ? chr0_ : chr1_;
    else bank = static_cast<uint8_t>((chr0_ & 0x1Eu) + (address >= 0x1000 ? 1u : 0u));
    return static_cast<uint32_t>(bank & chr_mask_) * 4096u + (address & 0x0FFFu);
}
uint8_t Cartridge::chr_read(uint16_t address) const { return data_.chr[chr_offset(address)]; }

uint16_t Cartridge::nametable_offset(uint16_t address) const {
    if (address < 0x2000 || address >= 0x3F00) throw std::out_of_range("Address outside nametables");
    const uint16_t table = static_cast<uint16_t>((address >> 10) & 3u);
    uint16_t page;
    switch (control_ & 3u) {
        case 0: page = 0; break;
        case 1: page = 1; break;
        case 2: page = static_cast<uint16_t>(table & 1u); break;
        default: page = static_cast<uint16_t>(table >> 1); break;
    }
    return static_cast<uint16_t>(page * 1024u + (address & 0x03FFu));
}
}
