#ifndef _DEPTH_H_
#define _DEPTH_H_

/* ================================================================== */
/* depth.h — Depth layer palette and transition system                */
/* ================================================================== */

#include <stdint.h>

/* Current depth layer (1, 2, or 3) */
extern uint8_t current_depth;

/* Set depth immediately (no animation) — fills ATTR + sets border */
void depth_set(uint8_t layer);

/* Get the resting attribute byte for a depth layer */
uint8_t depth_get_attr(uint8_t layer);

/* Get the resting border colour for a depth layer */
uint8_t depth_get_border(uint8_t layer);

/* Start a non-blocking depth transition (call tick each frame) */
void depth_start_transition(uint8_t from, uint8_t to);

/* Advance transition by one frame. Returns 1 when complete, 0 during. */
uint8_t depth_transition_tick(void);

/* Returns 1 if a transition is in progress */
uint8_t depth_is_transitioning(void);

#endif /* _DEPTH_H_ */
