/*
 * sealine.c — Sea line (sinewave) and sea floor renderers
 *
 * The sea line appears when the player is in Depth 1. It's a sinewave
 * drawn pixel-by-pixel across the screen width. The phase advances each
 * frame to create an undulating effect. The base_y scrolls off-screen
 * as the player descends.
 *
 * The sea floor is a flat horizontal line visible in Depth 3.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/gfx.h"
#include "../include/sealine.h"

/* --- Sine table: 32 entries, amplitude ±6 pixels --- */
#define SINE_LEN   32
#define SINE_SHIFT 3    /* divide x by 8 to index table (256/32=8) */

static const int8_t sine_tab[SINE_LEN] = {
     0,  1,  2,  3,  4,  5,  5,  6,
     6,  6,  5,  5,  4,  3,  2,  1,
     0, -1, -2, -3, -4, -5, -5, -6,
    -6, -6, -5, -5, -4, -3, -2, -1
};

/* --- Sea line state --- */
static uint8_t sl_phase;        /* animation phase (0..SINE_LEN-1) */
static uint8_t sl_prev_y[32];   /* previous y per 8-pixel column (for erase) */
static uint8_t sl_base_y;       /* current base y */
static uint8_t sl_visible;      /* 1 if line is on screen */
static uint8_t sl_drawn;        /* 1 if pixels are on screen to erase */

void sealine_init(void)
{
    uint8_t i;
    sl_phase = 0;
    sl_base_y = SEALINE_DEFAULT_Y;
    sl_visible = 0;
    sl_drawn = 0;
    for (i = 0; i < 32; i++)
        sl_prev_y[i] = 255;
}

void sealine_erase(void)
{
    uint8_t col;
    if (!sl_drawn) return;

    for (col = 0; col < 32; col++) {
        if (sl_prev_y[col] < 192)
            SCREEN[scr_off(col << 3, sl_prev_y[col])] ^= 0xFF;
        sl_prev_y[col] = 255;
    }
    sl_drawn = 0;
}

void sealine_update(uint8_t base_y)
{
    uint8_t col, idx;
    int16_t py;

    sl_base_y = base_y;

    /* If the whole sinewave is off screen, just erase and bail */
    if (base_y > 198 || base_y < 6) {
        sealine_erase();
        sl_visible = 0;
        return;
    }

    sl_visible = 1;

    /* Erase previous frame, draw new sinewave */
    for (col = 0; col < 32; col++) {
        if (sl_prev_y[col] < 192)
            SCREEN[scr_off(col << 3, sl_prev_y[col])] ^= 0xFF;

        idx = (col + sl_phase) & (SINE_LEN - 1);
        py = (int16_t)base_y + sine_tab[idx];

        if (py >= 0 && py < 192) {
            SCREEN[scr_off(col << 3, (uint8_t)py)] ^= 0xFF;
            sl_prev_y[col] = (uint8_t)py;
        } else {
            sl_prev_y[col] = 255;
        }
    }

    sl_drawn = 1;
    sl_phase = (sl_phase + 1) & (SINE_LEN - 1);
}

uint8_t sealine_get_cull_y(void)
{
    /* Bubbles above this y should be culled.
     * Use base_y minus amplitude as the cull line (conservative). */
    if (!sl_visible) return 0;
    if (sl_base_y < 7) return 0;
    return sl_base_y - 7;
}

uint8_t sealine_get_clamp_y(void)
{
    if (!sl_visible) return 0;
    return ((sl_base_y + 6) | 7) + 1;
}

uint8_t sealine_is_visible(void)
{
    return sl_visible;
}

/* ================================================================== */
/* Sea floor                                                           */
/* ================================================================== */

static uint8_t sf_y;        /* current top of solid surface */
static uint8_t sf_visible;
static uint8_t sf_drawn;

void seafloor_init(void)
{
    sf_y = VIEW_H;
    sf_visible = 0;
    sf_drawn = 0;
}

static void seafloor_xor_row(uint8_t y)
{
    uint16_t off = scr_off(0, y);
    uint8_t col;
    for (col = 0; col < 32; col++)
        SCREEN[off + col] ^= 0xFF;
}

void seafloor_erase(void)
{
    uint8_t y;
    if (!sf_drawn) return;
    for (y = sf_y; y < VIEW_H; y++)
        seafloor_xor_row(y);
    sf_y = VIEW_H;
    sf_drawn = 0;
}

void seafloor_update(uint8_t y)
{
    if (y >= VIEW_H) {
        seafloor_erase();
        sf_visible = 0;
        return;
    }

    sf_visible = 1;

    if (!sf_drawn) {
        seafloor_xor_row(y);
        sf_y = y;
        sf_drawn = 1;
        return;
    }

    if (y < sf_y) {
        /* Descending: floor rises — XOR new rows on */
        uint8_t row;
        for (row = y; row < sf_y; row++)
            seafloor_xor_row(row);
        sf_y = y;
    } else if (y > sf_y) {
        /* Ascending: floor drops — XOR exposed rows off */
        uint8_t row;
        for (row = sf_y; row < y; row++)
            seafloor_xor_row(row);
        sf_y = y;
    }
}

uint8_t seafloor_get_y(void)
{
    return sf_y;
}

uint8_t seafloor_is_visible(void)
{
    return sf_visible;
}
