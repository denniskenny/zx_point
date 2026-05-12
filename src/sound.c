/*
 * sound.c — Beeper sound effects
 *
 * Port 254 bit layout: bit 4 = speaker, bits 0-2 = border colour.
 * We read the current border from _border_val (depth.c) so the
 * beeper never changes the border colour.
 */

#include <stdint.h>
#include "../include/sound.h"

extern uint8_t border_val;

void beep_ping(void) __naked
{
    __asm
        ld  c, 254
        ld  a, (_border_val)
        ld  d, a            ; d = border with speaker off
        or  0x10
        ld  e, a            ; e = border with speaker on

        ;; --- main ping: 30 half-cycles -------------------------
        ld  b, 30
    bp_t1:
        ld  a, e
        out (c), a
        ld  a, 20
    bp_d1a:
        dec a
        jr  nz, bp_d1a
        ld  a, d
        out (c), a
        ld  a, 20
    bp_d1b:
        dec a
        jr  nz, bp_d1b
        djnz bp_t1

        ;; --- silence gap 1 (~40 ms) ---------------------------
        ld  hl, 5400
    bp_g1:
        dec hl
        ld  a, h
        or  l
        jr  nz, bp_g1

        ;; --- echo 1: 18 half-cycles ----------------------------
        ld  b, 18
    bp_t2:
        ld  a, e
        out (c), a
        ld  a, 20
    bp_d2a:
        dec a
        jr  nz, bp_d2a
        ld  a, d
        out (c), a
        ld  a, 20
    bp_d2b:
        dec a
        jr  nz, bp_d2b
        djnz bp_t2

        ;; --- silence gap 2 (~70 ms) ---------------------------
        ld  hl, 9400
    bp_g2:
        dec hl
        ld  a, h
        or  l
        jr  nz, bp_g2

        ;; --- echo 2: 10 half-cycles ----------------------------
        ld  b, 10
    bp_t3:
        ld  a, e
        out (c), a
        ld  a, 20
    bp_d3a:
        dec a
        jr  nz, bp_d3a
        ld  a, d
        out (c), a
        ld  a, 20
    bp_d3b:
        dec a
        jr  nz, bp_d3b
        djnz bp_t3

        ;; --- silence gap 3 (~100 ms) --------------------------
        ld  hl, 13500
    bp_g3:
        dec hl
        ld  a, h
        or  l
        jr  nz, bp_g3

        ;; --- echo 3: 5 half-cycles (faintest) ------------------
        ld  b, 5
    bp_t4:
        ld  a, e
        out (c), a
        ld  a, 20
    bp_d4a:
        dec a
        jr  nz, bp_d4a
        ld  a, d
        out (c), a
        ld  a, 20
    bp_d4b:
        dec a
        jr  nz, bp_d4b
        djnz bp_t4

        ret
    __endasm;
}
