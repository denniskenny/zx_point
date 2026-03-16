#ifndef _HW_H_
#define _HW_H_

/* ================================================================== */
/* hw.h — Hardware detection (128K)                                   */
/* ================================================================== */

#include <stdint.h>

/* Detect whether we're on a 128K Spectrum. Sets is_128k. */
void hw_detect(void);

/* 1 if 128K detected, 0 otherwise */
extern uint8_t is_128k;

#endif /* _HW_H_ */
