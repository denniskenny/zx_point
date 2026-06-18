/*
 * vsync.c — Floating bus vsync with model auto-detection
 *
 * Uses the floating bus trick to sync to a precise screen position,
 * maximising the time available for screen updates each frame.
 *
 * A unique attribute marker is placed at the bottom of the status area
 * (row 22, cols 0-2).  The timed loop reads only attribute bytes from
 * the floating bus; when it matches the marker, the beam is at row 22
 * — giving the full bottom border + vblank + top border (~28 000
 * T-states) before the active display is scanned again.
 *
 * The marker is computed each frame as (ATTR[0] & 0x78) | 0x03:
 * paper+bright from the current depth attribute, ink set to magenta,
 * bit 0 forced on for +2A/+3 compatibility.  This makes the marker
 * visually invisible (paper matches the depth) while remaining unique
 * — magenta ink is never used by any game element.
 *
 * Three modes, detected once at startup by vsync_detect():
 *
 *   Mode 1 — 48K/128K/+2  (port 0x40FF, 35 T-state loop)
 *   Mode 2 — +2A/+3        (port 0x0FFD, 42 T-state loop)
 *   Mode 0 — HALT fallback  (ei / halt / di)
 *
 * The +2A/+3 path requires paging enabled (bit 5 of port 0x7FFD = 0).
 * If the game locks paging at startup, mode 2 will not activate and
 * +2A/+3 machines fall back to HALT.
 *
 * Reference: Ast A. Moore, "The Definitive Programmer's Guide to
 * Using the Floating Bus Trick on the ZX Spectrum"
 */

#include "../include/vsync.h"
#include "../include/hw.h"

uint8_t vsync_mode = 0;

void vsync_detect(void) __naked
{
    __asm

    ;; ---- Write sync marker to attr row 22, cols 0-2 ----
    ;; 0x1B used only during boot detection (screen not yet set up).
    ;; vsync_wait() replaces this with a depth-derived marker.
    ld  hl, 0x5AC0         ; attr row 22, col 0
    ld  a, 0x1B
    ld  (hl), a
    inc l
    ld  (hl), a
    inc l
    ld  (hl), a
    ;; Col 3 = 0x00 — preload byte for +2A/+3 idle bus
    inc l
    xor a
    ld  (hl), a

    ;; ---- Test 48K floating bus (port 0xFF) ----
    ;; Read up to 10 000 times (~2 frames).  If we ever see
    ;; non-0xFF, the floating bus is active on this port.
    ld  bc, 10000
_vsd_48k_loop:
    in  a, (0xFF)
    cp  0xFF
    jr  nz, _vsd_48k_ok
    dec bc
    ld  a, b
    or  c
    jr  nz, _vsd_48k_loop

    ;; Port 0xFF failed.  If is_128k, try the +2A/+3 port.
    ld  a, (_is_128k)
    or  a
    jr  z, _vsd_halt

    ;; ---- Test +2A/+3 floating bus (port 0x0FFD) ----
    ;; Returns values ORed with 1.  Returns 0xFF if paging locked.
    ld  bc, 10000
_vsd_128k_loop:
    ld  a, 0x0F
    in  a, (0xFD)          ; port 0x0FFD
    cp  0xFF
    jr  nz, _vsd_128k_ok
    dec bc
    ld  a, b
    or  c
    jr  nz, _vsd_128k_loop

    ;; Both ports failed — emulator or paging-locked +2A/+3
_vsd_halt:
    xor a
    ld  (_vsync_mode), a
    ret

_vsd_48k_ok:
    ld  a, 1
    ld  (_vsync_mode), a
    ret

_vsd_128k_ok:
    ld  a, 2
    ld  (_vsync_mode), a
    ret

    __endasm;
}

void vsync_wait(void) __naked
{
    __asm

    ;; ---- Compute marker from current depth ----
    ;; marker = (ATTR[0] & 0x78) | 0x03
    ;; Paper+bright matches the depth palette; ink = magenta (unused
    ;; by any game element); bit 0 set for +2A/+3 bus compatibility.
    ld  a, (0x5800)         ; [13] read ATTR[0]
    and 0x78                ; [7]  paper + bright bits
    or  0x03                ; [7]  ink = magenta, bit 0 set
    ld  d, a                ; [4]  D = marker for detection loop

    ;; ---- Refresh marker every frame ----
    ;; Writes 4 bytes to attr RAM (~50 T with contention).
    ;; Guarantees marker survives screen_clear() between states.
    ld  hl, 0x5AC0          ; row 22, col 0
    ld  (hl), d
    inc l
    ld  (hl), d             ; col 1
    inc l
    ld  (hl), d             ; col 2
    inc l
    xor a
    ld  (hl), a             ; col 3 = preload byte (0x00)

    ;; ---- Branch on detected mode ----
    ld  a, (_vsync_mode)
    or  a
    jr  z, _vs_halt
    dec a
    jr  z, _vs_48k

    ;; ============================================================
    ;; Mode 2: +2A/+3 floating bus  (port 0x0FFD, 42 T per iter)
    ;; ============================================================
    ;; Contended read preloads bus with 0x00 during idle intervals.
    ;; ULA returns (attr | 1) during fetches — marker matches
    ;; because bit 0 is already set.
_vs_128k:
    ld  e, 0x0F            ; E = port MSB (D = marker, set above)
_vs_128k_loop:
    ld  a, (0x5AC3)        ; [13] contended read (bus ← 0x00)
    ld  a, e               ; [4]  A = 0x0F
    in  a, (0xFD)          ; [11] read port 0x0FFD
    cp  d                  ; [4]  marker?
    jp  nz, _vs_128k_loop  ; [10] 42 T total
    ret

    ;; ============================================================
    ;; Mode 1: 48K/128K/+2 floating bus  (port 0x40FF, 35 T per iter)
    ;; ============================================================
    ;; dec hl pads the loop to exactly 35 T-states so the IN
    ;; instruction always lands on an attribute fetch, never a
    ;; bitmap or idle interval.
_vs_48k:
    ld  e, 0x40            ; E = port MSB (D = marker, set above)
_vs_48k_loop:
    dec hl                 ; [6]  padding
    ld  a, e               ; [4]  A = 0x40
    in  a, (0xFF)          ; [11] read port 0x40FF
    cp  d                  ; [4]  marker?
    jp  nz, _vs_48k_loop   ; [10] 35 T total
    ret

    ;; ============================================================
    ;; Mode 0: HALT fallback
    ;; ============================================================
    ;; startup=31 boots with interrupts disabled.  Briefly enable
    ;; for HALT, then disable again.
_vs_halt:
    ei
    halt
    di
    ret

    __endasm;
}
