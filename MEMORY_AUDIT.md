# Memory Audit — `downship.map`

Regenerate with `make -B downship.tap USER_CFLAGS=-m` and read the
`__*_head` / `__*_tail` symbols in `downship.map`. 48K target, code origin
`0x8000`, `.tap` = **31,365 bytes** (audited 2026-11-17).

## Full 48K RAM Map (`0x4000`–`0xFFFF`)

| Region | Range | Size | Notes |
|---|---|---|---|
| ULA screen + attrs | `0x4000`–`0x5AFF` | 6,912 | display file |
| printer buffer, system variables, BASIC loader | `0x5B00`–`0x5FFF` | — | **in use by the ROM** (IM1 ISR, `IY=0x5C3A`); don't reuse |
| GOO scratch buffer | `0x6000`–`0x68FF` | 2,304 | **transient**: GOO death frames, and the boat-intro shifted-sprite cache (672 B) — never live at the same time |
| bit-reverse table | `0x6900`–`0x69FF` | 256 | GOO mirror blit |
| sprite arena (unpacked) | `0x6A00`–`0x7A10` | 4,112 | resident sprites, from a 1,355 B ZX0 blob |
| free / stack growth room | `0x7A10`–~`0x7F5D` | ~1,350 | |
| **stack** | grows down from `0x7FFF` | deepest seen `0x7F5D` | BASIC `CLEAR 32767` sets SP; z88dk keeps it (`__register_sp = -1`) |
| CRT startup | `0x8000`–`0x80E0` | 224 | |
| C code (`code_compiler`) | `0x82BC`–`0xC41B` | 16,735 | |
| asm code (`code_user`: Tritone engine + tunes) | `0xC61E`–`0xCD99` | 1,915 | |
| rodata (`rodata_compiler`) | `0xCD99`–`0xEF8D` | 8,692 | compressed assets + const tables |
| library / misc sections | `0xEF8D`–`0xF31B` | ~910 | |
| DATA (initialised) | `0xF31B`–`0xF33C` | 33 | |
| BSS (zeroed globals) | `0xF33C`–`0xFA35` | 1,785 | |
| **free** | `0xFA35`–`0xFF56` | **~1,313** | `0xFF56` = `CRT_MAX_HEAP_ADDRESS`; above it are the ROM UDGs |

**Important:** there is *no* large free gap in the code/data area. Everything
from `0x8000` to `0xFA35` is used, so the real headroom for new code,
constants or globals is the ~1.3 KB at the top. If the build reports
`Section exceeds 64k`, that region has been used up.

## Compressed Assets (rodata)

- `title_logo.h` — ~2,411 B (logo + attrs, ZX0)
- `goo_data.h` — 2,318 B (4 anglerfish frames, 3 stored: frames 3/4 are identical and share data)
- `sprites_blob.h` — 1,355 B (all resident sprites, ZX0 → unpacked to `0x6A00`)
- `vignette.h` — ~1,191 B (ZX0)

## Notes

- **Stack** lives below `0x8000`, between the sprite arena and the code. The
  sprite arena growing past ~`0x7E00` would start to collide with it. Mirroring
  another 32x32 sprite (with 16/8 downscales) costs ~336 B of arena.
- **Sprite data the game never reads** is kept out of the blob with
  `SPRITE_DROP` in the Makefile (per-sprite attribute arrays, etc.).
- **Biggest levers if space runs out:** `state.c` is the largest module; the
  compressed title logo and GOO frames dominate rodata.
