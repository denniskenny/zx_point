#!/usr/bin/env python3
"""Crop .scr files to their non-zero bounding box, ZX0-compress, and emit a C header.

Finds the union bounding box across all input frames, extracts that region
in row-major order, compresses each with ZX0, and writes a single C header
with the data arrays and crop constants.

Usage:
    python3 tools/scr_crop_zx0.py output.h zx0_path name1:file1.scr [name2:file2.scr ...]

Requires the zx0 compressor binary at zx0_path.
"""

import os
import subprocess
import sys
import tempfile


def scr_offset(x, y):
    col = x >> 3
    third = (y >> 6) & 3
    char_row = (y >> 3) & 7
    pixel_row = y & 7
    return (third << 11) | (pixel_row << 8) | (char_row << 5) | col


def find_bbox(frames):
    """Find union bounding box (byte-column, pixel-row) across all frames."""
    min_y, max_y = 192, 0
    min_col, max_col = 32, 0
    for data in frames:
        for y in range(192):
            for col in range(32):
                if data[scr_offset(col * 8, y)] != 0:
                    if y < min_y: min_y = y
                    if y > max_y: max_y = y
                    if col < min_col: min_col = col
                    if col > max_col: max_col = col
    return min_col, min_y, max_col, max_y


def crop_frame(data, min_col, min_y, max_col, max_y):
    """Extract bounding box region in row-major order."""
    out = bytearray()
    for y in range(min_y, max_y + 1):
        for col in range(min_col, max_col + 1):
            out.append(data[scr_offset(col * 8, y)])
    return bytes(out)


def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} output.h zx0_path name1:file1.scr [...]")
        sys.exit(1)

    dst = sys.argv[1]
    zx0_bin = sys.argv[2]
    entries = []
    for arg in sys.argv[3:]:
        name, path = arg.split(":", 1)
        with open(path, "rb") as f:
            data = f.read()[:6144]
        entries.append((name, data))

    # Union bounding box
    frames = [data for _, data in entries]
    min_col, min_y, max_col, max_y = find_bbox(frames)
    w = max_col - min_col + 1
    h = max_y - min_y + 1
    crop_size = w * h

    print(f"Bounding box: rows {min_y}-{max_y}, cols {min_col}-{max_col}")
    print(f"Crop: {h} rows x {w} byte-cols = {crop_size} bytes (was 6144)")

    # Crop and compress each frame
    compressed = []
    with tempfile.TemporaryDirectory() as tmpdir:
        for name, data in entries:
            cropped = crop_frame(data, min_col, min_y, max_col, max_y)
            bin_path = os.path.join(tmpdir, f"{name}.bin")
            zx0_path = os.path.join(tmpdir, f"{name}.zx0")
            with open(bin_path, "wb") as f:
                f.write(cropped)
            subprocess.run([zx0_bin, "-f", bin_path, zx0_path],
                           check=True, capture_output=True)
            with open(zx0_path, "rb") as f:
                zdata = f.read()
            compressed.append((name, zdata))
            print(f"  {name}: {len(zdata)} bytes ZX0")

    # Write header
    guard = "_GOO_DATA_H_"
    with open(dst, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write(f"#define GOO_CROP_COL  {min_col}\n")
        f.write(f"#define GOO_CROP_ROW  {min_y}\n")
        f.write(f"#define GOO_CROP_W    {w}\n")
        f.write(f"#define GOO_CROP_H    {h}\n")
        f.write(f"#define GOO_CROP_SIZE {crop_size}\n\n")
        for name, zdata in compressed:
            f.write(f"/* ZX0 compressed cropped screen data ({len(zdata)} bytes) */\n")
            f.write(f"static const unsigned char {name}[] = {{\n")
            for i in range(0, len(zdata), 16):
                chunk = zdata[i : i + 16]
                f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk))
                if i + 16 < len(zdata):
                    f.write(",")
                f.write("\n")
            f.write("};\n\n")
        f.write(f"#endif /* {guard} */\n")

    total = sum(len(z) for _, z in compressed)
    print(f"\nWritten {dst}: {len(compressed)} arrays, {total} bytes total")


if __name__ == "__main__":
    main()
