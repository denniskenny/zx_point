---
name: compile-scr
description: Compile a ZX Spectrum .scr file into a ZX0-compressed C header for inclusion in the game. Compresses pixel data only (first 6144 bytes), generates a static const array.
when_to_use: "compile scr" or "compile vignette" or "convert scr" or "new scr" or "update scr" or "compress screen"
allowed-tools: Bash Read Write Edit
effort: low
---

# Compile .scr to ZX0 C Header

Convert a ZX Spectrum .scr screen file into a ZX0-compressed C header array for use in the game.

## Prerequisites

- ZX0 compressor at `/tmp/ZX0/src/zx0`
- Input: a 6912-byte `.scr` file (6144 bytes pixel data + 768 bytes attributes)

## Process

1. **Compress the full .scr with ZX0** (all 6912 bytes — pixels + attributes are both needed):
   ```bash
   rm -f /tmp/TEMP.zx0
   /tmp/ZX0/src/zx0 INPUT.scr /tmp/TEMP.zx0
   ```

3. **Generate C header** with this Python one-liner:
   ```bash
   python3 -c "
   import sys
   with open('/tmp/TEMP.zx0', 'rb') as f:
       data = f.read()
   name = 'ARRAYNAME'  # e.g. 'vignette_zx0'
   guard = 'GUARDNAME'  # e.g. '_VIGNETTE_H_'
   lines = [f'#ifndef {guard}', f'#define {guard}', '',
            f'/* ZX0 compressed screen data ({len(data)} bytes) */',
            f'static const unsigned char {name}[] = {{']
   for i in range(0, len(data), 16):
       chunk = data[i:i+16]
       hex_bytes = ', '.join(f'0x{b:02X}' for b in chunk)
       lines.append(f'    {hex_bytes},')
   lines[-1] = lines[-1].rstrip(',')
   lines.append('};')
   lines.append('')
   lines.append(f'#endif /* {guard} */')
   lines.append('')
   with open('OUTPUT.h', 'w') as f:
       f.write('\n'.join(lines))
   print(f'Written {len(data)} bytes as C array to OUTPUT.h')
   "
   ```

4. **Force rebuild** (headers aren't tracked Makefile dependencies):
   ```bash
   touch src/state.c && make all
   ```

## Parameters

When the user asks to compile a .scr file, determine:
- **Input .scr path**: usually in `assets/` (e.g., `assets/vignette.scr`)
- **Output .h path**: usually in `include/` (e.g., `include/vignette.h`)
- **Array name**: matches existing convention (e.g., `vignette_zx0`)
- **Header guard**: matches existing convention (e.g., `_VIGNETTE_H_`)

If the user just says "compile the vignette" or similar, use the known defaults:
- `assets/vignette.scr` → `include/vignette.h`, array `vignette_zx0`, guard `_VIGNETTE_H_`

## Usage in game code

The compressed data is decompressed at runtime with:
```c
dzx0_decompress(vignette_zx0, SCREEN);  // decompress to 0x4000
```

The game area attributes (top 20 rows) are then overwritten by `depth_set()` / `memset(ATTR, ...)`, but the bottom 4 rows (status area) retain the attributes from the .scr file.

## Notes

- The full 6912-byte .scr is compressed (pixels + attributes) — the bottom 4 rows of attributes contain the status area layout and must be preserved
- The ZX0 compressor will error if the output file already exists — always `rm -f` first
- After updating a header, `touch` a .c file that includes it to force rebuild
