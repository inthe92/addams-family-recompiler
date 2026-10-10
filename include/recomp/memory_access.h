#ifndef NES_RECOMP_MEMORY_ACCESS_H
#define NES_RECOMP_MEMORY_ACCESS_H
#include <stdint.h>

/* Service boundary: translated C never includes a C++/SDL/OpenGL/Lua header. */
typedef struct MemoryAccess {
    void* context;
    uint8_t (*read8)(void* context, uint16_t address);
    void (*write8)(void* context, uint16_t address, uint8_t value);
} MemoryAccess;
#endif
