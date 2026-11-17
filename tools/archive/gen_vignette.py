#!/usr/bin/env python3
"""Generate a ZX Spectrum .scr file with a dithered circular vignette.

The vignette is clear in the centre and dithers to solid at the screen edges.
Only pixel data is meaningful; attributes are zeroed (game sets its own).

Usage: python3 tools/gen_vignette.py [output.scr]
"""

import math
import sys

WIDTH = 256
HEIGHT = 192
SCR_PIXEL_SIZE = 6144
SCR_ATTR_SIZE = 768

# 8x8 Bayer ordered-dither matrix (values 0-63)
BAYER_8x8 = [
    [ 0, 32,  8, 40,  2, 34, 10, 42],
    [48, 16, 56, 24, 50, 18, 58, 26],
    [12, 44,  4, 36, 14, 46,  6, 38],
    [60, 28, 52, 20, 62, 30, 54, 22],
    [ 3, 35, 11, 43,  1, 33,  9, 41],
    [51, 19, 59, 27, 49, 17, 57, 25],
    [15, 47,  7, 39, 13, 45,  5, 37],
    [63, 31, 55, 23, 61, 29, 53, 21],
]


def scr_offset(x, y):
    col = x >> 3
    third = (y >> 6) & 3
    char_row = (y >> 3) & 7
    pixel_row = y & 7
    return (third << 11) | (pixel_row << 8) | (char_row << 5) | col


def generate_vignette(inner=0.55, outer=1.0):
    """Generate vignette pixel data.

    inner: normalised radius where dithering begins (0 = centre, 1 = corner)
    outer: normalised radius where fully solid (1 = corner)
    """
    pixels = bytearray(SCR_PIXEL_SIZE)

    cx = WIDTH / 2.0
    cy = HEIGHT / 2.0
    max_dist = math.sqrt(cx * cx + cy * cy)

    for y in range(HEIGHT):
        for x in range(WIDTH):
            dx = x - cx
            dy = y - cy
            dist = math.sqrt(dx * dx + dy * dy) / max_dist

            if dist <= inner:
                intensity = 0.0
            elif dist >= outer:
                intensity = 1.0
            else:
                t = (dist - inner) / (outer - inner)
                intensity = t * t * (3 - 2 * t)

            threshold = (BAYER_8x8[y & 7][x & 7] + 0.5) / 64.0
            if intensity > threshold:
                off = scr_offset(x, y)
                pixels[off] |= 0x80 >> (x & 7)

    return bytes(pixels) + bytes(SCR_ATTR_SIZE)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "assets/vignette.scr"
    scr = generate_vignette()
    with open(out, "wb") as f:
        f.write(scr)
    print(f"Written {len(scr)} bytes to {out}")


if __name__ == "__main__":
    main()
