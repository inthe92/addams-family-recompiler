#ifndef NES_RECOMP_CPU_STATE_H
#define NES_RECOMP_CPU_STATE_H

#include <stdint.h>

/* C-compatible definition shared by the C++ runtime and generated C blocks. */
typedef struct CPUState {
    uint8_t A;
    uint8_t X;
    uint8_t Y;
    uint8_t SP;  /* Stack address = $0100 | SP, not a 16-bit host pointer. */
    uint8_t P;
    uint16_t PC;
} CPUState;

enum CPUFlag {
    CPU_C = 0x01, CPU_Z = 0x02, CPU_I = 0x04, CPU_D = 0x08,
    CPU_B = 0x10, CPU_U = 0x20, CPU_V = 0x40, CPU_N = 0x80
};

/* B/U are not physical status latches. This ABI stores U=1/B=0 in P. */
static inline uint8_t cpu_normalize_status(uint8_t value) {
    return (uint8_t)((value & 0xCFu) | CPU_U);
}

/* Deterministic project bootstrap; not a complete hardware reset sequence. */
static inline CPUState cpu_initial_state(uint16_t entry_point) {
    CPUState cpu = {0, 0, 0, 0xFD, CPU_I | CPU_U, entry_point};
    return cpu;
}

static inline uint8_t cpu_has_flag(const CPUState* cpu, uint8_t flag) {
    return (uint8_t)((cpu->P & flag) != 0);
}

static inline void cpu_set_flag(CPUState* cpu, uint8_t flag, uint8_t enabled) {
    const uint8_t mask = (uint8_t)(flag & 0xCFu);
    const uint8_t value = enabled ? (uint8_t)(cpu->P | mask)
                                  : (uint8_t)(cpu->P & (uint8_t)~mask);
    cpu->P = cpu_normalize_status(value);
}

static inline void cpu_set_nz(CPUState* cpu, uint8_t value) {
    cpu_set_flag(cpu, CPU_Z, (uint8_t)(value == 0));
    cpu_set_flag(cpu, CPU_N, (uint8_t)((value & 0x80u) != 0));
}

/* PHP/BRK pass 1; IRQ/NMI pass 0. Saving P does not set a B latch. */
static inline uint8_t cpu_status_for_push(const CPUState* cpu, uint8_t software) {
    return (uint8_t)(cpu_normalize_status(cpu->P) | (software ? CPU_B : 0));
}

/* PLP/RTI ignore bits 4/5 from the saved byte. */
static inline void cpu_status_from_pull(CPUState* cpu, uint8_t value) {
    cpu->P = cpu_normalize_status(value);
}

/* NES 2A03/2A07 arithmetic is binary even when the D flag is set. */
static inline void cpu_adc(CPUState* cpu, uint8_t operand) {
    const uint8_t before = cpu->A;
    const uint16_t sum = (uint16_t)((uint16_t)before + (uint16_t)operand
                                    + (uint16_t)cpu_has_flag(cpu, CPU_C));
    const uint8_t result = (uint8_t)sum;
    cpu_set_flag(cpu, CPU_C, (uint8_t)(sum > 0xFFu));
    cpu_set_flag(cpu, CPU_V, (uint8_t)(((uint8_t)~(before ^ operand)
                                      & (before ^ result) & 0x80u) != 0));
    cpu->A = result;
    cpu_set_nz(cpu, result);
}

static inline void cpu_sbc(CPUState* cpu, uint8_t operand) {
    const uint8_t before = cpu->A;
    const uint16_t sum = (uint16_t)((uint16_t)before
        + (uint16_t)(uint8_t)~operand + (uint16_t)cpu_has_flag(cpu, CPU_C));
    const uint8_t result = (uint8_t)sum;
    cpu_set_flag(cpu, CPU_C, (uint8_t)(sum > 0xFFu)); /* C=1 means no borrow. */
    cpu_set_flag(cpu, CPU_V, (uint8_t)(((before ^ operand)
                                      & (before ^ result) & 0x80u) != 0));
    cpu->A = result;
    cpu_set_nz(cpu, result);
}

#endif
