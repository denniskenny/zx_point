/*
 * sprites.c -- Direct-draw sprite system + utility functions
 *
 * Player sprite drawn directly to screen RAM using write_sprite().
 * No SP1 dependency.
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

/* ------------------------------------------------------------------ */
/* Draw player sprite at fixed screen centre                           */
/* ------------------------------------------------------------------ */
void sprites_player_draw(uint8_t frame_idx)
{
    const uint8_t *frame_data;

    frame_data = (frame_idx & 1) ? diver_f2 : diver_f1;
    write_sprite(SCREEN, frame_data, DIVER_X, DIVER_Y);
}

/* ------------------------------------------------------------------ */
/* Set player sprite colour: 2x2 ATTR cells at (col=15, row=11)       */
/* ------------------------------------------------------------------ */
void sprites_player_set_colour(uint8_t attr)
{
    set_attr_rect(DIVER_X >> 3, DIVER_Y >> 3, 2, 2, attr);
}

/* ================================================================== */
/* Sprite utility functions                                            */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* 3.2 Mask generation: 1-pixel border expansion                       */
/* ------------------------------------------------------------------ */
void sprites_gen_mask(const uint8_t *src, uint8_t *dst,
                      uint8_t bpr, uint8_t height)
{
    uint8_t r, c;
    uint8_t above, cur, below;
    uint8_t expanded;

    for (r = 0; r < height; r++) {
        for (c = 0; c < bpr; c++) {
            cur = src[r * bpr + c];
            above = (r > 0) ? src[(r - 1) * bpr + c] : 0;
            below = (r < height - 1) ? src[(r + 1) * bpr + c] : 0;

            /* Vertical expansion: OR current row with rows above/below */
            expanded = cur | above | below;

            /* Horizontal expansion: OR with left/right shifted versions */
            expanded |= (cur << 1);
            expanded |= (cur >> 1);

            /* Also shift neighbours for diagonal coverage */
            if (c > 0) {
                /* Bit 0 of previous byte shifted right = carry into MSB */
                expanded |= (src[r * bpr + c - 1] << 7);
                if (r > 0) expanded |= (src[(r - 1) * bpr + c - 1] << 7);
                if (r < height - 1) expanded |= (src[(r + 1) * bpr + c - 1] << 7);
            }
            if (c < bpr - 1) {
                /* Bit 7 of next byte shifted left = carry into LSB */
                expanded |= (src[r * bpr + c + 1] >> 7);
                if (r > 0) expanded |= (src[(r - 1) * bpr + c + 1] >> 7);
                if (r < height - 1) expanded |= (src[(r + 1) * bpr + c + 1] >> 7);
            }

            /* Mask: 0 = opaque (sprite or border), 1 = transparent */
            dst[r * bpr + c] = expanded ^ 0xFF;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 3.3 Horizontal mirroring                                            */
/* ------------------------------------------------------------------ */

/* Reverse bits in a single byte */
static uint8_t reverse_byte(uint8_t b)
{
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

void sprites_mirror_h(uint8_t *buf, uint8_t bpr, uint8_t height)
{
    uint8_t r, i, tmp;
    uint8_t half = bpr / 2;

    for (r = 0; r < height; r++) {
        uint8_t *row = buf + r * bpr;

        /* Reverse each byte's bits */
        for (i = 0; i < bpr; i++)
            row[i] = reverse_byte(row[i]);

        /* Reverse byte order within the row */
        for (i = 0; i < half; i++) {
            tmp = row[i];
            row[i] = row[bpr - 1 - i];
            row[bpr - 1 - i] = tmp;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 3.4 Shimmer effect: shift every second scanline right by 1 pixel    */
/* ------------------------------------------------------------------ */
void sprites_gen_shimmer(const uint8_t *src, uint8_t *dst,
                         uint8_t bpr, uint8_t height)
{
    uint8_t r, c;

    for (r = 0; r < height; r++) {
        if (r & 1) {
            /* Odd scanlines: shift right by 1 pixel */
            for (c = bpr; c > 0; c--) {
                dst[r * bpr + c - 1] = src[r * bpr + c - 1] >> 1;
                if (c > 1)
                    dst[r * bpr + c - 1] |= (src[r * bpr + c - 2] << 7);
            }
        } else {
            /* Even scanlines: copy unchanged */
            for (c = 0; c < bpr; c++)
                dst[r * bpr + c] = src[r * bpr + c];
        }
    }
}
