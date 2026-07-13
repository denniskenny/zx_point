/*
 * main.c — Entry point for ZX Point
 *
 * Detects hardware and floating bus before locking paging,
 * then hands off to the state machine.
 */

#include "../include/hw.h"
#include "../include/vsync.h"
#include "../include/state.h"
#include "../include/sprites_packed.h"

int main(void)
{
    /* Decompress all sprite pixels into the low-RAM arena before any state
       draws them (title/level-intro/game all rely on it). */
    sprites_unpack();

    /* Detect hardware BEFORE locking paging — the 128K bank-
       switching test and +2A/+3 floating bus both need paging
       enabled to work. */
    hw_detect();
    vsync_detect();

    /* Lock 128K paging to 48K-compatible mode.
       Harmless on 48K (port 0x7FFD not decoded). */
    __asm
    ld bc, #0x7FFD
    ld a, #0x30         ; bank 0, screen 0, ROM 1 (48K), lock
    out (c), a
    __endasm;

    state_run();
    return 0;
}
