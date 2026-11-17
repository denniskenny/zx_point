/*
 * grid.c — World-grid distance helpers
 */

#include "../include/grid.h"
#include "../include/entity_render.h"

uint8_t grid_absdiff(uint8_t a, uint8_t b)
{
    return (a > b) ? (uint8_t)(a - b) : (uint8_t)(b - a);
}

uint8_t grid_dist(uint8_t ax, uint8_t az, uint8_t bx, uint8_t bz)
{
    uint8_t dx = grid_absdiff(ax, bx);
    uint8_t dz = grid_absdiff(az, bz);
    return (dx > dz) ? dx : dz;
}

uint8_t grid_cube_step(int16_t *sub, uint8_t *g, uint8_t g_last, int16_t size)
{
    if (*sub >= size) {
        *sub -= size;
        if (*g < g_last) { (*g)++; return 0; }
        *sub = size - 1;
        return 1;
    }
    if (*sub < 0) {
        *sub += size;
        if (*g > 0) { (*g)--; return 0; }
        *sub = 0;
        return 1;
    }
    return 0;
}

void grid_steer(int8_t *d, uint8_t from, uint8_t to)
{
    if (to > from) *d = 1;
    else if (to < from) *d = -1;
}

/* Grid Z distance -> sprite scale (close = large) */
uint8_t entity_z_to_scale(uint8_t z_dist)
{
    if (z_dist <= 1) return SCALE_32;
    if (z_dist == 2) return SCALE_16;
    if (z_dist == 3) return SCALE_8;
    if (z_dist == 4) return SCALE_4;
    return SCALE_NONE;
}

/* Sub-cube X offset from the player -> screen X of a 32 px sprite box */
uint8_t entity_screen_x(int16_t dx_sub)
{
    int16_t sx = (DIVER_X - 8) - (dx_sub * 3 / 4);
    if (sx < PRED_X_MIN) sx = PRED_X_MIN;
    if (sx > PRED_X_MAX) sx = PRED_X_MAX;
    return (uint8_t)sx & 0xF8;
}
