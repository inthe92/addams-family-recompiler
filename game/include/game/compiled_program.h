#ifndef NES_RECOMP_COMPILED_PROGRAM_H
#define NES_RECOMP_COMPILED_PROGRAM_H
#include "recomp/runtime_services.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Execute one statically translated basic block/scheduler slice.
   1: progressed/yielded; 0: address not represented in the recovered source.
   No interpreter fallback is provided for a missing compiled address. */
#ifdef NES_USER_ROM
extern uint8_t (*imported_execute_slice)(CPUState*, const RuntimeServices*, uint32_t*);
extern uint8_t (*imported_has_address)(uint8_t, uint16_t);
#define game_execute_slice imported_execute_slice
#define game_has_compiled_address imported_has_address
#else
uint8_t game_execute_slice(CPUState*, const RuntimeServices*, uint32_t* remaining);
uint8_t game_has_compiled_address(uint8_t physical_bank, uint16_t pc);
#endif
#ifdef __cplusplus
}
#endif
#endif
