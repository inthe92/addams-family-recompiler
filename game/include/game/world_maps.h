#ifndef ADDAMS_WORLD_MAPS_H
#define ADDAMS_WORLD_MAPS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct GameWorldColumn {
    uint8_t tiles[24];
    uint8_t palette_slots[24];
} GameWorldColumn;
typedef struct GameWorldMap {
    uint16_t column_count;
    uint8_t layout;
    uint8_t data_bank;
    const GameWorldColumn* columns;
} GameWorldMap;
typedef struct GameWorldEdit {
    uint8_t column;
    uint8_t row;
    uint8_t tile;
    uint8_t flag_index;
} GameWorldEdit;
/* Horizontal base maps decoded at build time from the verified ASM source.
 * Vertical layouts have no columns here; geometry and dynamic edits are separate. */
#ifdef NES_USER_ROM
extern const GameWorldMap* imported_world_maps;
extern const GameWorldEdit* imported_level13_edits;
#define game_world_maps imported_world_maps
#define game_level13_edits imported_level13_edits
#else
extern const GameWorldMap game_world_maps[37];
/* D9DB applies these twenty edits in order, controlled by ten bytes at04B0. */
extern const GameWorldEdit game_level13_edits[20];
#endif
static inline uint8_t game_world_tile(uint8_t level,uint16_t column,uint8_t row,const uint8_t flags[10]) {
    uint8_t tile=game_world_maps[level].columns[column].tiles[row];
    if(level==13) {
        for(unsigned i=0;i<20;++i) {
            const GameWorldEdit* edit=&game_level13_edits[i];
            if(edit->column==column && edit->row==row && flags[edit->flag_index])tile=edit->tile;
        }
    }
    return tile;
}
#ifdef __cplusplus
}
#endif
#endif
