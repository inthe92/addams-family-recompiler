#ifndef NES_RECOMP_RECOVERED_RESOURCES_H
#define NES_RECOMP_RECOVERED_RESOURCES_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifdef NES_USER_ROM
extern const uint8_t (*imported_prg_data)[16384];
extern const uint8_t* imported_chr_data;
extern uint16_t imported_reset_vector;
#define game_prg_data imported_prg_data
#define game_chr_data imported_chr_data
#define game_reset_vector imported_reset_vector
#else
extern const uint8_t game_prg_data[8][16384];
extern const uint8_t game_chr_data[131072];
extern const uint16_t game_reset_vector;
extern const char game_source_sha256[65];
#endif
#ifdef __cplusplus
}
#endif
#endif
