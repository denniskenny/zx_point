/*
 * sprites.c -- Direct-draw sprite system + utility functions
 *
 * Player sprite drawn directly to screen RAM using write_sprite().
 * No SP1 dependency.
 */
#pragma disable_warning 110
/*
 */

#include <string.h>
#include <stdint.h>
#include "../config/game_config.h"
#include "../include/sprites.h"
#include "../include/gfx.h"
#include "../include/diver.h"

/* ------------------------------------------------------------------ */
/* Initialise sprite system: clear pixel RAM                           */
/* ------------------------------------------------------------------ */
void sprites_init(void)
{
    memset(SCREEN, 0, PIX_SIZE);
}

/* Pre-computed screen offsets for player position (120, 88..103) */
static const uint16_t player_scr[16] = {
    0x086F, 0x096F, 0x0A6F, 0x0B6F, 0x0C6F, 0x0D6F, 0x0E6F, 0x0F6F,
    0x088F, 0x098F, 0x0A8F, 0x0B8F, 0x0C8F, 0x0D8F, 0x0E8F, 0x0F8F
};

/* ------------------------------------------------------------------ */
/* Draw player sprite at fixed screen centre                           */
/* ------------------------------------------------------------------ */
void sprites_player_draw(uint8_t frame_idx)
{
    const uint8_t *frame_data;
    uint8_t r;

    frame_data = (frame_idx & 1) ? diver_f2 : diver_f1;
    for (r = 0; r < 16; r++) {
        uint16_t off = player_scr[r];
        SCREEN[off]     = *frame_data++;
        SCREEN[off + 1] = *frame_data++;
    }
}

/* ------------------------------------------------------------------ */
/* Get pointer to a diver frame (for drawing at custom positions)      */
/* ------------------------------------------------------------------ */
const uint8_t *sprites_get_frame(uint8_t frame_idx)
{
    return (frame_idx & 1) ? diver_f2 : diver_f1;
}

/* ------------------------------------------------------------------ */
/* Set player sprite colour: 2x2 ATTR cells at (col=15, row=11)       */
/* ------------------------------------------------------------------ */
void sprites_player_set_colour(uint8_t attr)
{
    set_attr_rect(DIVER_X >> 3, DIVER_Y >> 3, 2, 2, attr);
}

