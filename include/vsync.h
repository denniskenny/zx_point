#ifndef _VSYNC_H_
#define _VSYNC_H_

/* ================================================================== */
/* vsync.h — Vsync / flicker avoidance                                */
/* ================================================================== */

/* Wait for the vertical blanking interval.
 * Uses the floating bus technique when available,
 * falls back to HALT for emulators that don't support it.
 * Safe on both 48K and 128K. */
void vsync_wait(void) __naked;

#endif /* _VSYNC_H_ */
