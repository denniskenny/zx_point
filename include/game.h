#ifndef _GAME_H_
#define _GAME_H_

/* ================================================================== */
/* game.h -- Player, treasure, level structs                          */
/* ================================================================== */

#include <stdint.h>
#include "../config/game_config.h"

typedef struct {
    uint8_t gx, gy, gz;           /* grid position (gy=depth 0-2) */
    int16_t sub_x, sub_y, sub_z;  /* sub-cube position */
    uint8_t health;
    uint8_t oxygen;
    uint8_t invuln_timer;          /* frames of invulnerability remaining */
} player_t;

typedef struct {
    uint8_t gx, gy, gz;           /* grid position */
    uint8_t type;                  /* treasure type id */
    uint8_t collected;             /* 0 = on map, 1 = picked up */
} treasure_t;

typedef struct {
    uint8_t number;                /* current level (1-based) */
    uint8_t treasure_count;        /* how many treasures this level */
    uint8_t collected_count;       /* how many collected so far */
    uint16_t time_remaining;       /* frames left */
} level_t;

#endif /* _GAME_H_ */
