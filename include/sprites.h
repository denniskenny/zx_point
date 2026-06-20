#ifndef _SPRITES_H_
#define _SPRITES_H_

/* ================================================================== */
/* sprites.h -- Direct-draw sprite system                             */
/* ================================================================== */

#include <stdint.h>

extern const uint8_t diver_frame_count;

/* --- Initialise sprite system (clear pixel RAM) --- */
void sprites_init(void);

/* --- Draw player sprite at fixed screen centre --- */
void sprites_player_draw(uint8_t frame_idx);

/* --- Set player sprite colour from diver_attr[] (paper bits 3-5 merged) --- */
void sprites_player_set_colour(uint8_t paper);
void sprites_diver_set_colour_at(uint8_t col, uint8_t row, uint8_t rows, uint8_t paper);

/* --- Get pointer to a diver frame's pixel data --- */
const uint8_t *sprites_get_frame(uint8_t frame_idx);

#endif /* _SPRITES_H_ */
