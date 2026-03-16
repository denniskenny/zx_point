/*
 * vsync.c — Vsync wait with floating bus technique + HALT fallback
 *
 * The floating bus trick reads port 0xFF during the ULA's screen
 * rendering. During active display, the ULA drives the bus with
 * pixel/attribute data (non-0xFF values). During vertical blanking
 * (and horizontal blanking), the bus floats high → reads as 0xFF.
 *
 * Strategy:
 * 1. Wait until we're NOT in vblank (read != 0xFF) — i.e. wait for
 *    active display. This avoids catching the tail end of a vblank.
 * 2. Then wait until we ARE in vblank (read == 0xFF for several
 *    consecutive reads to filter horizontal blanking).
 *
 * If the floating bus doesn't work (emulator returns constant 0xFF),
 * we detect this and fall back to HALT.
 *
 * 48K/128K safe: port 0xFF is the same on both models.
 */

#include "../include/vsync.h"

void vsync_wait(void) __naked
{
    __asm

        ;; --- Phase 1: wait for active display (non-0xFF) ---
        ;; Timeout after ~20000 reads (~35ms). If we never see
        ;; non-0xFF, floating bus is unsupported → use HALT.
        ld  de, 20000
    _vs_wait_active:
        in  a, (0xFF)
        cp  0xFF
        jr  nz, _vs_phase2
        dec de
        ld  a, d
        or  e
        jr  nz, _vs_wait_active

        ;; Timeout — floating bus not working, fall back to HALT.
        ;; startup=31 runs with interrupts disabled; briefly enable
        ;; for HALT, then disable again.  Safe because SP1 is not
        ;; active at this point (we are between frames).
        ei
        halt
        di
        ret

    _vs_phase2:
        ;; --- Phase 2: wait for vblank (0xFF sustained) ---
        ;; We need several consecutive 0xFF reads to distinguish
        ;; vblank from horizontal blanking gaps.
        ld  b, 16           ; consecutive 0xFF reads needed
    _vs_wait_vblank:
        in  a, (0xFF)
        cp  0xFF
        jr  nz, _vs_phase2  ; reset counter on non-0xFF
        djnz _vs_wait_vblank

        ;; We are in vblank -- return
        ret

    __endasm;
}
