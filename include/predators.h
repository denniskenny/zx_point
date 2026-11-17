#ifndef _PREDATORS_H_
#define _PREDATORS_H_

/* ================================================================== */
/* predators.h -- Predator AI, spawning, rendering, collision         */
/* ================================================================== */

#include <stdint.h>
#include "../config/game_config.h"

typedef struct {
    uint8_t gx, gz;        /* grid position (depth implied by type) */
    int8_t  gdx, gdz;      /* grid-space direction (+1 or -1) */
    uint8_t type;           /* PRED_RAY, PRED_SHARK, PRED_GOO */
    uint8_t active;         /* 1 = alive */
    uint8_t visible;        /* 1 = within range at same depth */
    uint8_t anim_ctr;       /* animation frame counter */
    uint8_t proximity_ctr;  /* frames player has been in contact range */
    uint8_t face_left;      /* 1 = draw mirrored (_m) frames, facing left */
    uint16_t grid_move_ctr; /* countdown to next grid move */
} predator_t;

extern predator_t predators[MAX_PREDATORS];
extern uint8_t predator_count;
extern uint8_t pred_ray_count;
extern uint8_t pred_shark_count;
extern uint8_t pred_goo_count;

/* Reset draw tracking (call once at game start) */
void predators_init(void);

/* Place predators for a given level number */
void predators_spawn(uint8_t n_rays, uint8_t n_sharks, uint8_t n_goos);

/* Move all predators: grid traversal + screen bounce */
void predators_update(void);

/* Erase old + draw new predator sprites in one tight pass (call in the
 * draw phase). show=0 hides them all (e.g. during a depth transition). */
void predators_render(uint8_t frame_ctr, uint8_t show);

/* Check player-predator overlap.
 * Returns: 0 = none, 1 = ray/shark damage, 255 = GOO instant death */
uint8_t predators_check_collision(void);

/* Erase all currently drawn predator sprites */
void predators_hide_all(void);

#endif /* _PREDATORS_H_ */
