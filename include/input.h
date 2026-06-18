#ifndef _INPUT_H_
#define _INPUT_H_

/* ================================================================== */
/* input.h — Keyboard / joystick reading                              */
/* ================================================================== */

#include <stdint.h>

/* Read a ZX Spectrum keyboard half-row or joystick port.
 * fastcall: 16-bit port in HL, result in L. */
uint8_t read_keys(uint16_t port) __z88dk_fastcall __naked;

/* Packed direction bits returned by scan_input() (active-high). */
#define INPUT_FWD    0x01
#define INPUT_BACK   0x02
#define INPUT_LEFT   0x04
#define INPUT_RIGHT  0x08
#define INPUT_DESC   0x10
#define INPUT_ASC    0x20

/* Read all keyboard half-rows + Kempston in one call.
 * Returns packed direction byte (see INPUT_* defines). */
uint8_t scan_input(void) __naked;

#endif /* _INPUT_H_ */
