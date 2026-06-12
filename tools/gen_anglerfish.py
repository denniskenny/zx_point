#!/usr/bin/env python3
"""Generate 6 ZX Spectrum .scr files of an anglerfish face approaching.

Frames 1-4: gradually appearing from darkness (increasing dither reveal)
Frame 5: full detail, mouth open
Frame 6: full detail, mouth closed (swallowing)

The fish is ~2/3 screen size, facing the viewer.
"""

import math
import os

WIDTH = 256
HEIGHT = 192
SCR_PIXEL_SIZE = 6144
SCR_ATTR_SIZE = 768

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


def dist(x, y, cx, cy):
    return math.sqrt((x - cx) ** 2 + (y - cy) ** 2)


def ellipse_dist(x, y, cx, cy, rx, ry):
    """Normalised distance from center of ellipse (1.0 = on edge)."""
    if rx == 0 or ry == 0:
        return 999
    return math.sqrt(((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2)


def smoothstep(edge0, edge1, x):
    t = max(0, min(1, (x - edge0) / (edge1 - edge0))) if edge1 != edge0 else 0
    return t * t * (3 - 2 * t)


def generate_anglerfish(mouth_open=True):
    """Return a 256x192 float array of intensities (0=black, 1=white)."""
    img = [[0.0] * WIDTH for _ in range(HEIGHT)]

    cx, cy = 128, 100

    # --- Body: large rough ellipse ---
    body_rx, body_ry = 80, 70
    for y in range(HEIGHT):
        for x in range(WIDTH):
            d = ellipse_dist(x, y, cx, cy, body_rx, body_ry)
            # Body outline (ring of pixels)
            if 0.92 < d < 1.08:
                img[y][x] = max(img[y][x], 0.9)
            # Skin texture inside body
            if d < 0.92:
                # Warty texture using pseudo-random pattern
                noise = math.sin(x * 0.7 + y * 1.3) * math.sin(x * 1.1 - y * 0.9)
                skin = 0.15 + 0.1 * noise
                # Darker toward edges
                skin *= (1.0 - d * 0.5)
                img[y][x] = max(img[y][x], skin)

    # --- Mouth: large dark opening ---
    mouth_cy = cy + 15
    mouth_rx, mouth_ry = (50, 40) if mouth_open else (45, 12)

    for y in range(HEIGHT):
        for x in range(WIDTH):
            md = ellipse_dist(x, y, cx, mouth_cy, mouth_rx, mouth_ry)
            # Mouth rim
            if 0.90 < md < 1.10:
                img[y][x] = max(img[y][x], 1.0)
            # Inside mouth is dark
            if md < 0.88:
                img[y][x] = 0.0
            # Inner mouth: faint throat detail
            if md < 0.5 and mouth_open:
                throat = 0.05 + 0.03 * math.sin(x * 0.5) * math.sin(y * 0.5)
                img[y][x] = max(img[y][x], throat)

    # --- Teeth: triangular shapes around mouth rim ---
    num_teeth_top = 9
    num_teeth_bot = 7
    tooth_len = 18 if mouth_open else 8

    for i in range(num_teeth_top):
        angle = math.pi + (math.pi * (i + 0.5) / num_teeth_top)
        tx = cx + mouth_rx * 0.95 * math.cos(angle)
        ty = mouth_cy + mouth_ry * 0.95 * math.sin(angle)
        tip_x = tx + tooth_len * math.cos(angle + math.pi)
        tip_y = ty + tooth_len * math.sin(angle + math.pi)

        for t in range(20):
            frac = t / 19.0
            px = tx + (tip_x - tx) * frac
            py = ty + (tip_y - ty) * frac
            w = max(1, int(3 * (1 - frac)))
            for dx in range(-w, w + 1):
                ix, iy = int(px + dx), int(py)
                if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
                    img[iy][ix] = 1.0

    for i in range(num_teeth_bot):
        angle = (math.pi * (i + 0.5) / num_teeth_bot)
        tx = cx + mouth_rx * 0.95 * math.cos(angle)
        ty = mouth_cy + mouth_ry * 0.95 * math.sin(angle)
        tip_x = tx + tooth_len * math.cos(angle + math.pi)
        tip_y = ty + tooth_len * math.sin(angle + math.pi)

        for t in range(20):
            frac = t / 19.0
            px = tx + (tip_x - tx) * frac
            py = ty + (tip_y - ty) * frac
            w = max(1, int(3 * (1 - frac)))
            for dx in range(-w, w + 1):
                ix, iy = int(px + dx), int(py)
                if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
                    img[iy][ix] = 1.0

    # --- Eyes: two menacing circles ---
    eye_y = cy - 25
    eye_lx, eye_rx_pos = cx - 35, cx + 35
    eye_r = 12

    for ey_cx in [eye_lx, eye_rx_pos]:
        for y in range(HEIGHT):
            for x in range(WIDTH):
                d = dist(x, y, ey_cx, eye_y)
                # Eye outline
                if eye_r - 2 < d < eye_r + 2:
                    img[y][x] = max(img[y][x], 1.0)
                # Pupil (small dark center)
                if d < 5:
                    img[y][x] = 0.0
                # Iris ring
                if 5 <= d < 8:
                    img[y][x] = max(img[y][x], 0.7)
                # Glint
                if dist(x, y, ey_cx - 2, eye_y - 2) < 2:
                    img[y][x] = 1.0

    # --- Lure: bioluminescent esca ---
    lure_x, lure_y = cx, cy - 75
    # Stalk
    for t in range(40):
        frac = t / 39.0
        sx = cx + 8 * math.sin(frac * math.pi * 1.5)
        sy = cy - 35 - frac * 40
        ix, iy = int(sx), int(sy)
        if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
            img[iy][ix] = max(img[iy][ix], 0.8)

    # Lure glow
    for y in range(HEIGHT):
        for x in range(WIDTH):
            d = dist(x, y, lure_x + 8, lure_y)
            if d < 4:
                img[y][x] = 1.0
            elif d < 10:
                img[y][x] = max(img[y][x], 0.7 * (1 - d / 10))

    # --- Fin ridges on sides ---
    for side in [-1, 1]:
        for i in range(5):
            fy = cy - 20 + i * 18
            for t in range(25):
                frac = t / 24.0
                fx = cx + side * (body_rx * 0.85 + frac * 20)
                fyt = fy + frac * 8 * side
                ix, iy = int(fx), int(fyt)
                if 0 <= ix < WIDTH and 0 <= iy < HEIGHT:
                    img[iy][ix] = max(img[iy][ix], 0.6)

    return img


def apply_dither(img, reveal):
    """Apply ordered dither with given reveal level (0=all black, 1=full detail)."""
    pixels = bytearray(SCR_PIXEL_SIZE)

    for y in range(HEIGHT):
        for x in range(WIDTH):
            intensity = img[y][x] * reveal
            threshold = (BAYER_8x8[y & 7][x & 7] + 0.5) / 64.0
            if intensity > threshold:
                off = scr_offset(x, y)
                pixels[off] |= 0x80 >> (x & 7)

    # Attributes: white ink on black paper (depth 3 palette)
    attrs = bytearray([0x07] * SCR_ATTR_SIZE)
    return bytes(pixels) + bytes(attrs)


def main():
    out_dir = "assets"
    os.makedirs(out_dir, exist_ok=True)

    print("Generating anglerfish (mouth open)...")
    fish_open = generate_anglerfish(mouth_open=True)

    print("Generating anglerfish (mouth closed)...")
    fish_closed = generate_anglerfish(mouth_open=False)

    # Frames 1-4: gradual reveal of the open-mouth fish
    reveal_levels = [0.12, 0.30, 0.55, 0.80]
    for i, reveal in enumerate(reveal_levels):
        fname = f"{out_dir}/goo_{i+1}.scr"
        scr = apply_dither(fish_open, reveal)
        with open(fname, "wb") as f:
            f.write(scr)
        print(f"  Frame {i+1}: reveal={reveal:.0%} -> {fname}")

    # Frame 5: full detail open mouth
    fname = f"{out_dir}/goo_5.scr"
    scr = apply_dither(fish_open, 1.0)
    with open(fname, "wb") as f:
        f.write(scr)
    print(f"  Frame 5: full open mouth -> {fname}")

    # Frame 6: full detail closed mouth (swallow)
    fname = f"{out_dir}/goo_6.scr"
    scr = apply_dither(fish_closed, 1.0)
    with open(fname, "wb") as f:
        f.write(scr)
    print(f"  Frame 6: mouth closed -> {fname}")

    print("Done. 6 .scr files generated.")


if __name__ == "__main__":
    main()
