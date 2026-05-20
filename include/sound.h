#ifndef _SOUND_H_
#define _SOUND_H_

/* ================================================================== */
/* sound.h — Non-blocking beeper sound effects                        */
/* ================================================================== */

/* Start a sonar ping (tone + 3 echoes, spread across frames). */
void beep_ping_start(void);

/* Advance the sound state machine. Call once per frame. */
void beep_tick(void);

#endif /* _SOUND_H_ */
