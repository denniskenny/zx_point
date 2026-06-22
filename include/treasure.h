#ifndef _TREASURE_H_
#define _TREASURE_H_

/* ================================================================== */
/* treasure.h -- Treasure spawning & collection                       */
/* ================================================================== */

#include <stdint.h>
#include "game.h"

/* Global treasure array and level state */
extern treasure_t treasures[MAX_TREASURES];
extern level_t level;

/* Seed the treasure PRNG (call once from title screen). */
void treasure_seed(uint16_t seed);

/* Place treasures for a given level number.
 * At least one archaeological treasure; extras are random. */
void treasure_spawn(uint8_t level_num);

/* Check all uncollected treasures against player position.
 * Marks collected, applies pickup effects (firstaid, oxygen). */
void treasure_check_collection(void);

/* Reset treasure render tracking (call on game init). */
void treasure_init_render(void);

/* XOR-erase treasure sprites from previous frame.
 * Must be called BEFORE seafloor changes to avoid XOR artifacts. */
void treasure_erase(void);

/* Draw visible treasure sprites at current positions.
 * frame_ctr drives shimmer animation (toggle every 8 frames). */
void treasure_draw(uint8_t frame_ctr);

/* Erase all drawn treasure sprites (call on depth change). */
void treasure_hide_all(void);

#endif /* _TREASURE_H_ */
