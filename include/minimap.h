#ifndef _MINIMAP_H_
#define _MINIMAP_H_

/* ================================================================== */
/* minimap.h -- 32x32 pixel minimap in bottom-right corner            */
/* ================================================================== */

/* Reset the minimap update timer. Call once at game/level start. */
void minimap_init(void);

/* Draw the minimap: 4x4 grid, player dot (yellow), item dots (red).
 * Throttled to update once per second (50 frames).
 * Uses XOR writes for dots over the grid.
 * Call after all other rendering (starfield, sprites, HUD). */
void minimap_draw(void);

#endif /* _MINIMAP_H_ */
