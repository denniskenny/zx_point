#ifndef _PLAYER_H_
#define _PLAYER_H_

/* ================================================================== */
/* player.h -- Player movement & cube traversal                       */
/* ================================================================== */

#include <stdint.h>
#include "game.h"

/* Global player instance */
extern player_t player;

/* Initialise player at top-centre of grid, full health/oxygen */
void player_init(void);

/* Accumulate velocity into sub-cube position.
 * Handles grid boundary crossings and depth transitions.
 * Returns 1 if depth (gy) changed this frame, 0 otherwise. */
uint8_t player_update(int8_t vx, int8_t vy, int8_t vz);

#endif /* _PLAYER_H_ */
