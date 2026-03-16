#ifndef _PREDATORS_H_
#define _PREDATORS_H_

/* ================================================================== */
/* predators.h -- Predator AI, spawning, rendering, collision         */
/* ================================================================== */

#include <stdint.h>
#include "../config/game_config.h"

typedef struct {
    uint8_t gx, gz;        /* grid position (depth implied by type) */
    uint8_t sx, sy;        /* screen position (when visible) */
    int8_t  sdx, sdy;      /* screen-space velocity (pixels/frame) */
    int8_t  gdx, gdz;      /* grid-space direction (+1 or -1) */
    uint8_t type;           /* PRED_RAY, PRED_SHARK, PRED_GOO */
    uint8_t active;         /* 1 = alive */
    uint8_t visible;        /* 1 = in player's cube, on screen */
    uint8_t anim_ctr;       /* animation frame counter */
    uint16_t grid_move_ctr; /* countdown to next grid move */
} predator_t;

extern predator_t predators[MAX_PREDATORS];
extern uint8_t predator_count;

/* Reset draw tracking (call once at game start) */
void predators_init(void);

/* Place predators for a given level number */
void predators_spawn(uint8_t level_num);

/* Move all predators: grid traversal + screen bounce */
void predators_update(void);

/* Erase old + draw new visible predators via direct screen writes */
void predators_render(void);

/* Check player-predator overlap.
 * Returns: 0 = none, 1 = ray/shark damage, 255 = GOO instant death */
uint8_t predators_check_collision(void);

/* Erase all currently drawn predator sprites */
void predators_hide_all(void);

#endif /* _PREDATORS_H_ */
