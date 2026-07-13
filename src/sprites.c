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
#include "../include/dzx0.h"
#include "../include/sprites_packed.h"
#include "../include/sprites_blob.h"   /* defines sprites_zx0[] (once) */

const uint8_t diver_frame_count = DIVER_FRAMES;

/* ------------------------------------------------------------------ */
/* Decompress all sprite pixels into the low-RAM arena. Call once at    */
/* boot, before any sprite is drawn (see main.c).                       */
/* ------------------------------------------------------------------ */
void sprites_unpack(void)
{
    dzx0_decompress(sprites_zx0, (uint8_t *)SPRITE_ARENA);
}

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
static const uint8_t * const diver_frames[] = { diver_f1, diver_f2, diver_f3, diver_f4 };

void sprites_player_draw(uint8_t frame_idx)
{
    const uint8_t *frame_data;
    uint8_t r;

    frame_data = diver_frames[frame_idx % DIVER_FRAMES];
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
    return diver_frames[frame_idx % DIVER_FRAMES];
}

/* ------------------------------------------------------------------ */
/* Apply diver colour attrs from diver_attr[] at a given cell position */
/* paper = bits 3-5 to merge with each cell's ink/bright/flash         */
/* ------------------------------------------------------------------ */
void sprites_diver_set_colour_at(uint8_t col, uint8_t row,
                                 uint8_t rows, uint8_t paper)
{
    uint16_t base = (uint16_t)row * 32 + col;
    uint8_t stride = (DIVER_WIDTH >> 3) * DIVER_FRAMES;
    uint8_t a0 = (diver_attr[0] & 0xC7) | paper;
    uint8_t a1 = (diver_attr[1] & 0xC7) | paper;
    uint8_t b0 = (diver_attr[stride] & 0xC7) | paper;
    uint8_t b1 = (diver_attr[stride + 1] & 0xC7) | paper;
    uint8_t r;
    ATTR[base]     = a0;
    ATTR[base + 1] = a1;
    for (r = 1; r < rows; r++) {
        base += 32;
        ATTR[base]     = b0;
        ATTR[base + 1] = b1;
    }
}

void sprites_player_set_colour(uint8_t paper)
{
    sprites_diver_set_colour_at(DIVER_X >> 3, DIVER_Y >> 3, 2, paper);
}

