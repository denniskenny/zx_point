/*
 * hud.c -- HUD display (oxygen bar + health gauge)
 *
 * Row 23 (last character row) displays:
 *   Cols 2-14:  Oxygen bar (cyan paper, white ink, depletes right-to-left)
 *   Cols 16-20: Health gauge (cyan paper, red ink, 5 segments)
 *
 * Bar cells use pixel pattern 0x7E (1-pixel border) XOR'd onto the
 * vignette. Attributes control filled vs empty state.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/hud.h"
#include "../include/gfx.h"

/* Gauge attributes: ink and paper swapped */
#define OXY_ATTR    0x47   /* bright black paper, bright white ink */
#define OXY_EMPTY   0x00   /* black paper, black ink */
#define HP_ATTR     0x42   /* bright black paper, bright red ink */
#define HP_EMPTY    0x00   /* black paper, black ink */

/* Bar pixel pattern: 1px border (paper), 6px fill (ink) */
static const uint8_t bar_pattern[8] = {
    0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00
};

/* Oxygen bar layout */
#define OXY_COL_START  2
#define OXY_COL_COUNT  13

/* Health bar layout */
#define HP_COL_START   16
#define HP_COL_COUNT   5

void hud_init(void)
{
    uint8_t c, s;
    uint8_t py_base = HUD_ROW * 8;

    /* XOR bar pixel patterns onto row 23 screen RAM (preserves vignette) */
    for (c = OXY_COL_START; c < OXY_COL_START + OXY_COL_COUNT; c++) {
        for (s = 0; s < 8; s++) {
            SCREEN[scr_off(c * 8, py_base + s)] ^= bar_pattern[s];
        }
    }

    for (c = HP_COL_START; c < HP_COL_START + HP_COL_COUNT; c++) {
        for (s = 0; s < 8; s++) {
            SCREEN[scr_off(c * 8, py_base + s)] ^= bar_pattern[s];
        }
    }
}

void hud_draw(uint8_t oxygen, uint8_t health)
{
    uint8_t c;
    uint8_t oxy_filled;
    uint8_t *row_attr = ATTR + HUD_ROW * 32;

    /* Oxygen bar: map 0-OXYGEN_MAX to 0-OXY_COL_COUNT filled cells */
    oxy_filled = (uint8_t)(((uint16_t)oxygen * OXY_COL_COUNT) / OXYGEN_MAX);

    for (c = 0; c < OXY_COL_COUNT; c++) {
        row_attr[OXY_COL_START + c] =
            (c < oxy_filled) ? OXY_ATTR : OXY_EMPTY;
    }

    /* Health bar: 1 cell per health point */
    for (c = 0; c < HP_COL_COUNT; c++) {
        row_attr[HP_COL_START + c] =
            (c < health) ? HP_ATTR : HP_EMPTY;
    }
}
