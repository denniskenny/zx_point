/*
 * sound.c — Non-blocking beeper sound effects
 *
 * The sonar ping is split across frames:
 *   Frame 0: 30 half-cycles (main tone)   ~10,500 T
 *   Frames 1-2: silence                   (gap 1)
 *   Frame 3: 18 half-cycles (echo 1)      ~6,300 T
 *   Frames 4-7: silence                   (gap 2)
 *   Frame 8: 10 half-cycles (echo 2)      ~3,500 T
 *   Frames 9-13: silence                  (gap 3)
 *   Frame 14: 5 half-cycles (echo 3)      ~1,750 T
 *
 * Worst-case cost: ~10,500 T in a tone frame (15% of budget).
 * Port 254 bit layout: bit 4 = speaker, bits 0-2 = border colour.
 */

#include <stdint.h>
#include "../include/sound.h"

extern uint8_t border_val;

/* --- State machine --- */
static uint8_t sfx_step;   /* 0=idle, 1-4 = next tone phase */
static uint8_t sfx_wait;   /* frames to wait before next phase */

/* Half-cycles per phase, silence frames after each phase */
static const uint8_t ping_cycles[] = { 30, 18, 10, 5 };
static const uint8_t ping_gaps[]   = {  2,  4,  5, 0 };

void beep_ping_start(void)
{
    sfx_step = 1;
    sfx_wait = 0;
}

/* Play n half-cycles at ping frequency (~5 kHz). ~350n T-states. */
void sfx_play_tone(uint8_t n) __naked
{
    (void)n;
    __asm
    ld  hl, #2
    add hl, sp
    ld  b, (hl)             ; B = half-cycle count

    ld  c, #254
    ld  a, (_border_val)
    ld  d, a                ; D = border with speaker off
    or  #0x10
    ld  e, a                ; E = border with speaker on

    _spt_loop:
    ld  a, e
    out (c), a
    ld  a, #20
    _spt_d1:
    dec a
    jr  nz, _spt_d1
    ld  a, d
    out (c), a
    ld  a, #20
    _spt_d2:
    dec a
    jr  nz, _spt_d2
    djnz _spt_loop

    ret
    __endasm;
}

void beep_tick(void)
{
    if (sfx_step == 0) return;
    if (sfx_wait > 0) { sfx_wait--; return; }

    sfx_play_tone(ping_cycles[sfx_step - 1]);
    sfx_wait = ping_gaps[sfx_step - 1];
    if (++sfx_step > 4) sfx_step = 0;
}
