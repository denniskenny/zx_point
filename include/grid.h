#ifndef _GRID_H_
#define _GRID_H_

/* ================================================================== */
/* grid.h — World-grid distance helpers                               */
/* ================================================================== */

#include <stdint.h>
#include "../config/game_config.h"

/* |a - b| for unsigned grid coordinates */
uint8_t grid_absdiff(uint8_t a, uint8_t b);

/* Chebyshev distance on the X/Z grid: max(|ax - bx|, |az - bz|).
 * "Both axes within R" is the same test as grid_dist(...) <= R. */
uint8_t grid_dist(uint8_t ax, uint8_t az, uint8_t bx, uint8_t bz);

/* Grid coordinate -> minimap pixel offset. Higher gx/gz (west/north)
 * maps further left/up, matching the main-screen orientation. */
#define GRID_TO_MINIMAP(g)  ((uint8_t)(GRID_W - 1 - (g)))

/* Carry sub-cube overflow into the grid cell (*sub in 0..size-1).
 * At the grid edge the position is clamped instead and 1 is returned. */
uint8_t grid_cube_step(int16_t *sub, uint8_t *g, uint8_t g_last, int16_t size);

/* Point direction *d (+1/-1) from `from` toward `to`; unchanged if equal */
void grid_steer(int8_t *d, uint8_t from, uint8_t to);

#endif /* _GRID_H_ */
