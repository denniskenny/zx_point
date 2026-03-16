/*
 * hud.c -- HUD display (oxygen bar + health gauge)
 *
 * Row 23 (last character row) displays:
 *   Cols 1-13:  Oxygen bar (cyan on black, depletes right-to-left)
 *   Cols 15-19: Health gauge (red on black, 5 segments)
 *
 * Bar cells use pixel pattern 0x7E (1-pixel border) with ink colour
 * for fill. Attributes control filled vs empty state.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/hud.h"
#include "../include/gfx.h"

/* Attribute values */
#define ATTR_OXY_FULL  0x45   /* bright, paper=black, ink=cyan */
#define ATTR_HP_FULL   0x42   /* bright, paper=black, ink=red */
#define ATTR_BAR_EMPTY 0x00   /* paper=black, ink=black */

/* Bar pixel pattern: 1px border (paper), 6px fill (ink) */
static const uint8_t bar_pattern[8] = {
    0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00
};

/* Oxygen bar layout */
#define OXY_COL_START  1
#define OXY_COL_COUNT  13

/* Health bar layout */
#define HP_COL_START   15
#define HP_COL_COUNT   5

void hud_init(void)
{
    uint8_t c, s;
    uint8_t py_base = HUD_ROW * 8;  /* pixel row 184 */

    /* Write bar pixel patterns into row 23 screen RAM */
    for (c = OXY_COL_START; c < OXY_COL_START + OXY_COL_COUNT; c++) {
        for (s = 0; s < 8; s++) {
            SCREEN[scr_off(c * 8, py_base + s)] = bar_pattern[s];
        }
    }

    for (c = HP_COL_START; c < HP_COL_START + HP_COL_COUNT; c++) {
        for (s = 0; s < 8; s++) {
            SCREEN[scr_off(c * 8, py_base + s)] = bar_pattern[s];
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
            (c < oxy_filled) ? ATTR_OXY_FULL : ATTR_BAR_EMPTY;
    }

    /* Health bar: 1 cell per health point */
    for (c = 0; c < HP_COL_COUNT; c++) {
        row_attr[HP_COL_START + c] =
            (c < health) ? ATTR_HP_FULL : ATTR_BAR_EMPTY;
    }
}
