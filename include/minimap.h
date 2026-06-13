#ifndef _MINIMAP_H_
#define _MINIMAP_H_

/* ================================================================== */
/* minimap.h -- 32x32 pixel minimap in bottom-right corner            */
/* ================================================================== */

/* Reset the minimap update timer. Call once at game/level start. */
void minimap_init(void);

/* XOR-draw the initial full depth bar. Call once at game start. */
void depth_indicator_init(void);

/* Draw the minimap: 4x4 grid, player dot (yellow), item dots (red).
 * Updates on cell boundary crossings.
 * Uses XOR writes for dots over the grid.
 * Call after all other rendering (bubblefield, sprites, HUD). */
void minimap_draw(void);

/* Draw the 4-digit depth indicator to the left of the minimap.
 * Call every frame for smooth depth updates. */
void depth_indicator_draw(void);

#endif /* _MINIMAP_H_ */
