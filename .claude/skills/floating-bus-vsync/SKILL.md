---
name: floating-bus-vsync
description: Implement or modify floating bus vsync detection on the ZX Spectrum for both 48K and 128K models. Covers timed attribute-matching loops, marker setup, model detection, and +2A/+3 differences.
when_to_use: "floating bus" or "vsync" or "frame timing" or "sync to beam" or "free frame time" or "screen tearing"
allowed-tools: Bash Read Write Edit
effort: medium
---

# Floating Bus Vsync — ZX Spectrum

The floating bus trick exploits a hardware quirk where the CPU can read the same data the ULA is fetching from screen RAM. By timing reads precisely, we can detect which screen row the beam is scanning and sync our drawing to it.

**Reference**: Ast A. Moore, "The Definitive Programmer's Guide to Using the Floating Bus Trick on the ZX Spectrum" (sky.relative-path.com/zx/floating_bus.html)

## How It Works

During active display, the ULA fetches bitmap+attribute bytes in a repeating pattern with 4 T-state idle gaps. Reading an unattached port during this time returns whatever the ULA last put on the bus. During border/vblank, the bus floats high → reads 0xFF.

By placing a **unique marker attribute** at a known screen row and running a **precisely timed loop**, we detect when the beam reaches that row. This gives a sync point near the bottom of the display, maximising safe drawing time.

## Key Constants

| Item | Value |
|------|-------|
| 48K frame | 69 888 T-states, 312 lines × 224 T/line |
| Active display | 192 lines (lines 64–255) |
| Top border | 64 lines |
| Bottom border | 56 lines |
| Safe time (marker at row 22) | ~28 000 T-states (bottom border + vblank + top border) |
| Safe time (HALT fallback) | ~14 000 T-states (top border only) |

## Two Floating Bus Techniques

### 48K / 128K / +2 — Port 0xFF, 35 T-state loop

```z80
    ld  d, MARKER        ; expected attribute
    ld  e, 0x40          ; port MSB (port = 0x40FF)
loop:
    dec hl               ; [6]  padding
    ld  a, e             ; [4]
    in  a, (0xFF)        ; [11] read floating bus
    cp  d                ; [4]  match marker?
    jp  nz, loop         ; [10] total = 35 T
    ret
```

The 35 T-state timing ensures each `IN` instruction lands on an **attribute fetch**, never a bitmap or idle interval. The port MSB (A register) selects 0x40FF; only the LSB matters for ULA decoding.

**Requirements:**
- Code must be in **non-contended memory** (≥ 0x8000)
- Marker attribute must be **unique** — not used anywhere else on screen
- Marker must **not** be 0xFF

### +2A / +3 — Port 0x0FFD, 42 T-state loop

```z80
    ld  d, MARKER        ; expected attribute (bit 0 must be set)
    ld  e, 0x0F          ; port MSB (port = 0x0FFD)
loop:
    ld  a, (PRELOAD)     ; [13] contended read — preloads bus for idle
    ld  a, e             ; [4]
    in  a, (0xFD)        ; [11] read floating bus
    cp  d                ; [4]  match marker?
    jp  nz, loop         ; [10] total = 42 T
    ret
```

**Key differences from 48K:**
1. Only works on ports matching `1 + 4n` where n < 4096 (e.g., 0x0FFD = 4093)
2. **Paging must be enabled** (bit 5 of port 0x7FFD = 0). If paging is locked, bus always returns 0xFF.
3. Returned value is **ORed with 1** — marker must have bit 0 set
4. During idle intervals, bus returns the last **contended memory** value (not 0xFF). A `ld a,(addr)` reading from contended RAM (0x4000–0x7FFF) preloads a known non-marker value.
5. Padding instruction changes from `dec hl` (6T) to `ld a,(nnnn)` (13T) to hit 42 T-states

## Marker Attribute Selection

The marker must:
1. Have **bit 0 set** (for +2A/+3 — value ORed with 1 must still match)
2. **Not appear** anywhere else on the display as an attribute byte
3. **Not be 0xFF**
4. For +2A/+3: no other attribute on screen, when ORed with 1, should equal the marker

Current implementation uses **0x1B** (PAPER 3 magenta, INK 3 magenta — invisible). Verify against all game attribute values:
- Depth attrs: 0x29, 0x0D, 0x41
- Transition attrs: 0x21, 0x19, 0x11, 0x09
- Entity attrs: 0x2C, 0x0C, 0x44, 0x2F, 0x0F, 0x47
- HUD attrs: 0x47, 0x42, 0x00
- Death attrs: 0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x00
- Screen clear: 0x07

None of these (nor any | 1) equals 0x1B. ✓

## Marker Placement

The marker is written to **attribute row 22, cols 0–3** (addresses 0x5AC0–0x5AC3). This row is:
- Below the main viewport (VIEW_H = 160 = 20 rows)
- Below the HUD (HUD_ROW = 21)
- Not touched by minimap (cols 28–31) or depth bar (col 27)
- Refreshed at the start of every `vsync_wait()` call (~60 T-states with contention)

A fifth byte at col 4 (0x5AC4) is set to 0x00 as the +2A/+3 preload value.

## Auto-Detection (vsync_detect)

Called once at startup after `hw_detect()` and **before** locking paging:

1. Write the marker to attr RAM
2. Read port 0xFF up to 10 000 times. If any read returns non-0xFF → **mode 1** (48K floating bus)
3. If timeout and `is_128k` → read port 0x0FFD up to 10 000 times. If non-0xFF → **mode 2** (+2A/+3)
4. If both timeout → **mode 0** (HALT fallback)

The detection loop does NOT need the 35/42 T-state timing — it only needs to observe whether non-0xFF values appear on the port. A simple untimed loop with a counter timeout suffices.

**Critical startup order:**
```c
hw_detect();       // needs paging enabled for 128K bank test
vsync_detect();    // needs paging enabled for +2A/+3 floating bus test
// THEN lock paging:
// ld a, 0x30 / out (0x7FFD), a
```

## Emulator Support

The 48K floating bus (port 0xFF) is supported by most emulators including **ZEsarUX** (confirmed working). The +2A/+3 technique is only supported by:
- SpecEmu (Windows)
- Spectramine (Windows)
- SpecIde (cross-platform)
- ZXDS (Nintendo DS)

Emulators without floating bus support will timeout during detection and fall back to HALT.

## Implementation Files

| File | Role |
|------|------|
| `src/vsync.c` | `vsync_detect()` and `vsync_wait()` — all-assembly, `__naked` |
| `include/vsync.h` | Declarations + `extern uint8_t vsync_mode` |
| `src/main.c` | Calls `vsync_detect()` after `hw_detect()`, before paging lock |
| `src/hw_detect.c` | Sets `is_128k` (used by vsync_detect to try +2A/+3 path) |

## Troubleshooting

- **Game hangs on startup**: Marker attr matches something on screen during the ROM/loader phase. Pick a different marker value.
- **Flicker after state transition**: `screen_clear()` wipes the marker. The marker is auto-refreshed at the start of every `vsync_wait()` call, so one frame uses HALT fallback, then the next frame has the marker back.
- **+2A/+3 always falls back to HALT**: Paging is locked (bit 5 of port 0x7FFD). Either move the paging lock after detection, or don't lock paging on +2A/+3 machines.
- **Sync jitter / unstable**: Code is in contended memory (< 0x8000). Move to non-contended RAM.
- **Wrong sync point**: Marker attribute appears elsewhere on screen. Verify uniqueness against all game attributes including entity colours and transition steps.
