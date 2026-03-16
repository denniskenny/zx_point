#ifndef _HUD_H_
#define _HUD_H_

/* ================================================================== */
/* hud.h -- HUD display (oxygen bar + health gauge)                   */
/* ================================================================== */

#include <stdint.h>

/* Write bar pixel patterns into row 23 screen RAM */
void hud_init(void);

/* Update bar attributes based on current oxygen and health values */
void hud_draw(uint8_t oxygen, uint8_t health);

#endif /* _HUD_H_ */
