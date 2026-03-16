/*
 * input.c — Keyboard / joystick reading via naked asm
 *
 * Extracted from starfield.c lines 69-79.
 */

#include "../include/input.h"

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
