/*
 * starfield.c — PRNG, star_t, init/update/draw stars
 *
 * Extracted from root starfield.c lines 40-401.
 * Public API:
 *   init_stars()                         — seed all stars
 *   stars_set_count(n)                   — change active star count
 *   stars_erase_all()                    — erase visible stars from screen
 *   update_and_draw_stars(vx, vy, vz)    — one frame update
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/gfx.h"

/* --- Star type --- */
typedef struct {
    int16_t x, y, z;
    uint8_t psx, psy;   /* previous screen coords (psy=255 = not visible) */
    uint8_t pclose;      /* previous frame was 2x2 */
} star_t;

#define PSY_NONE 255

static star_t stars[NUM_STARS];
static uint8_t star_count = NUM_STARS;  /* active stars (can be < NUM_STARS) */

/* ------------------------------------------------------------------ */
/* 16-bit LFSR PRNG                                                    */
/* ------------------------------------------------------------------ */
static uint16_t lfsr = 0xACE1;
static uint16_t weyl = 0;

static uint16_t rng(void)
{
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^
                    (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);
    weyl += 0x9E35;
    return lfsr ^ weyl;
}

/* ------------------------------------------------------------------ */
/* Random coordinate helpers                                           */
/* ------------------------------------------------------------------ */
static int16_t rand_xy(void)
{
    return (int16_t)(rng() & 0xFF) - XY_HALF;
}

static int16_t rand_z(void)
{
    int16_t v = (int16_t)(rng() & 0xFF);
    return v ? v : 1;
}

/* ------------------------------------------------------------------ */
/* Erase a star at its previous screen position                        */
/* ------------------------------------------------------------------ */
static void erase_star(uint8_t *buf, uint8_t px, uint8_t py, uint8_t pclose)
{
    unplot(buf, px, py);
    if (pclose) {
        if (px < 255) unplot(buf, px + 1, py);
        if (py < 191) unplot(buf, px, py + 1);
        if (px < 255 && py < 191) unplot(buf, px + 1, py + 1);
    }
}

/* ------------------------------------------------------------------ */
/* Set the active star count (capped at NUM_STARS)                     */
/* ------------------------------------------------------------------ */
void stars_set_count(uint8_t count)
{
    uint8_t i;

    if (count > NUM_STARS) count = NUM_STARS;

    /* Erase stars that are being deactivated */
    for (i = count; i < star_count; i++) {
        if (stars[i].psy != PSY_NONE)
            erase_star(SCREEN, stars[i].psx, stars[i].psy,
                       stars[i].pclose);
        stars[i].psy = PSY_NONE;
    }

    star_count = count;
}

/* ------------------------------------------------------------------ */
/* Erase all currently visible stars from the screen                   */
/* ------------------------------------------------------------------ */
void stars_erase_all(void)
{
    uint8_t i;
    for (i = 0; i < star_count; i++) {
        if (stars[i].psy != PSY_NONE) {
            erase_star(SCREEN, stars[i].psx, stars[i].psy,
                       stars[i].pclose);
            stars[i].psy = PSY_NONE;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Initialise all active stars to random positions                     */
/* ------------------------------------------------------------------ */
void init_stars(void)
{
    uint8_t i;
    for (i = 0; i < star_count; i++) {
        stars[i].x = rand_xy();
        stars[i].y = rand_xy();
        stars[i].z = rand_z();
        stars[i].psy = PSY_NONE;
        stars[i].pclose = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Update positions and draw all active stars for one frame            */
/* ------------------------------------------------------------------ */
void update_and_draw_stars(int8_t vx, int8_t vy, int8_t vz)
{
    uint8_t i;
    int16_t sx, sy;

    for (i = 0; i < star_count; i++) {

        /* Erase star at its previous screen position */
        if (stars[i].psy != PSY_NONE)
            erase_star(SCREEN, stars[i].psx, stars[i].psy,
                       stars[i].pclose);

        /* Apply continuous velocity */
        stars[i].x += vx;
        stars[i].y += vy;
        stars[i].z += vz;

        /* Respawn star when it passes through Z bounds */
        if (stars[i].z < Z_MIN) {
            stars[i].x = rand_xy();
            stars[i].y = rand_xy();
            stars[i].z = Z_MAX;
        } else if (stars[i].z > Z_MAX) {
            stars[i].x = rand_xy();
            stars[i].y = rand_xy();
            stars[i].z = Z_MIN;
        }

        /* Wrap X and Y so stars re-enter from the opposite edge */
        if (stars[i].x < -XY_HALF)
            stars[i].x += XY_SPAN;
        else if (stars[i].x >= XY_HALF)
            stars[i].x -= XY_SPAN;

        if (stars[i].y < -XY_HALF)
            stars[i].y += XY_SPAN;
        else if (stars[i].y >= XY_HALF)
            stars[i].y -= XY_SPAN;

        /* Perspective projection (all 16-bit safe) */
        sx = (stars[i].x * FOCAL) / stars[i].z + 128;
        sy = (stars[i].y * FOCAL) / stars[i].z + 96;

        /* Clip to visible screen area and draw */
        if (sx >= 0 && sx < 256 && sy >= 0 && sy < 192) {
            plot(SCREEN, (uint8_t)sx, (uint8_t)sy);

            /* Close stars (z < 85) get a 2x2 dot for depth cue */
            stars[i].pclose = (stars[i].z < (Z_MAX / 3)) ? 1 : 0;
            if (stars[i].pclose) {
                if (sx < 255)
                    plot(SCREEN, (uint8_t)(sx + 1), (uint8_t)sy);
                if (sy < 191)
                    plot(SCREEN, (uint8_t)sx, (uint8_t)(sy + 1));
                if (sx < 255 && sy < 191)
                    plot(SCREEN, (uint8_t)(sx + 1), (uint8_t)(sy + 1));
            }
            stars[i].psx = (uint8_t)sx;
            stars[i].psy = (uint8_t)sy;
        } else {
            stars[i].psy = PSY_NONE;
        }
    }
}
