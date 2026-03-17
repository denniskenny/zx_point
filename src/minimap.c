/*
 * minimap.c -- 32x32 pixel minimap in bottom-right corner
 *
 * Shows the full 64x64 horizontal grid at the current depth.
 * Player = yellow 2x2 dot (centred in grid cell).
 * Treasures & predators = red 1x1 dots (centred in grid cell).
 * Drawn last each frame to overlay the play area using XOR writes for dots.
 * Updates once per second (50 frames).
 *
 * Screen position: char rows 20-23, cols 28-31
 *                  pixel coords (224, 160) to (255, 191)
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/minimap.h"
#include "../include/gfx.h"
#include "../include/player.h"
#include "../include/treasure.h"
#include "../include/predators.h"
#include "../include/minimap_grid.h"

/* Pixel origin of the minimap */
#define MM_PX  ((uint8_t)(MINIMAP_COL * 8))   /* 224 */
#define MM_PY  ((uint8_t)(MINIMAP_ROW * 8))   /* 160 */

/* Update throttle: 50 frames = 1 second at 50fps */
#define MM_UPDATE_INTERVAL  50

static uint8_t mm_timer;

/* XOR a single pixel onto the screen */
static void xor_plot(uint8_t x, uint8_t y)
{
    if (y >= 192) return;
    SCREEN[scr_off(x, y)] ^= (0x80 >> (x & 7));
}

/* Map a grid coordinate (0..63) to minimap pixel offset.
 * 4 cells of 8px each; returns centre of the cell (offset 4). */
static uint8_t grid_to_mm(uint8_t g)
{
    return (uint8_t)(((g >> 4) << 3) + 4);
}

/* Blit the minimap grid image from the asset header.
 * Overwrites the minimap area each frame, providing a clean
 * slate for XOR dots and restoring the grid lines. */
static void draw_grid(void)
{
    uint8_t y;
    uint16_t off;
    const uint8_t *src = minimap_grid_f1;

    for (y = 0; y < MINIMAP_SIZE; y++) {
        off = scr_off(MM_PX, MM_PY + y);
        SCREEN[off]     = *src++;
        SCREEN[off + 1] = *src++;
        SCREEN[off + 2] = *src++;
        SCREEN[off + 3] = *src++;
    }
}

/* Check if player shares a minimap grid cell with any visible predator */
static uint8_t player_predator_overlap(void)
{
    uint8_t i;
    uint8_t pcx = player.gx >> 4;
    uint8_t pcz = player.gz >> 4;

    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        if (predators[i].type == PRED_GOO) continue;
        /* Depth filter: rays at depth 0, sharks at depth 1 */
        if (predators[i].type == PRED_RAY   && player.gy != 0) continue;
        if (predators[i].type == PRED_SHARK && player.gy != 1) continue;
        if ((predators[i].gx >> 4) == pcx &&
            (predators[i].gz >> 4) == pcz)
            return 1;
    }
    return 0;
}

void minimap_init(void)
{
    mm_timer = 0;
}

void minimap_draw(void)
{
    uint8_t i;
    uint8_t px, py;
    uint8_t cell_row, cell_col;
    uint8_t y;

    /* --- Throttle: only update once per second --- */
    if (mm_timer > 0) {
        mm_timer--;
        return;
    }
    mm_timer = MM_UPDATE_INTERVAL;

    /* --- Draw the 4x4 grid (direct writes, clears area first) --- */
    draw_grid();

    /* --- Set base minimap attributes from the grid asset --- */
    for (y = 0; y < 4; y++) {
        for (i = 0; i < 4; i++) {
            ATTR[(MINIMAP_ROW + y) * 32 + MINIMAP_COL + i] =
                minimap_grid_attr[y * 4 + i];
        }
    }

    /* --- Draw treasure dots (XOR, red, 1x1 pixel, centred in cell) --- */
    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        if (treasures[i].gy != player.gy) continue;  /* depth filter */
        px = MM_PX + grid_to_mm(treasures[i].gx);
        py = MM_PY + grid_to_mm(treasures[i].gz);
        xor_plot(px, py);
        /* Set cell attribute to red ink */
        cell_col = MINIMAP_COL + (treasures[i].gx >> 4);
        cell_row = MINIMAP_ROW + (treasures[i].gz >> 4);
        ATTR[cell_row * 32 + cell_col] = 0x02;  /* red ink, black paper */
    }

    /* --- Draw predator dots (XOR, red, 1x1; skip GOOs) --- */
    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        if (predators[i].type == PRED_GOO) continue;
        /* Depth filter: rays at depth 0, sharks at depth 1 */
        if (predators[i].type == PRED_RAY   && player.gy != 0) continue;
        if (predators[i].type == PRED_SHARK && player.gy != 1) continue;
        px = MM_PX + grid_to_mm(predators[i].gx);
        py = MM_PY + grid_to_mm(predators[i].gz);
        xor_plot(px, py);
        /* Set cell attribute to red ink */
        cell_col = MINIMAP_COL + (predators[i].gx >> 4);
        cell_row = MINIMAP_ROW + (predators[i].gz >> 4);
        ATTR[cell_row * 32 + cell_col] = 0x02;  /* red ink, black paper */
    }

    /* --- Draw player dot (XOR, yellow, 2x2 pixels, centred) --- */
    px = MM_PX + grid_to_mm(player.gx);
    py = MM_PY + grid_to_mm(player.gz);
    xor_plot(px, py);
    xor_plot(px + 1, py);
    xor_plot(px, py + 1);
    xor_plot(px + 1, py + 1);

    /* --- Set player cell attribute: yellow, or red+flash if overlap --- */
    cell_col = MINIMAP_COL + (player.gx >> 4);
    cell_row = MINIMAP_ROW + (player.gz >> 4);
    if (player_predator_overlap()) {
        ATTR[cell_row * 32 + cell_col] = 0x82;  /* flash + red ink */
    } else {
        ATTR[cell_row * 32 + cell_col] = 0x06;  /* yellow ink, black paper */
    }
}
