/*
 * gfx.c — Low-level ZX Spectrum graphics helpers
 *
 * Extracted from starfield.c lines 113-160.
 */

#include "../include/gfx.h"

uint16_t scr_off(uint8_t x, uint8_t y)
{
    return ((uint16_t)(y & 0xC0) << 5) |
           ((uint16_t)(y & 0x07) << 8) |
           ((uint16_t)(y & 0x38) << 2) |
           (x >> 3);
}

void plot(uint8_t *buf, uint8_t x, uint8_t y)
{
    if (y >= 192) return;
    buf[scr_off(x, y)] |= (0x80 >> (x & 7));
}

void unplot(uint8_t *buf, uint8_t x, uint8_t y)
{
    if (y >= 192) return;
    buf[scr_off(x, y)] &= ~(0x80 >> (x & 7));
}

void write_sprite(uint8_t *buf, const uint8_t *spr,
                  uint8_t x, uint8_t y)
{
    uint8_t row, py;
    uint16_t off;

    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py >= 192) continue;
        off = scr_off(x, py);
        buf[off]     = spr[row * 2];
        buf[off + 1] = spr[row * 2 + 1];
    }
}

void write_sprite_32(uint8_t *buf, const uint8_t *spr,
                     uint8_t x, uint8_t y)
{
    uint8_t row, py;
    uint16_t off;

    for (row = 0; row < 32; row++) {
        py = y + row;
        if (py >= 192) continue;
        off = scr_off(x, py);
        buf[off]     = spr[row * 4];
        buf[off + 1] = spr[row * 4 + 1];
        buf[off + 2] = spr[row * 4 + 2];
        buf[off + 3] = spr[row * 4 + 3];
    }
}

void erase_sprite_32(uint8_t *buf, uint8_t x, uint8_t y)
{
    uint8_t row, py;
    uint16_t off;

    for (row = 0; row < 32; row++) {
        py = y + row;
        if (py >= 192) continue;
        off = scr_off(x, py);
        buf[off]     = 0;
        buf[off + 1] = 0;
        buf[off + 2] = 0;
        buf[off + 3] = 0;
    }
}

void set_attr_rect(uint8_t col, uint8_t row, uint8_t w, uint8_t h,
                   uint8_t attr)
{
    uint8_t r, c;
    uint8_t *base = (uint8_t *)0x5800;

    for (r = 0; r < h; r++) {
        if (row + r >= 24) break;
        for (c = 0; c < w; c++) {
            if (col + c >= 32) break;
            base[(row + r) * 32 + col + c] = attr;
        }
    }
}
