#ifndef NES_RECOMP_MODULE_ABI_H
#define NES_RECOMP_MODULE_ABI_H
#include <stdint.h>
/* Bump whenever the native service layout or callback contract changes. */
#define NES_RECOMP_ABI_VERSION 3u
typedef uint32_t (*NesRecompAbiVersion)(void);
#endif
