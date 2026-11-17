/*
 * input.c — Keyboard / joystick reading via naked asm
 *
 * Extracted from bubblefield.c.
 */

#include "../config/game_config.h"
#include "../include/input.h"
#include "../include/hw.h"

uint8_t read_keys(uint16_t port) __z88dk_fastcall __naked
{
    (void)port;
    __asm
        ld  b, h
        ld  c, l
        in  a, (c)
        ld  l, a
        ret
    __endasm;
}

uint8_t scan_input(void) __naked
{
    __asm

    ld  d, #0               ; D accumulates result
    ld  c, #(KEY_QWERT & 0xFF) ; all keyboard ports share low byte 0xFE

    ;; Q-T half-row: Q is bit 0 → INPUT_FWD
    ld  b, #(KEY_QWERT >> 8)
    in  a, (c)
    bit 0, a
    jr  nz, _si_no_q
    set 0, d
_si_no_q:

    ;; A-G half-row: A is bit 0 → INPUT_BACK
    ld  b, #(KEY_ASDFG >> 8)
    in  a, (c)
    bit 0, a
    jr  nz, _si_no_a
    set 1, d
_si_no_a:

    ;; P-Y half-row: P is bit 0 → INPUT_RIGHT, O is bit 1 → INPUT_LEFT
    ld  b, #(KEY_POIUY >> 8)
    in  a, (c)
    bit 0, a
    jr  nz, _si_no_p
    set 3, d
_si_no_p:
    bit 1, a
    jr  nz, _si_no_o
    set 2, d
_si_no_o:

    ;; Shift-V half-row: Z is bit 1 → INPUT_DESC, X is bit 2 → INPUT_ASC
    ld  b, #(KEY_SHZXCV >> 8)
    in  a, (c)
    bit 1, a
    jr  nz, _si_no_z
    set 4, d
_si_no_z:
    bit 2, a
    jr  nz, _si_no_x
    set 5, d
_si_no_x:

    ;; Kempston port — R=0 L=1 D=2 U=3 Fire=4 Fire2=5
    ld  a, (_has_kempston)
    or  a
    jr  z, _si_done
    ld  b, #(KEMP_PORT >> 8)
    ld  c, #(KEMP_PORT & 0xFF)
    in  a, (c)
    bit 3, a
    jr  z, _si_no_ju
    set 0, d
_si_no_ju:
    bit 2, a
    jr  z, _si_no_jd
    set 1, d
_si_no_jd:
    bit 1, a
    jr  z, _si_no_jl
    set 2, d
_si_no_jl:
    bit 0, a
    jr  z, _si_no_jr
    set 3, d
_si_no_jr:
    bit 4, a
    jr  z, _si_no_jf1
    set 4, d
_si_no_jf1:
    bit 5, a
    jr  z, _si_no_jf2
    set 5, d
_si_no_jf2:

_si_done:
    ld  l, d
    ret

    __endasm;
}
