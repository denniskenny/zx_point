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

/* Erase previous frame + draw visible treasure sprites.
 * frame_ctr drives shimmer animation (toggle every 8 frames). */
void treasure_render(uint8_t frame_ctr);

/* Erase all drawn treasure sprites (call on depth change). */
void treasure_hide_all(void);

/* Chebyshev distance to nearest uncollected treasure, or 255 if none. */
uint8_t treasure_nearest_distance(void);

#endif /* _TREASURE_H_ */
