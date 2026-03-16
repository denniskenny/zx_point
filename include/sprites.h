#ifndef _SPRITES_H_
#define _SPRITES_H_

/* ================================================================== */
/* sprites.h -- Direct-draw sprite system + utility functions          */
/* ================================================================== */

#include <stdint.h>

/* --- Initialise sprite system (clear pixel RAM) --- */
void sprites_init(void);

/* --- Draw player sprite at fixed screen centre --- */
void sprites_player_draw(uint8_t frame_idx);

/* --- Set player sprite colour (4 ATTR cells) --- */
void sprites_player_set_colour(uint8_t attr);

/* ================================================================== */
/* Sprite utility functions (operate on raw byte arrays)              */
/* ================================================================== */

/* Generate a 1-pixel-border expansion mask from a graphic.
 * src:    source graphic bytes (row-major, bytes_per_row * height)
 * dst:    destination mask bytes (same size, caller-allocated)
 * bpr:    bytes per row
 * height: rows
 * Mask bits: 0 = opaque (sprite pixel or 1px border), 1 = transparent.
 */
void sprites_gen_mask(const uint8_t *src, uint8_t *dst,
                      uint8_t bpr, uint8_t height);

/* Mirror a graphic horizontally in place.
 * buf:    graphic bytes (row-major)
 * bpr:    bytes per row (must be power of 2 or at least even)
 * height: rows
 */
void sprites_mirror_h(uint8_t *buf, uint8_t bpr, uint8_t height);

/* Generate a shimmer frame: copy src to dst, shifting every second
 * scanline right by 1 pixel.
 * src/dst: graphic bytes (row-major), same dimensions
 * bpr:     bytes per row
 * height:  rows
 */
void sprites_gen_shimmer(const uint8_t *src, uint8_t *dst,
                         uint8_t bpr, uint8_t height);

#endif /* _SPRITES_H_ */
