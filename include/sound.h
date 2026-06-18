#ifndef _SOUND_H_
#define _SOUND_H_

/* ================================================================== */
/* sound.h — Non-blocking beeper sound effects                        */
/* ================================================================== */

/* Sonar target types (higher = lower pitch) */
#define SONAR_TREASURE  0
#define SONAR_PREDATOR  1
#define SONAR_GOO       2

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
 * distance to the nearest same-depth target (1-10), or 0/255 for none.
 * type: SONAR_TREASURE, SONAR_PREDATOR, or SONAR_GOO — controls pitch. */
void sonar_update(uint8_t dist, uint8_t type);

#endif /* _SOUND_H_ */
