#ifndef _SPRITES_H_
#define _SPRITES_H_

/* ================================================================== */
/* sprites.h -- Direct-draw sprite system                             */
/* ================================================================== */

#include <stdint.h>

/* --- Initialise sprite system (clear pixel RAM) --- */
void sprites_init(void);

/* --- Draw player sprite at fixed screen centre --- */
void sprites_player_draw(uint8_t frame_idx);

/* --- Set player sprite colour (4 ATTR cells) --- */
void sprites_player_set_colour(uint8_t attr);

/* --- Get pointer to a diver frame's pixel data --- */
const uint8_t *sprites_get_frame(uint8_t frame_idx);

#endif /* _SPRITES_H_ */
