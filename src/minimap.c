/*
 * minimap.c -- 32x32 pixel minimap in bottom-right corner
 *
 * Shows the full 64x64 horizontal grid (depth not displayed).
 * Player = red 2x2 dot, treasures & predators = white 1x1 dots.
 * Drawn last each frame to overlay the play area.
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

/* Pixel origin of the minimap */
#define MM_PX  ((uint8_t)(MINIMAP_COL * 8))   /* 224 */
#define MM_PY  ((uint8_t)(MINIMAP_ROW * 8))   /* 160 */

void minimap_draw(void)
{
    uint8_t y, i;
    uint8_t px, py;
    uint16_t off;
    uint8_t cell_row, cell_col;

    /* --- Clear minimap screen area (4 bytes per pixel row) --- */
    for (y = 0; y < MINIMAP_SIZE; y++) {
        off = scr_off(MM_PX, MM_PY + y);
        SCREEN[off]     = 0;
        SCREEN[off + 1] = 0;
        SCREEN[off + 2] = 0;
        SCREEN[off + 3] = 0;
    }

    /* --- Set minimap attributes: white ink on black paper --- */
    for (y = 0; y < 4; y++) {
        for (i = 0; i < 4; i++) {
            ATTR[(MINIMAP_ROW + y) * 32 + MINIMAP_COL + i] = 0x07;
        }
    }

    /* --- Draw border --- */
    /* Top and bottom horizontal lines (write full bytes) */
    off = scr_off(MM_PX, MM_PY);
    SCREEN[off] = 0xFF; SCREEN[off+1] = 0xFF;
    SCREEN[off+2] = 0xFF; SCREEN[off+3] = 0xFF;

    off = scr_off(MM_PX, (uint8_t)(MM_PY + MINIMAP_SIZE - 1));
    SCREEN[off] = 0xFF; SCREEN[off+1] = 0xFF;
    SCREEN[off+2] = 0xFF; SCREEN[off+3] = 0xFF;

    /* Left and right vertical lines */
    for (y = 0; y < MINIMAP_SIZE; y++) {
        off = scr_off(MM_PX, MM_PY + y);
        SCREEN[off]     |= 0x80;  /* leftmost pixel (x=224, bit 7) */
        SCREEN[off + 3] |= 0x01;  /* rightmost pixel (x=255, bit 0) */
    }

    /* --- Draw treasure dots (white, 1x1 pixel) --- */
    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        px = MM_PX + (treasures[i].gx >> 1);
        py = MM_PY + (treasures[i].gz >> 1);
        plot(SCREEN, px, py);
    }

    /* --- Draw predator dots (white, 1x1 pixel; skip GOOs) --- */
    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        if (predators[i].type == PRED_GOO) continue;
        px = MM_PX + (predators[i].gx >> 1);
        py = MM_PY + (predators[i].gz >> 1);
        plot(SCREEN, px, py);
    }

    /* --- Draw player dot (red, 2x2 pixels) --- */
    px = MM_PX + (player.gx >> 1);
    py = MM_PY + (player.gz >> 1);
    plot(SCREEN, px, py);
    if (px < 255) plot(SCREEN, px + 1, py);
    if (py < 191) plot(SCREEN, px, py + 1);
    if (px < 255 && py < 191) plot(SCREEN, px + 1, py + 1);

    /* --- Set player dot's character cell to red ink --- */
    cell_col = MINIMAP_COL + ((player.gx >> 1) >> 3);
    cell_row = MINIMAP_ROW + ((player.gz >> 1) >> 3);
    ATTR[cell_row * 32 + cell_col] = 0x02;  /* paper=black, ink=red */
}
