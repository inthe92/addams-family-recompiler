#ifndef NES_RECOMP_RUNTIME_SERVICES_H
#define NES_RECOMP_RUNTIME_SERVICES_H
#include "recomp/cpu_state.h"
#include "recomp/module_abi.h"
#include <stdint.h>

/* Native blocks call these C-ABI services; no instruction decoder is involved. */
typedef struct RuntimeServices {
    void* context;
    /* Offsets are relative to the instruction start; neither read nor write
       commits CPU cycles. advance(total) commits once at completion. */
    uint8_t (*read8)(void*, uint16_t, uint8_t cycle_offset);
    void (*write8)(void*, uint16_t, uint8_t, uint8_t cycle_offset);
    uint8_t (*bank_for_pc)(void*, uint16_t);
    void (*advance)(void*, uint8_t cycles);
    /* Return 1 after interrupt/mod delivery or a requested scheduler yield. */
    uint8_t (*boundary)(void*, CPUState*, uint8_t irq_mask_at_poll);
} RuntimeServices;

static inline uint8_t recomp_read8(const RuntimeServices* s, uint16_t address, uint8_t offset) {
    return s->read8(s->context, address, offset);
}
static inline void recomp_write8(const RuntimeServices* s, uint16_t address,
                                 uint8_t value, uint8_t offset) {
    s->write8(s->context, address, value, offset);
}
static inline uint16_t recomp_read16(const RuntimeServices* s, uint16_t address, uint8_t offset) {
    const uint8_t low = recomp_read8(s, address, offset);
    const uint8_t high = recomp_read8(s, (uint16_t)(address + 1u), (uint8_t)(offset + 1u));
    return (uint16_t)((uint16_t)low | ((uint16_t)high << 8));
}
static inline uint16_t recomp_read16_zp(const RuntimeServices* s, uint8_t address, uint8_t offset) {
    const uint8_t low = recomp_read8(s, address, offset);
    const uint8_t high = recomp_read8(s, (uint8_t)(address + 1u), (uint8_t)(offset + 1u));
    return (uint16_t)((uint16_t)low | ((uint16_t)high << 8));
}
static inline void recomp_push(CPUState* cpu, const RuntimeServices* s,
                               uint8_t value, uint8_t offset) {
    recomp_write8(s, (uint16_t)(0x0100u | cpu->SP), value, offset);
    cpu->SP = (uint8_t)(cpu->SP - 1u);
}
static inline uint8_t recomp_pull(CPUState* cpu, const RuntimeServices* s, uint8_t offset) {
    cpu->SP = (uint8_t)(cpu->SP + 1u);
    return recomp_read8(s, (uint16_t)(0x0100u | cpu->SP), offset);
}
static inline void recomp_compare(CPUState* cpu, uint8_t left, uint8_t right) {
    cpu_set_flag(cpu, CPU_C, (uint8_t)(left >= right));
    cpu_set_nz(cpu, (uint8_t)(left - right));
}
static inline uint8_t recomp_finish_polled(CPUState* cpu, const RuntimeServices* s,
                                    uint8_t bank, uint16_t next_pc,
                                    uint8_t cycles, uint32_t* remaining, uint8_t irq_mask) {
    cpu->PC = next_pc;
    if (s->advance) s->advance(s->context, cycles);
    --*remaining;
    if (s->boundary && s->boundary(s->context, cpu, irq_mask)) return 1;
    return (uint8_t)(*remaining == 0 || s->bank_for_pc(s->context, cpu->PC) != bank);
}
static inline uint8_t recomp_finish(CPUState* cpu, const RuntimeServices* s,
                                    uint8_t bank, uint16_t next_pc,
                                    uint8_t cycles, uint32_t* remaining) {
    return recomp_finish_polled(cpu, s, bank, next_pc, cycles, remaining,
                                (uint8_t)(cpu->P & CPU_I));
}
#endif
