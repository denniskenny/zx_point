#ifndef _STATE_H_
#define _STATE_H_

/* ================================================================== */
/* state.h — State machine enum and dispatcher                        */
/* ================================================================== */

typedef enum {
    STATE_TITLE,
    STATE_INTRO,
    STATE_GAME,
    STATE_SUMMARY,
    STATE_GAMEOVER,
    STATE_COUNT
} game_state_t;

/* Run the state machine (never returns) */
void state_run(void);

#endif /* _STATE_H_ */
