#ifndef _INPUT_H_
#define _INPUT_H_

/* ================================================================== */
/* input.h — Keyboard / joystick reading                              */
/* ================================================================== */

#include <stdint.h>

/* Read a ZX Spectrum keyboard half-row or joystick port.
 * fastcall: 16-bit port in HL, result in L. */
uint8_t read_keys(uint16_t port) __z88dk_fastcall __naked;

#endif /* _INPUT_H_ */
