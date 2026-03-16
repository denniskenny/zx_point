#ifndef _MINIMAP_H_
#define _MINIMAP_H_

/* ================================================================== */
/* minimap.h -- 32x32 pixel minimap in bottom-right corner            */
/* ================================================================== */

/* Draw the minimap: border, player dot (red), treasure dots (white).
 * Clears the minimap screen area and redraws each frame.
 * Call after all other rendering (starfield, SP1, HUD). */
void minimap_draw(void);

#endif /* _MINIMAP_H_ */
