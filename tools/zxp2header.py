#!/usr/bin/env python3
"""
zxp2header.py - Convert ZX-Paintbrush .zxp files to C header files.

Produces static const uint8_t arrays suitable for Z88DK / SDCC.

ZXP format (text-based):
  Line 0: "ZX-Paintbrush"
  Line 1: dimensions or metadata (e.g. "16x32")
  Lines 2..N: pixel rows as ASCII '0' and '1' characters
  Blank separator line
  Attribute lines: space-separated hex bytes, one line per character row

For multi-frame sprites, frames are stacked vertically in the pixel data.
The frame height is inferred from: total_pixel_rows / num_frames.

Usage:
  zxp2header.py <input.zxp> <output.h> [--frames N] [--name NAME]
"""

import argparse
import os
import sys


def parse_zxp(path):
    """Parse a .zxp file, returning (width, pixel_rows, attr_bytes)."""
    with open(path, "r") as f:
        lines = [l.rstrip("\r\n") for l in f.readlines()]

    # Skip header (first 2 lines)
    if len(lines) < 3:
        sys.exit(f"Error: {path} too short to be a valid .zxp file")

    # Find pixel data: lines of '0' and '1' characters
    pixel_lines = []
    attr_start = None
    i = 2
    while i < len(lines):
        line = lines[i]
        # A blank line separates pixels from attributes
        if line.strip() == "":
            attr_start = i + 1
            break
        # Validate pixel line
        if all(c in "01" for c in line) and len(line) > 0:
            pixel_lines.append(line)
        else:
            sys.exit(
                f"Error: unexpected content on line {i+1}: {line[:40]!r}"
            )
        i += 1

    if not pixel_lines:
        sys.exit(f"Error: no pixel data found in {path}")

    width = len(pixel_lines[0])
    for idx, pl in enumerate(pixel_lines):
        if len(pl) != width:
            sys.exit(
                f"Error: inconsistent width on pixel line {idx+2+1} "
                f"(expected {width}, got {len(pl)})"
            )

    # Parse attribute data (if present)
    attr_bytes = []
    if attr_start is not None:
        for j in range(attr_start, len(lines)):
            line = lines[j].strip()
            if not line:
                continue
            for token in line.split():
                try:
                    attr_bytes.append(int(token, 16))
                except ValueError:
                    sys.exit(
                        f"Error: invalid attribute hex value "
                        f"'{token}' on line {j+1}"
                    )

    return width, pixel_lines, attr_bytes


def pixels_to_bytes(pixel_lines, width):
    """Convert ASCII '0'/'1' pixel rows to packed bytes (MSB = leftmost)."""
    bytes_per_row = (width + 7) // 8
    result = []
    for row in pixel_lines:
        row_bytes = []
        for col in range(0, width, 8):
            byte = 0
            for bit in range(8):
                px = col + bit
                if px < width and row[px] == "1":
                    byte |= 0x80 >> bit
            row_bytes.append(byte)
        result.extend(row_bytes)
    return result


def format_c_array(name, data, bytes_per_row=16):
    """Format a byte array as a C static const uint8_t declaration."""
    lines = [f"static const uint8_t {name}[{len(data)}] = {{"]
    for i in range(0, len(data), bytes_per_row):
        chunk = data[i : i + bytes_per_row]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        if i + bytes_per_row < len(data):
            hex_vals += ","
        lines.append(f"    {hex_vals}")
    lines.append("};")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Convert ZX-Paintbrush .zxp to C header"
    )
    parser.add_argument("input", help="Input .zxp file")
    parser.add_argument("output", help="Output .h file")
    parser.add_argument(
        "--frames",
        type=int,
        default=1,
        help="Number of animation frames (stacked vertically)",
    )
    parser.add_argument(
        "--name",
        default=None,
        help="Base name for C identifiers (default: derived from filename)",
    )
    args = parser.parse_args()

    width, pixel_lines, attr_bytes = parse_zxp(args.input)
    total_rows = len(pixel_lines)

    if total_rows % args.frames != 0:
        sys.exit(
            f"Error: {total_rows} pixel rows not divisible by "
            f"{args.frames} frames"
        )

    frame_height = total_rows // args.frames

    # Derive C identifier base name
    if args.name:
        base = args.name
    else:
        base = os.path.splitext(os.path.basename(args.input))[0]
        # Sanitise for C identifier
        base = "".join(c if c.isalnum() or c == "_" else "_" for c in base)

    # Build header guard
    guard = f"_{base.upper()}_H_"

    header_lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "/* Auto-generated from "
        + os.path.basename(args.input)
        + " by zxp2header.py */",
        "",
        "#include <stdint.h>",
        "",
        f"#define {base.upper()}_WIDTH  {width}",
        f"#define {base.upper()}_HEIGHT {frame_height}",
        f"#define {base.upper()}_FRAMES {args.frames}",
        "",
    ]

    # Generate per-frame arrays
    bytes_per_row = (width + 7) // 8
    for frame in range(args.frames):
        start_row = frame * frame_height
        end_row = start_row + frame_height
        frame_pixels = pixel_lines[start_row:end_row]
        data = pixels_to_bytes(frame_pixels, width)

        if args.frames == 1:
            arr_name = f"{base}_bitmap"
        else:
            arr_name = f"{base}_f{frame + 1}"

        header_lines.append(format_c_array(arr_name, data))
        header_lines.append("")

    # Attribute array (if present)
    if attr_bytes:
        header_lines.append(format_c_array(f"{base}_attr", attr_bytes))
        header_lines.append("")

    header_lines.append(f"#endif /* {guard} */")
    header_lines.append("")

    with open(args.output, "w") as f:
        f.write("\n".join(header_lines))

    print(
        f"Generated {args.output}: {width}x{frame_height}, "
        f"{args.frames} frame(s), "
        f"{len(attr_bytes)} attribute byte(s)"
    )


if __name__ == "__main__":
    main()
