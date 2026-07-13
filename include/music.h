#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <stdint.h>

/* ================================================================== */
/* music.h — Frame-paced single-voice beeper melody player            */
/*                                                                    */
/* For static screens only (title, intro briefing, summary, game     */
/* over). Plays the Channel A lead of the design-doc sea shanties on  */
/* the 48K beeper. Advances one step per music_tick(); a tick must be */
/* driven from a screen's per-frame loop, so the melody only plays    */
/* while that loop runs. NOT interrupt-driven — safe with -startup=31 */
/* (interrupts disabled) and incompatible with the GAME state, where  */
/* the sonar ping owns the speaker.                                   */
/* ================================================================== */

/* One note: delay = pitch (0 = rest, higher = lower pitch, matches
 * sfx_play_note's delay scale), frames = duration in 50 Hz ticks.    */
typedef struct {
    uint8_t delay;
    uint8_t frames;
} music_note_t;

/* Songs (defined in music.c). */
extern const music_note_t music_oro[];           /* title / intro shanty   */
extern const uint8_t      music_oro_len;
extern const music_note_t music_lowlands[];       /* level-complete shanty  */
extern const uint8_t      music_lowlands_len;
extern const music_note_t music_spanish[];        /* game-over dirge        */
extern const uint8_t      music_spanish_len;
extern const music_note_t music_fanfare[];        /* level-complete trigger */
extern const uint8_t      music_fanfare_len;

/* Begin playing a song. loop != 0 restarts at the end. */
void music_start(const music_note_t *song, uint8_t len, uint8_t loop);

/* Advance the player one frame. Call once per frame from a screen loop. */
void music_tick(void);

/* Silence and stop playback. */
void music_stop(void);

/* Play a song straight through, blocking, with its own vsync pacing.
 * Used for the short level-complete fanfare. */
void music_play_blocking(const music_note_t *song, uint8_t len);

/* ------------------------------------------------------------------ */
/* Three-channel Tritone arrangement of "Óró Sé do Bheatha 'Bhaile"   */
/* (assets/music/oro_linkable.asm, Shiru's Tritone v2 engine).        */
/*                                                                    */
/* oro_play() BLOCKS: it loops the shanty on all three beeper         */
/* channels and returns only when any key/joystick is pressed. It     */
/* runs with interrupts disabled and leaves them disabled. oro_ticks  */
/* holds the number of rows played before the keypress — use it as    */
/* PRNG entropy after oro_play() returns.                             */
/* ------------------------------------------------------------------ */
void oro_play(void);
extern uint16_t oro_ticks;

#endif /* _MUSIC_H_ */
