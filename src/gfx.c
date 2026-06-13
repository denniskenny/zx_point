/*
 * gfx.c — Low-level ZX Spectrum graphics helpers
 *
 * Extracted from bubblefield.c.
 */

#include "../config/game_config.h"
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
    buf[scr_off(x, y)] ^= (0x80 >> (x & 7));
}

void unplot(uint8_t *buf, uint8_t x, uint8_t y)
{
    if (y >= 192) return;
    buf[scr_off(x, y)] ^= (0x80 >> (x & 7));
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

void xor_sprite_32(uint8_t *buf, const uint8_t *spr,
                   uint8_t x, uint8_t y)
{
    uint8_t row, py;
    uint16_t off;

    for (row = 0; row < 32; row++) {
        py = y + row;
        if (py >= 192) continue;
        off = scr_off(x, py);
        buf[off]     ^= spr[row * 4];
        buf[off + 1] ^= spr[row * 4 + 1];
        buf[off + 2] ^= spr[row * 4 + 2];
        buf[off + 3] ^= spr[row * 4 + 3];
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

/* --- Fast XOR 32x32 sprite + 4x4 attr rect (Z80 asm) --- */
uint8_t xor32_x, xor32_y, xor32_attr;
const uint8_t *xor32_spr;

void xor_sprite_32_fast(void) __naked
{
    (void)xor32_x; (void)xor32_y; (void)xor32_attr; (void)xor32_spr;
    __asm

    push    bc
    push    de

    ;; DE = sprite data pointer
    ld      de, (_xor32_spr)

    ;; C = column (x >> 3)
    ld      a, (_xor32_x)
    rrca
    rrca
    rrca
    and     #0x1F
    ld      c, a

    ;; Compute initial screen address from y
    ld      a, (_xor32_y)
    ld      b, a                    ; save y in B

    and     #0x07
    or      #0x40
    ld      h, a                    ; H partial = 010 00 SSS

    ld      a, b
    and     #0xC0
    rrca
    rrca
    rrca
    or      h
    ld      h, a                    ; H = 010 TT SSS

    ld      a, b
    and     #0x38
    rlca
    rlca
    or      c
    ld      l, a                    ; L = RRR CCCCC

    ;; B = row counter (32 pixel rows)
    ld      b, #32

_x32f_row:
    ld      a, (de)
    xor     (hl)
    ld      (hl), a
    inc     l
    inc     de

    ld      a, (de)
    xor     (hl)
    ld      (hl), a
    inc     l
    inc     de

    ld      a, (de)
    xor     (hl)
    ld      (hl), a
    inc     l
    inc     de

    ld      a, (de)
    xor     (hl)
    ld      (hl), a
    inc     de

    ;; Restore L to start column
    dec     l
    dec     l
    dec     l

    ;; Advance HL to next pixel row
    inc     h
    ld      a, h
    and     #0x07
    jr      nz, _x32f_nxt
    ;; Crossed character cell boundary
    ld      a, l
    add     a, #32
    ld      l, a
    jr      c, _x32f_nxt            ; third boundary — H already correct
    ld      a, h
    sub     #8
    ld      h, a
_x32f_nxt:
    djnz    _x32f_row

    ;; --- Set 4x4 attribute rect ---
    ;; attr address = 0x5800 + (y >> 3) * 32 + column
    ld      a, (_xor32_y)
    rrca
    rrca
    rrca
    and     #0x1F                   ; A = char_row
    ld      l, a
    ld      h, #0
    add     hl, hl                  ; *2
    add     hl, hl                  ; *4
    add     hl, hl                  ; *8
    add     hl, hl                  ; *16
    add     hl, hl                  ; *32
    ld      a, l
    or      c
    ld      l, a                    ; L += column
    ld      a, h
    add     a, #0x58
    ld      h, a                    ; HL = attr address

    ld      a, (_xor32_attr)
    ld      b, #4
_x32f_arow:
    ld      (hl), a
    inc     l
    ld      (hl), a
    inc     l
    ld      (hl), a
    inc     l
    ld      (hl), a
    ;; Advance to next attr row: L += 29 (= +32 - 3)
    ld      e, a
    ld      a, l
    add     a, #29
    ld      l, a
    jr      nc, _x32f_anxt
    inc     h
_x32f_anxt:
    ld      a, e
    djnz    _x32f_arow

    pop     de
    pop     bc
    ret

    __endasm;
}

/* ROM character set: 96 chars (space..copyright), 8 bytes each */
#define ROM_FONT ((const uint8_t *)0x3D00)

void print_at(uint8_t col, uint8_t row, const char *s)
{
    uint8_t px, py, i;
    uint16_t off;
    const uint8_t *glyph;

    px = col << 3;
    py = row << 3;

    while (*s) {
        if (col >= 32) break;
        glyph = ROM_FONT + (((uint8_t)*s - 32) << 3);
        off = scr_off(px, py);
        for (i = 0; i < 8; i++) {
            SCREEN[off] = glyph[i];
            off += 256;  /* next pixel row within char cell */
        }
        s++;
        col++;
        px += 8;
    }
}
