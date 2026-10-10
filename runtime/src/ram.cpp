#include "runtime/ram.hpp"
#include <stdexcept>

namespace nes::runtime {
std::size_t MainRAM::index(uint16_t address) {
    if (address >= 0x2000) {
        throw std::out_of_range("MainRAM handles only CPU addresses $0000-$1FFF");
    }
    return static_cast<std::size_t>(address & 0x07FFu);
}

uint8_t MainRAM::read(uint16_t address) const {
    return bytes_[index(address)];
}

void MainRAM::write(uint16_t address, uint8_t value) {
    bytes_[index(address)] = value;
}

void MainRAM::clear(uint8_t value) noexcept {
    bytes_.fill(value);
}

void MainRAM::push(CPUState& cpu, uint8_t value) noexcept {
    bytes_[static_cast<std::size_t>(0x0100u | cpu.SP)] = value;
    cpu.SP = static_cast<uint8_t>(cpu.SP - 1u);
}

uint8_t MainRAM::pull(CPUState& cpu) noexcept {
    cpu.SP = static_cast<uint8_t>(cpu.SP + 1u);
    return bytes_[static_cast<std::size_t>(0x0100u | cpu.SP)];
}

MemoryAccess main_ram_access(MainRAM& ram) noexcept {
    return MemoryAccess{
        &ram,
        [](void* context, uint16_t address) -> uint8_t {
            return static_cast<MainRAM*>(context)->read(address);
        },
        [](void* context, uint16_t address, uint8_t value) {
            static_cast<MainRAM*>(context)->write(address, value);
        }
    };
}
}
