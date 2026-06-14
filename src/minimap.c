/*
 * minimap.c -- 32x32 pixel minimap in bottom-right corner
 *
 * Shows the full 32x32 horizontal grid at the current depth.
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
#include "../include/depth.h"

/* Pixel origin of the minimap */
#define MM_PX  ((uint8_t)(MINIMAP_COL * 8))   /* 224 */
#define MM_PY  ((uint8_t)(MINIMAP_ROW * 8))   /* 160 */

/* Previous player grid position — used to detect cell crossings */

/* Depth bar: 1 char column to the left of the minimap */
#define DEPTH_COL  (MINIMAP_COL - 1)  /* 27 */
#define DEPTH_ROW  MINIMAP_ROW        /* 20 */
#define DEPTH_BAR_H  MINIMAP_SIZE     /* 32 pixels tall */
#define DEPTH_BAR_MASK  0xFF          /* 8px wide, full character */

static uint8_t mm_prev_gx, mm_prev_gz, mm_prev_gy;

/* Pre-computed screen offsets for depth bar: col 27, y=160..191 */
static const uint16_t depth_scr[32] = {
    0x109B, 0x119B, 0x129B, 0x139B, 0x149B, 0x159B, 0x169B, 0x179B,
    0x10BB, 0x11BB, 0x12BB, 0x13BB, 0x14BB, 0x15BB, 0x16BB, 0x17BB,
    0x10DB, 0x11DB, 0x12DB, 0x13DB, 0x14DB, 0x15DB, 0x16DB, 0x17DB,
    0x10FB, 0x11FB, 0x12FB, 0x13FB, 0x14FB, 0x15FB, 0x16FB, 0x17FB
};
static uint8_t depth_prev_bar_h;

/* XOR a single pixel onto the screen */
static void xor_plot(uint8_t x, uint8_t y)
{
    if (y >= 192) return;
    SCREEN[scr_off(x, y)] ^= (0x80 >> (x & 7));
}

/* Map a grid coordinate (0..31) to minimap pixel offset.
 * Axes are mirrored so O=left, P=right, W=up, S=down on the map. */
static uint8_t grid_to_mm(uint8_t g)
{
    return 31 - g;
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
    uint8_t pcx = player.gx >> 3;
    uint8_t pcz = player.gz >> 3;

    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        if ((predators[i].gx >> 3) == pcx &&
            (predators[i].gz >> 3) == pcz)
            return 1;
    }
    return 0;
}

void depth_indicator_init(void)
{
    uint8_t y;

    for (y = 0; y < 32; y++)
        SCREEN[depth_scr[y]] ^= DEPTH_BAR_MASK;
    depth_prev_bar_h = 32;
}

void minimap_init(void)
{
    mm_prev_gx = 0xFF;  /* force first draw */
    mm_prev_gz = 0xFF;
    mm_prev_gy = 0xFF;
}

void minimap_draw(void)
{
    uint8_t i;
    uint8_t px, py;
    uint8_t cell_row, cell_col;
    uint8_t y;

    /* --- Only update when player crosses a cell boundary --- */
    if (player.gx == mm_prev_gx && player.gz == mm_prev_gz &&
        player.gy == mm_prev_gy)
        return;
    mm_prev_gx = player.gx;
    mm_prev_gz = player.gz;
    mm_prev_gy = player.gy;

    /* --- Draw the 4x4 grid (direct writes, clears area first) --- */
    draw_grid();

    /* --- Set base minimap attributes from the grid asset --- */
    for (y = 0; y < 4; y++) {
        for (i = 0; i < 4; i++) {
            ATTR[(MINIMAP_ROW + y) * 32 + MINIMAP_COL + i] =
                minimap_grid_attr[y * 4 + i];
        }
    }

    /* --- Draw treasure dots (XOR, red, 1x1 pixel, all depths) --- */
    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        px = MM_PX + grid_to_mm(treasures[i].gx);
        py = MM_PY + grid_to_mm(treasures[i].gz);
        xor_plot(px, py);
        /* Set cell attribute to red ink (mirrored) */
        cell_col = MINIMAP_COL + 3 - (treasures[i].gx >> 3);
        cell_row = MINIMAP_ROW + 3 - (treasures[i].gz >> 3);
        ATTR[cell_row * 32 + cell_col] = 0x02;  /* red ink, black paper */
    }

    /* --- Draw predator dots (XOR, green, 1x1, all depths) --- */
    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        px = MM_PX + grid_to_mm(predators[i].gx);
        py = MM_PY + grid_to_mm(predators[i].gz);
        xor_plot(px, py);
        cell_col = MINIMAP_COL + 3 - (predators[i].gx >> 3);
        cell_row = MINIMAP_ROW + 3 - (predators[i].gz >> 3);
        if (predators[i].type == PRED_GOO)
            ATTR[cell_row * 32 + cell_col] = 0x84;  /* green ink + FLASH */
        else
            ATTR[cell_row * 32 + cell_col] = 0x04;  /* green ink, black paper */
    }

    /* --- Draw player dot (XOR, yellow, 2x2 pixels, centred) --- */
    px = MM_PX + grid_to_mm(player.gx);
    py = MM_PY + grid_to_mm(player.gz);
    xor_plot(px, py);
    xor_plot(px + 1, py);
    xor_plot(px, py + 1);
    xor_plot(px + 1, py + 1);

    /* --- Set player cell attribute: yellow, or red+flash if overlap --- */
    cell_col = MINIMAP_COL + 3 - (player.gx >> 3);
    cell_row = MINIMAP_ROW + 3 - (player.gz >> 3);
    if (player_predator_overlap()) {
        ATTR[cell_row * 32 + cell_col] = 0x82;  /* flash + red ink */
    } else {
        ATTR[cell_row * 32 + cell_col] = 0x06;  /* yellow ink, black paper */
    }

}

void depth_indicator_draw(void)
{
    uint16_t depth_val;
    uint8_t bar_h, y, prev_empty, new_empty;
    uint8_t paper_attr;

    depth_val = player.sub_z + (uint16_t)CUBE_SUB_Z * player.gy;
    paper_attr = 0x46;  /* bright black paper, yellow ink */

#define DEPTH_BAR_SCALE (31 * 1024 / (CUBE_SUB_Z * GRID_D))
    if (depth_val >= (uint16_t)CUBE_SUB_Z * GRID_D)
        bar_h = 1;
    else
        bar_h = 32 - (uint8_t)(((uint16_t)depth_val * DEPTH_BAR_SCALE) >> 10);

    prev_empty = 32 - depth_prev_bar_h;

    if (bar_h != depth_prev_bar_h) {
        /* XOR off the old bar (restores background) */
        for (y = prev_empty; y < 32; y++)
            SCREEN[depth_scr[y]] ^= DEPTH_BAR_MASK;
        /* XOR on the new bar */
        new_empty = 32 - bar_h;
        for (y = new_empty; y < 32; y++)
            SCREEN[depth_scr[y]] ^= DEPTH_BAR_MASK;
        depth_prev_bar_h = bar_h;
    }

    for (y = 0; y < 4; y++)
        ATTR[(DEPTH_ROW + y) * 32 + DEPTH_COL] = paper_attr;
}
