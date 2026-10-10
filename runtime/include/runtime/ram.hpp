#pragma once
#include "runtime/cpu_state.hpp"
#include "recomp/memory_access.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace nes::runtime {
class MainRAM {
public:
    static constexpr std::size_t Size = 0x0800;

    uint8_t read(uint16_t cpu_address) const;
    void write(uint16_t cpu_address, uint8_t value);
    void clear(uint8_t value = 0) noexcept;

    void push(CPUState& cpu, uint8_t value) noexcept;
    uint8_t pull(CPUState& cpu) noexcept;

private:
    // Zero fill is a deliberate runtime initialization policy, not power-on RAM.
    std::array<uint8_t, Size> bytes_{};
    static std::size_t index(uint16_t cpu_address);
};

// Current adapter handles only the main-RAM range. Full bus services come later.
MemoryAccess main_ram_access(MainRAM& ram) noexcept;

struct RuntimeState {
    CPUState cpu;
    MainRAM ram;

    explicit RuntimeState(uint16_t entry_point)
        : cpu(cpu_initial_state(entry_point)) {}
};
}
