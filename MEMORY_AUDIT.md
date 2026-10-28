# Memory Audit — `downship.map`

Parsed from the current build (map was the newest file, so it reflects current
sources). Code origin `0x8000`, 48K target, `.tap` = **31,214 bytes**.

## Full 48K RAM Map (`0x4000`–`0xFFFF`)

| Region | Range | Size | Notes |
|---|---|---|---|
| ULA screen + attrs | `0x4000`–`0x5AFF` | 6,912 | display file |
| free | `0x5B00`–`0x5FFF` | ~1,280 | (printer buffer area) |
| GOO scratch buffer | `0x6000`–`0x68FF` | 2,304 | **transient** — GOO death only |
| bit-reverse table | `0x6900`–`0x69FF` | 256 | GOO mirror blit |
| sprite arena (unpacked) | `0x6A00`–~`0x7900` | ~3,800 | resident sprites (from 977 B ZX0 blob) |
| free | ~`0x7900`–`0x80E0` | ~1,900 | low-RAM headroom |
| CRT startup | `0x8000`–`0x80E0` | 224 | |
| **CODE + rodata** | `0x80E0`–`0xCB65` | **19,077** | code + compressed assets |
| **free (largest block)** | `0xCB65`–`0xF373` | **10,254** | biggest contiguous gap |
| DATA (init) | `0xF373`–`0xF393` | 32 | |
| BSS (globals) | `0xF393`–`0xF99E` | 1,547 | |
| **stack** | `0xF99E`–`0xFFFF` | **1,633** | SP starts `0xFFFF`, grows down |

## Code Size by Module (compiled span)

Pure compiled code (`__code_compiler`) = **16,171 B**. Largest consumers:

- `state.c` — ~4,810 B (all game states + GOO death asm)
- `predators.c` — ~1,885 B
- `gfx.c` — ~1,486 B
- `bubblefield.c` — ~1,383 B
- `minimap.c` — ~1,193 B
- `treasure.c` — ~1,063 B
- `entity_render.c` — ~995 B
- Remaining modules each < 600 B

## Compressed Assets (rodata, in CODE section)

- `goo_data.h` — ~3,380 B (4 anglerfish frames, ZX0)
- `title_logo.h` — ~2,411 B (logo + attrs, ZX0)
- `vignette.h` — ~1,191 B (ZX0)
- `sprites_blob.h` — 977 B (all resident sprites, ZX0 → unpacked to `0x6A00`)

## Verdict

**Fits 48K comfortably — no collisions, ample headroom.**

- **~13.4 KB free** total: 10,254 B main gap (`0xCB65`–`0xF373`) + ~1,900 B
  low-RAM + ~1,280 B below screen.
- **Low-RAM arenas are safe**: sprite arena ends ~`0x7900`, ~1,900 B clear of the
  `0x8000` code origin. The GOO scratch buffer at `0x6000` is transient (death
  sequence only) and doesn't overlap resident data.
- **Stack: 1,633 B** (`0xF99E`→`0xFFFF`). Fine for this codebase (no recursion),
  but it's bounded by BSS top at `0xF99E`, not by the 10 KB gap.

## Recommendations

- **No trimming needed** — plenty of room for remaining Phase 10 / lore features.
- **If stack ever gets tight**: the 10 KB gap sits below DATA (`0xF373`), so
  raising stack headroom would mean relocating DATA/BSS lower rather than
  expanding upward. Not needed now.
- **Biggest future savings lever** (if ever required): `state.c` and the
  compressed assets dominate — but at ~13 KB free there's no pressure.
