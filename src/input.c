/*
 * input.c — Keyboard / joystick reading via naked asm
 *
 * Extracted from bubblefield.c.
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
