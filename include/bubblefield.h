#ifndef _BUBBLEFIELD_H_
#define _BUBBLEFIELD_H_

/* ================================================================== */
/* bubblefield.h — 3D bubble particle field                           */
/* ================================================================== */

#include <stdint.h>

/* Seed every bubble at a random position (call at game start) */
void init_bubbles(void);

/* Change the active bubble count (capped at NUM_BUBBLES) */
void bubbles_set_count(uint8_t count);

/* One frame: erase, move by (vx, vy, vz), project and redraw (Z80 asm) */
void update_and_draw_bubbles(int8_t vx, int8_t vy, int8_t vz);

/* Bubbles above sf_cull_y / below sf_cull_y_floor are not drawn
 * (sea line and sea floor). 0 = no culling. Set before each update. */
extern uint8_t sf_cull_y;
extern uint8_t sf_cull_y_floor;

#endif /* _BUBBLEFIELD_H_ */
