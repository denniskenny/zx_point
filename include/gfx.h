#ifndef _GFX_H_
#define _GFX_H_

/* ================================================================== */
/* gfx.h — Low-level graphics helpers                                 */
/* ================================================================== */

#include <stdint.h>

/* Convert (x,y) pixel coords to screen-RAM byte offset */
uint16_t scr_off(uint8_t x, uint8_t y);

/* Set a single pixel */
void plot(uint8_t *buf, uint8_t x, uint8_t y);

/* Clear a single pixel */
void unplot(uint8_t *buf, uint8_t x, uint8_t y);

/* Write a byte-aligned 16x16 sprite (overwrites, no flicker) */
void write_sprite(uint8_t *buf, const uint8_t *spr,
                  uint8_t x, uint8_t y);

/* Write a byte-aligned 32x32 sprite (overwrites) */
void write_sprite_32(uint8_t *buf, const uint8_t *spr,
                     uint8_t x, uint8_t y);

/* Erase a 32x32 area (zero bytes) */
void erase_sprite_32(uint8_t *buf, uint8_t x, uint8_t y);

/* Set a rectangle of attribute cells */
void set_attr_rect(uint8_t col, uint8_t row, uint8_t w, uint8_t h,
                   uint8_t attr);

#endif /* _GFX_H_ */
