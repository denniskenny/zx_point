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

/* Play n half-cycles at a given pitch (delay controls frequency).
 * Higher delay = lower pitch.  ~(43 + 32*delay)*n T-states.
 *
 * Approximate delays for musical notes:
 *   G5=138  C6=103  E6=82  G6=68 */
void sfx_play_note(uint8_t n, uint8_t delay) __naked
{
    (void)n; (void)delay;
    __asm
    ld  hl, #2
    add hl, sp
    ld  b, (hl)             ; B = half-cycle count
    inc hl
    ld  h, (hl)             ; H = delay value (pitch)

    ld  c, #254
    ld  a, (_border_val)
    ld  d, a                ; D = border with speaker off
    or  #0x10
    ld  e, a                ; E = border with speaker on

    _spn_loop:
    ld  a, e
    out (c), a
    ld  a, h
    _spn_d1:
    dec a
    jr  nz, _spn_d1
    ld  a, d
    out (c), a
    ld  a, h
    _spn_d2:
    dec a
    jr  nz, _spn_d2
    djnz _spn_loop

    ret
    __endasm;
}

/* Taps-inspired collection jingle: G-G-C, G-C-E
 * Fast ascending arpeggio, total ~140K T-states (~40ms). */
void sfx_collect_jingle(void)
{
    sfx_play_note(18, 138);   /* G5 — dotted */
    sfx_play_note(9,  138);   /* G5 — short  */
    sfx_play_note(30, 103);   /* C6 — held   */
    sfx_play_note(9,  138);   /* G5 — short  */
    sfx_play_note(18, 103);   /* C6 — dotted */
    sfx_play_note(36,  82);   /* E6 — held   */
}

/* Low-pitched Taps — same pattern as collect jingle, ~1 octave lower.
 * Max delay is 255 so G4 is capped there. */
void sfx_damage_jingle(void)
{
    sfx_play_note(18, 255);   /* G4 (capped) — dotted */
    sfx_play_note(9,  255);   /* G4 — short  */
    sfx_play_note(30, 206);   /* C5 — held   */
    sfx_play_note(9,  255);   /* G4 — short  */
    sfx_play_note(18, 206);   /* C5 — dotted */
    sfx_play_note(36, 164);   /* E5 — held   */
}

void beep_tick(void)
{
    if (sfx_step == 0) return;
    if (sfx_wait > 0) { sfx_wait--; return; }

    sfx_play_tone(ping_cycles[sfx_step - 1]);
    sfx_wait = ping_gaps[sfx_step - 1];
    if (++sfx_step > 4) sfx_step = 0;
}
