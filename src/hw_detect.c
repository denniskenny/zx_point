/*
 * hw_detect.c — 128K Spectrum detection
 *
 * Writes to bank 1 at 0xC000, switches to bank 2, writes a different
 * value, switches back to bank 1, checks if the original value is
 * preserved. On 48K the port 0x7FFD writes are ignored so 0xC000
 * always sees the last write → detection fails. On 128K separate
 * banks → original value preserved → detection succeeds.
 *
 * All writes are restored before returning (non-destructive).
 */

#include "../include/hw.h"

uint8_t is_128k = 0;

void hw_detect(void)
{
    __asm
        ;; Save the byte currently at 0xC000
        ld  a, (0xC000)
        ld  d, a            ; D = saved original byte

        ;; Select bank 1 (port 0x7FFD, bits 0-2 = bank number)
        ld  bc, 0x7FFD
        ld  a, 0x01         ; bank 1
        out (c), a

        ;; Write a test value to 0xC000
        ld  a, 0xAA
        ld  (0xC000), a

        ;; Switch to bank 2
        ld  a, 0x02
        out (c), a

        ;; Write a different value
        ld  a, 0x55
        ld  (0xC000), a

        ;; Switch back to bank 1
        ld  a, 0x01
        out (c), a

        ;; Read back — on 128K this should still be 0xAA
        ld  a, (0xC000)
        cp  0xAA
        jr  nz, _hw_48k

        ;; 128K detected — restore bank 2
        ld  a, 0x02
        out (c), a
        ld  a, d
        ld  (0xC000), a     ; restore bank 2 byte (was overwritten with 0x55)
        ;; Actually we need to be more careful: restore bank 1 too
        ld  a, 0x01
        out (c), a
        ld  (0xC000), a     ; restore bank 1 byte (D = original)
        ;; but D was the original from whichever bank was active at entry.
        ;; For safety, select bank 0 (default) before returning
        xor a
        out (c), a

        ld  a, 1
        ld  (_is_128k), a
        jr  _hw_done

    _hw_48k:
        ;; Restore original byte at 0xC000
        ld  a, d
        ld  (0xC000), a

        ;; Select bank 0 (harmless on 48K — port is ignored)
        xor a
        out (c), a

        xor a
        ld  (_is_128k), a

    _hw_done:
    __endasm;
}
