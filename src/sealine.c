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

const int8_t sine_tab[SINE_LEN] = {
     0,  1,  2,  3,  4,  5,  5,  6,
     6,  6,  5,  5,  4,  3,  2,  1,
     0, -1, -2, -3, -4, -5, -5, -6,
    -6, -6, -5, -5, -4, -3, -2, -1
};

/* --- Sea line state --- */
static uint8_t sl_phase;        /* animation phase (0..SINE_LEN-1) */
static uint8_t *sl_prev[32];    /* screen byte XORed per 8-pixel column; NULL = none */
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
        sl_prev[i] = 0;
}

void sealine_erase(void)
{
    uint8_t col;
    if (!sl_drawn) return;

    for (col = 0; col < 32; col++) {
        if (sl_prev[col]) *sl_prev[col] ^= 0xFF;
        sl_prev[col] = 0;
    }
    sl_drawn = 0;
}

/* Screen row start for base_y-6 .. base_y+6 (0 = off screen); set per frame */
static uint8_t *sl_row[13];

/* Per-column erase + draw, in asm (the C loop cost ~1000 T per column).
 * For each column: addr = sl_row[6 + sine_tab[(col + sl_phase) & 31]] + col
 * (0 if the row is off screen). If it differs from sl_prev[col], XOR the
 * old byte off, XOR the new one on, and remember it. */
static void sealine_cols(void) __naked
{
    __asm
    push ix
    ld  ix, _sl_prev            ; IX -> sl_prev[col] (2 bytes per column)
    ld  c, 0                    ; C = col
_slc_loop:
    ;; A = 6 + sine_tab[(col + phase) & 31]
    ld  a, (_sl_phase)
    add a, c
    and #31
    ld  e, a
    ld  d, #0
    ld  hl, _sine_tab
    add hl, de
    ld  a, (hl)
    add a, #6                   ; row index 0..12
    ;; HL = sl_row[A]
    add a, a
    ld  e, a
    ld  hl, _sl_row
    add hl, de
    ld  e, (hl)
    inc hl
    ld  d, (hl)                 ; DE = row start (0 = off screen)
    ld  a, d
    or  a
    jr  z, _slc_have            ; off screen: DE = 0
    ld  a, e
    add a, c
    ld  e, a                    ; + col (stays within the 32-byte row)
_slc_have:
    ;; compare with previous address
    ld  l, 0 (ix)
    ld  h, 1 (ix)
    ld  a, l
    cp  e
    jr  nz, _slc_change
    ld  a, h
    cp  d
    jr  z, _slc_next            ; unchanged
_slc_change:
    ld  a, h                    ; erase old (if any)
    or  a
    jr  z, _slc_noold
    ld  a, (hl)
    cpl
    ld  (hl), a
_slc_noold:
    ld  a, d                    ; draw new (if any)
    or  a
    jr  z, _slc_nonew
    ld  a, (de)
    cpl
    ld  (de), a
_slc_nonew:
    ld  0 (ix), e
    ld  1 (ix), d
_slc_next:
    inc ix
    inc ix
    inc c
    ld  a, c
    cp  #32
    jr  nz, _slc_loop
    pop ix
    ret
    __endasm;
}

void sealine_update(uint8_t base_y)
{
    uint8_t i;
    int16_t py;

    sl_base_y = base_y;

    /* If the whole sinewave is off screen, just erase and bail */
    if (base_y > 198 || base_y < 6) {
        sealine_erase();
        sl_visible = 0;
        return;
    }

    sl_visible = 1;

    /* The wave only spans base_y +-6: work out those 13 row addresses once */
    for (i = 0; i < 13; i++) {
        py = (int16_t)base_y - 6 + i;
        sl_row[i] = (py >= 0 && py < 192) ? SCREEN + scr_off(0, (uint8_t)py) : 0;
    }

    sealine_cols();

    sl_drawn = 1;
    sl_phase = (sl_phase + 1) & (SINE_LEN - 1);
}

uint8_t sealine_get_clamp_y(void)
{
    if (!sl_visible) return 0;
    return ((sl_base_y + 6) | 7) + 1;
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
