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

/* Place treasures for a given level number.
 * At least one archaeological treasure; extras are random. */
void treasure_spawn(uint8_t level_num);

/* Check all uncollected treasures against player position.
 * Marks collected, applies pickup effects (firstaid, oxygen). */
void treasure_check_collection(void);

#endif /* _TREASURE_H_ */
