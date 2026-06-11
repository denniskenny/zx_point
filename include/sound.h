#ifndef _SOUND_H_
#define _SOUND_H_

/* ================================================================== */
/* sound.h — Non-blocking beeper sound effects                        */
/* ================================================================== */

/* Start a sonar ping (tone + 3 echoes, spread across frames). */
void beep_ping_start(void);

/* Advance the sound state machine. Call once per frame. */
void beep_tick(void);

/* Play n half-cycles of the ping tone. ~350n T-states.
 * Used directly for death sequence continuous tone. */
void sfx_play_tone(uint8_t n) __naked;

/* Play n half-cycles at a given pitch (delay controls frequency).
 * Higher delay = lower pitch.  G5=138, C6=103, E6=82, G6=68. */
void sfx_play_note(uint8_t n, uint8_t delay) __naked;

/* Fast Taps-inspired collection jingle (~40ms blocking). */
void sfx_collect_jingle(void);

/* Low-pitched Taps — played on predator damage. */
void sfx_damage_jingle(void);

/* Distance-based sonar ping. Call once per frame with the Chebyshev
 * distance to the nearest target (1-10), or 0/255 for out of range.
 * Manages its own countdown; triggers beep_ping_start when due. */
void sonar_update(uint8_t dist);

#endif /* _SOUND_H_ */
