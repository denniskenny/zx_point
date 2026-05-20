/*
 * main.c — Entry point for ZX Point
 *
 * Detects hardware, then hands off to the state machine.
 */

#include "../include/hw.h"
#include "../include/state.h"

int main(void)
{
    /* Lock 128K paging to 48K-compatible mode.
       Harmless on 48K (port 0x7FFD not decoded). */
    __asm
    ld bc, #0x7FFD
    ld a, #0x30         ; bank 0, screen 0, ROM 1 (48K), lock
    out (c), a
    __endasm;

    hw_detect();
    state_run();
    return 0;
}
