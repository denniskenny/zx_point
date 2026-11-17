#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <stdint.h>

/* ================================================================== */
/* music.h — Frame-paced single-voice beeper melody player            */
/*                                                                    */
/* For static screens only (title, intro briefing, summary, game     */
/* over). Plays the Channel A lead of the design-doc sea shanties on  */
/* the 48K beeper. Playback is blocking and vsync-paced, so it never */
/* overlaps the GAME state (where the sonar ping owns the speaker).   */
/* ================================================================== */

/* One note: delay = pitch (0 = rest, higher = lower pitch, matches
 * sfx_play_note's delay scale), frames = duration in 50 Hz ticks.    */
typedef struct {
    uint8_t delay;
    uint8_t frames;
} music_note_t;

/* Songs (defined in music.c).  The sea shanties are now 3-channel Tritone
 * arrangements (see below); only the short fanfare remains single-voice. */
extern const music_note_t music_fanfare[];        /* level-complete trigger */
extern const uint8_t      music_fanfare_len;

/* Play a song straight through, blocking, with its own vsync pacing.
 * Used for the short level-complete fanfare. */
void music_play_blocking(const music_note_t *song, uint8_t len);

/* ------------------------------------------------------------------ */
/* Three-channel Tritone tunes (Shiru's Tritone v2 engine).           */
/*                                                                    */
/* Each NAME_play() BLOCKS: it loops the tune on all three beeper      */
/* channels and returns only when any key/joystick is pressed.  They  */
/* run with interrupts disabled and leave them disabled.  All tunes    */
/* share ONE engine (assets/music/tritone_engine.asm); each is a data  */
/* module (assets/music/NAME_linkable.asm).  tritone_ticks holds the   */
/* rows played by the LAST tune before its keypress — use it as PRNG   */
/* entropy right after a *_play() returns.                            */
/* ------------------------------------------------------------------ */
void oro_play(void);        /* "Óró Sé do Bheatha 'Bhaile" — title    */
void lowlands_play(void);   /* "Lowlands Away" — level summary        */
void spanish_play(void);    /* "Spanish Ladies" dirge — game over     */
extern uint16_t tritone_ticks;

#endif /* _MUSIC_H_ */
