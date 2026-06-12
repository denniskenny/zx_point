/*
 * dzx0.c -- ZX0 decompressor (68-byte "standard" Z80 routine)
 *
 * ZX0 by Einar Saukas & Urusergi.
 * C wrapper passes src/dst via globals to avoid calling-convention issues.
 */

#include <stdint.h>
#include "../include/dzx0.h"

static const uint8_t *dzx0_src;
static uint8_t *dzx0_dst;

static void dzx0_run(void) __naked
{
    __asm

    ld  hl, (_dzx0_src)
    ld  de, (_dzx0_dst)

    ld      bc, #0xffff
    push    bc
    inc     bc
    ld      a, #0x80

_dzx0s_literals:
    call    _dzx0s_elias
    ldir
    add     a, a
    jr      c, _dzx0s_new_offset
    call    _dzx0s_elias

_dzx0s_copy:
    ex      (sp), hl
    push    hl
    add     hl, de
    ldir
    pop     hl
    ex      (sp), hl
    add     a, a
    jr      nc, _dzx0s_literals

_dzx0s_new_offset:
    pop     bc
    ld      c, #0xfe
    call    _dzx0s_elias_loop
    inc     c
    ret     z
    ld      b, c
    ld      c, (hl)
    inc     hl
    rr      b
    rr      c
    push    bc
    ld      bc, #1
    call    nc, _dzx0s_elias_backtrack
    inc     bc
    jr      _dzx0s_copy

_dzx0s_elias:
    inc     c
_dzx0s_elias_loop:
    add     a, a
    jr      nz, _dzx0s_elias_skip
    ld      a, (hl)
    inc     hl
    rla
_dzx0s_elias_skip:
    ret     c
_dzx0s_elias_backtrack:
    add     a, a
    rl      c
    rl      b
    jr      _dzx0s_elias_loop

    __endasm;
}

void dzx0_decompress(const uint8_t *src, uint8_t *dst)
{
    dzx0_src = src;
    dzx0_dst = dst;
    dzx0_run();
}
