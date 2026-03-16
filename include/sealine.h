#ifndef _SEALINE_H_
#define _SEALINE_H_

/* ================================================================== */
/* sealine.h — Sea line (sinewave) and sea floor renderers            */
/* ================================================================== */

#include <stdint.h>

/* Initialise the sea line at its default position (2 char rows above centre) */
void sealine_init(void);

/* Erase previous frame's sea line pixels, advance phase, draw new line.
 * base_y is the vertical centre of the sinewave (scrolls with player). */
void sealine_update(uint8_t base_y);

/* Erase the sea line without redrawing (used on depth change) */
void sealine_erase(void);

/* Get the current cull y-coordinate.
 * Stars and rays with sy < this value should not be drawn. */
uint8_t sealine_get_cull_y(void);

/* Returns 1 if the sea line is currently visible on screen */
uint8_t sealine_is_visible(void);

/* --- Sea floor --- */

/* Initialise the sea floor */
void seafloor_init(void);

/* Draw the sea floor at the given y-coordinate */
void seafloor_update(uint8_t y);

/* Erase the sea floor without redrawing */
void seafloor_erase(void);

/* Get the sea floor y-coordinate (for clipping) */
uint8_t seafloor_get_y(void);

/* Returns 1 if the sea floor is currently visible */
uint8_t seafloor_is_visible(void);

#endif /* _SEALINE_H_ */
