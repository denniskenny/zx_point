/*
 * bubblefield.c — PRNG, bubble_t, init/update/draw bubbles
 *
 * Public API:
 *   init_bubbles()                         — seed all bubbles
 *   bubbles_set_count(n)                   — change active bubble count
 *   update_and_draw_bubbles(vx, vy, vz)    — one frame update (Z80 asm)
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/prng.h"
#include "../include/gfx.h"
#include "../include/bubblefield.h"

/* --- Bubble type --- */
typedef struct {
    int8_t  x, y;       /* -128..127, wrap naturally */
    uint8_t z;          /* 1..255 (depth, larger = further) */
    uint8_t *paddr;     /* screen byte it was last drawn at; NULL = not drawn */
    uint8_t pmask;      /* far: pixel mask; close: (x & 7) * 2 mask-table offset */
    uint8_t pclose;     /* last drawn as a 4x4 (close) bubble */
} bubble_t;

static bubble_t bubbles[NUM_BUBBLES];
static uint8_t bubble_count = NUM_BUBBLES;  /* active bubbles (can be < NUM_BUBBLES) */

/* ------------------------------------------------------------------ */
/* 16-bit LFSR PRNG (shared algorithm, own stream)                     */
/* ------------------------------------------------------------------ */
static prng_t b_rng = { 0xACE1, 0 };

static uint16_t rng(void)
{
    return prng_next(&b_rng);
}

/* ------------------------------------------------------------------ */
/* Random coordinate helpers                                           */
/* ------------------------------------------------------------------ */
static int16_t rand_xy(void)
{
    return (int16_t)(rng() & 0xFF) - XY_HALF;
}

/* Random depth in the visible range 1 .. BF_Z_CULL-1 */
static int16_t rand_z(void)
{
    return 1 + (int16_t)(((rng() & 0xFF) * (uint16_t)(BF_Z_CULL - 1)) >> 8);
}

/* Erase a bubble at its last drawn position (asm helper below) */
static void erase_bubble(bubble_t *b) __z88dk_fastcall __naked
{
    (void)b;
    __asm
    push ix
    push hl
    pop  ix
    call _sf_erase_cur
    pop  ix
    ret
    __endasm;
}

/* ------------------------------------------------------------------ */
/* Set the active bubble count (capped at NUM_BUBBLES)                 */
/* ------------------------------------------------------------------ */
void bubbles_set_count(uint8_t count)
{
    uint8_t i;

    if (count > NUM_BUBBLES) count = NUM_BUBBLES;

    /* Erase bubbles that are being deactivated */
    for (i = count; i < bubble_count; i++) {
        erase_bubble(&bubbles[i]);
        bubbles[i].paddr = 0;
    }

    bubble_count = count;
}

/* ------------------------------------------------------------------ */
/* Initialise all active bubbles to random positions                   */
/* ------------------------------------------------------------------ */
void init_bubbles(void)
{
    uint8_t i;
    for (i = 0; i < bubble_count; i++) {
        bubbles[i].x = (int8_t)rand_xy();
        bubbles[i].y = (int8_t)rand_xy();
        bubbles[i].z = (uint8_t)rand_z();
        bubbles[i].paddr = 0;
        bubbles[i].pclose = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Reciprocal table: recip_z[z] = floor(32768 / z)                    */
/* Perspective projection: (x * recip_z[z]) >> 8  approx  x * 128 / z */
/* ------------------------------------------------------------------ */
static const uint16_t recip_z[256] = {
        0, 32768, 16384, 10922,  8192,  6553,  5461,  4681,
     4096,  3640,  3276,  2978,  2730,  2520,  2340,  2184,
     2048,  1927,  1820,  1724,  1638,  1560,  1489,  1424,
     1365,  1310,  1260,  1213,  1170,  1129,  1092,  1057,
     1024,   992,   963,   936,   910,   885,   862,   840,
      819,   799,   780,   762,   744,   728,   712,   697,
      682,   668,   655,   642,   630,   618,   606,   595,
      585,   574,   564,   555,   546,   537,   528,   520,
      512,   504,   496,   489,   481,   474,   468,   461,
      455,   448,   442,   436,   431,   425,   420,   414,
      409,   404,   399,   394,   390,   385,   381,   376,
      372,   368,   364,   360,   356,   352,   348,   344,
      341,   337,   334,   330,   327,   324,   321,   318,
      315,   312,   309,   306,   303,   300,   297,   295,
      292,   289,   287,   284,   282,   280,   277,   275,
      273,   270,   268,   266,   264,   262,   260,   258,
      256,   254,   252,   250,   248,   246,   244,   242,
      240,   239,   237,   235,   234,   232,   230,   229,
      227,   225,   224,   222,   221,   219,   218,   217,
      215,   214,   212,   211,   210,   208,   207,   206,
      204,   203,   202,   201,   199,   198,   197,   196,
      195,   193,   192,   191,   190,   189,   188,   187,
      186,   185,   184,   183,   182,   181,   180,   179,
      178,   177,   176,   175,   174,   173,   172,   171,
      170,   169,   168,   168,   167,   166,   165,   164,
      163,   163,   162,   161,   160,   159,   159,   158,
      157,   156,   156,   155,   154,   153,   153,   152,
      151,   151,   150,   149,   148,   148,   147,   146,
      146,   145,   144,   144,   143,   143,   142,   141,
      141,   140,   140,   139,   138,   138,   137,   137,
      136,   135,   135,   134,   134,   133,   133,   132,
      132,   131,   131,   130,   130,   129,   129,   128
};

/* Bitmask table for pixel-within-byte */
static const uint8_t bit_mask[8] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Workspace statics bridging C and asm */
static uint8_t sf_vx, sf_vy, sf_vz;
static uint8_t sf_count;
static uint16_t sf_recip;
static uint8_t sf_sx;
static uint8_t sf_phase;     /* 0 or 1: which half of bubbles to project this frame */
uint8_t sf_cull_y;           /* bubbles with sy < this are culled (0 = no culling) */
uint8_t sf_cull_y_floor;     /* bubbles with sy > this are culled (0 = no culling) */

/* ------------------------------------------------------------------ */
/* Update positions and draw all active bubbles for one frame.         */
/* Full Z80 assembly inner loop — replaces the C version for speed.    */
/*                                                                     */
/* Bubble struct layout (7 bytes):                                     */
/*   +0: x (int8)   +1: y (int8)   +2: z (uint8, 1..255)              */
/*   +3,+4: paddr (screen address; high byte 0 = not drawn)           */
/*   +5: pmask (far: pixel mask, close: mask-table offset)            */
/*   +6: pclose (1 = drawn as 4x4 bubble)                              */
/* ------------------------------------------------------------------ */
void update_and_draw_bubbles(int8_t vx, int8_t vy, int8_t vz) __naked
{
    (void)vx; (void)vy; (void)vz;
    __asm

    ;; ---- Read params from stack (before modifying SP) ----
    ;; __naked: SP+0 = ret addr, SP+2 = vx, SP+3 = vy, SP+4 = vz
    ld  hl, 2
    add hl, sp
    ld  a, (hl)
    ld  (_sf_vx), a
    inc hl
    ld  a, (hl)
    ld  (_sf_vy), a
    inc hl
    ld  a, (hl)
    ld  (_sf_vz), a

    ;; Each bubble updates at 25fps (split-frame), so velocity is already
    ;; effectively halved compared to 50fps. No doubling needed.

    ;; ---- Loop setup ----
    ld  a, (_bubble_count)
    or  a
    ret z

    ;; Toggle frame phase (0→1→0→1...)
    ld  a, (_sf_phase)
    xor 1
    ld  (_sf_phase), a

    ;; Split-frame: this frame handles bubbles phase, phase+2, phase+4...
    ;; (the other half next frame), so each bubble updates at 25 fps.
    ;; count = (bubble_count - phase + 1) / 2
    ld  b, a                    ; B = phase
    ld  a, (_bubble_count)
    sub b
    inc a
    srl a
    ret z
    ld  (_sf_count), a

    ;; Save IX (callee-saved in SDCC convention)
    push ix
    ld  ix, _bubbles
    ld  a, b
    or  a
    jr  z, _sf_loop
    ld  de, 7                   ; phase 1: start at the second bubble
    add ix, de

    ;; ================================================================
    ;; Main loop — one iteration per bubble handled this frame
    ;; IX points to current bubble_t throughout
    ;; ================================================================
_sf_loop:

    ;; ======== UPDATE POSITION ========
    ;; x and y are signed bytes: 8-bit add wraps them within [-128, 127]
    ;; for free. An axis with zero velocity is skipped entirely.

    ld  a, (_sf_vx)             ; x += vx
    or  a
    jr  z, _sf_x_done
    add a, (ix+0)
    ld  (ix+0), a
_sf_x_done:

    ld  a, (_sf_vy)             ; y += vy
    or  a
    jr  z, _sf_y_done
    add a, (ix+1)
    ld  (ix+1), a
_sf_y_done:

    ;; z += vz, z must stay in [1, BF_Z_CULL-1] (the visible depth range):
    ;; moving past the far limit respawns close at 1, at/below 0 respawns
    ;; at the far limit (new random x, y either way). Bubbles never sit in
    ;; the invisible z >= BF_Z_CULL band.
    ld  a, (_sf_vz)
    or  a
    jr  z, _sf_z_ok             ; z unchanged → still in range
    jp  m, _sf_z_dec
    add a, (ix+2)
    jr  c, _sf_z_high           ; carried past 255
    cp  #BF_Z_CULL
    jr  nc, _sf_z_high          ; past the far limit
    ld  (ix+2), a
    jr  _sf_z_ok
_sf_z_dec:
    add a, (ix+2)               ; adding a negative byte: carry = no borrow
    jr  nc, _sf_z_low           ; went below 0
    jr  z, _sf_z_low            ; landed on 0
    ld  (ix+2), a
    jr  _sf_z_ok
_sf_z_high:
    ld  (ix+2), 1
    jr  _sf_z_respawn
_sf_z_low:
    ld  (ix+2), #(BF_Z_CULL - 1)
_sf_z_respawn:
    call _rand_xy
    ld  (ix+0), l
    call _rand_xy
    ld  (ix+1), l

_sf_z_ok:

    ;; ======== ERASE previous screen position ========
    ;; Uses the address/mask stored when it was drawn: no address maths.
    call _sf_erase_cur

    ;; ======== PROJECTION ========

    ;; Pre-screen: if |x| > z or |y| > z, bubble is off-screen.
    ;; Avoids the expensive multiply for ~40-60% of bubbles.
    ld  c, (ix+2)               ; C = z (1-255)

    ld  a, (ix+0)               ; x (signed byte)
    or  a
    jp  p, _sf_psx_pos
    neg
_sf_psx_pos:
    cp  c
    jp  nc, _sf_offscr          ; |x| >= z → off-screen

    ld  a, (ix+1)               ; y (signed byte)
    or  a
    jp  p, _sf_psy_pos
    neg
_sf_psy_pos:
    cp  c
    jp  nc, _sf_offscr          ; |y| >= z → off-screen

    ;; Look up recip_z[z]
    ld  l, c
    ld  h, 0
    add hl, hl                  ; z * 2 (word-sized entries)
    ld  de, _recip_z
    add hl, de                  ; HL = &recip_z[z]
    ld  a, (hl)
    inc hl
    ld  h, (hl)
    ld  l, a                    ; HL = recip value (L=lo, H=hi)
    ld  (_sf_recip), hl

    ;; Project X: sx = (x * recip) >> 8 + 128
    ld  a, (ix+0)               ; x (signed byte)
    call _sf_proj               ; HL = (x * recip) >> 8
    ld  de, 128
    add hl, de
    ;; Check sx: must be in [BF_X_MIN, BF_X_MAX)
    ld  a, h
    or  a
    jp  nz, _sf_offscr
    ld  a, l
    cp  #BF_X_MAX
    jp  nc, _sf_offscr
    cp  #BF_X_MIN
    jp  c, _sf_offscr
    ld  (_sf_sx), a

    ;; Project Y: sy = (y * recip) >> 8 + 96
    ld  a, (ix+1)               ; y (signed byte)
    call _sf_proj
    ld  de, 96
    add hl, de
    ;; Check sy: must be in [0, VIEW_H-1] i.e. [0, 159]
    ld  a, h
    or  a
    jp  nz, _sf_offscr
    ld  a, l
    cp  160
    jp  nc, _sf_offscr
    ;; Cull bubbles above sea line
    ld  a, (_sf_cull_y)
    or  a
    jr  z, _sf_no_cull
    cp  l
    jp  nc, _sf_offscr
_sf_no_cull:
    ;; Cull bubbles below sea floor
    ld  a, (_sf_cull_y_floor)
    or  a
    jr  z, _sf_no_cull_f
    cp  l
    jp  c, _sf_offscr
_sf_no_cull_f:

    ;; ======== PLOT ========
    ;; L = sy, _sf_sx = sx.  Compute the screen address once, remember it
    ;; (plus mask) in the bubble so next frame's erase is a plain XOR.
    ld  d, l                    ; D = sy
    ld  a, (_sf_sx)
    ld  e, a                    ; E = sx
    call _sf_scr_addr           ; HL = screen address of (sx, sy)
    ld  (ix+3), l
    ld  (ix+4), h

    ;; Close bubble depth cue: z < 85 → draw 4x4 bubble
    ld  a, (ix+2)
    cp  85
    jr  nc, _sf_far_plot

    ld  (ix+6), 1               ; pclose = 1
    ld  a, e
    and 0x07
    add a, a                    ; mask-table offset = (x & 7) * 2
    ld  (ix+5), a
    ld  e, a
    call _sf_xor_bubble         ; HL = addr, E = offset
    jr  _sf_next

_sf_far_plot:
    ld  (ix+6), 0               ; pclose = 0
    push hl
    ld  a, e
    and 0x07
    ld  e, a
    ld  d, 0
    ld  hl, _bit_mask
    add hl, de
    ld  a, (hl)                 ; A = pixel mask
    pop hl
    ld  (ix+5), a
    xor (hl)
    ld  (hl), a
    jr  _sf_next

_sf_offscr:
    ld  (ix+4), 0               ; not drawn

_sf_next:
    ;; Advance IX two bubbles (+14 bytes): skip the other frame's half
    ld  de, 14
    add ix, de
    ld  hl, _sf_count
    dec (hl)
    jp  nz, _sf_loop

    pop ix
    ret

    ;; ================================================================
    ;; _sf_erase_cur — XOR-erase bubble IX at its stored address
    ;; Destroys: A, BC, DE, HL.  Preserves: IX.
    ;; ================================================================
_sf_erase_cur:
    ld  h, (ix+4)
    ld  a, h
    or  a
    ret z                       ; not drawn
    ld  l, (ix+3)
    ld  a, (ix+6)
    or  a
    jr  nz, _sf_erase_close
    ld  a, (ix+5)               ; far: single pixel
    xor (hl)
    ld  (hl), a
    ret
_sf_erase_close:
    ld  e, (ix+5)               ; close: mask-table offset
    ;; fall through into _sf_xor_bubble

    ;; ================================================================
    ;; _sf_xor_bubble — XOR the 4x4 bubble at screen address HL
    ;; Input: HL = top-left screen byte, E = (x & 7) * 2
    ;; Pattern  .##. / #..# / #..# / .##.  — masks loaded once into
    ;; B,C (row type A) and D,E (row type B). Both bytes of every row are
    ;; XORed unconditionally (a 0 mask is a no-op; bubbles never reach
    ;; column 31, so the right-hand byte stays on the same row).
    ;; Destroys: A, BC, DE, HL.
    ;; ================================================================
_sf_xor_bubble:
    push hl
    ld  d, 0
    ld  hl, _bub_mask_a
    add hl, de
    ld  b, (hl)
    inc hl
    ld  c, (hl)
    ld  hl, _bub_mask_b
    add hl, de
    ld  d, (hl)
    inc hl
    ld  e, (hl)
    pop hl

    ld  a, (hl)                 ; row 0: .##.
    xor b
    ld  (hl), a
    inc l
    ld  a, (hl)
    xor c
    ld  (hl), a
    dec l
    inc h
    ld  a, h
    and 0x07
    call z, _sf_row_fix

    ld  a, (hl)                 ; row 1: #..#
    xor d
    ld  (hl), a
    inc l
    ld  a, (hl)
    xor e
    ld  (hl), a
    dec l
    inc h
    ld  a, h
    and 0x07
    call z, _sf_row_fix

    ld  a, (hl)                 ; row 2: #..#
    xor d
    ld  (hl), a
    inc l
    ld  a, (hl)
    xor e
    ld  (hl), a
    dec l
    inc h
    ld  a, h
    and 0x07
    call z, _sf_row_fix

    ld  a, (hl)                 ; row 3: .##.
    xor b
    ld  (hl), a
    inc l
    ld  a, (hl)
    xor c
    ld  (hl), a
    ret

    ;; HL crossed a character-row boundary after INC H: fix up (rare)
_sf_row_fix:
    ld  a, h
    sub 8
    ld  h, a
    ld  a, l
    add a, 32
    ld  l, a
    ret nc
    ld  a, h
    add a, 8
    ld  h, a
    ret

    ;; ================================================================
    ;; Bubble row mask tables.  Indexed by (x & 7) * 2:
    ;; [left_byte, right_byte], MSB = leftmost pixel.
    ;; ================================================================
_bub_mask_a:                    ;; Row A (.##.): pixels at x+1, x+2
    .db 0x60, 0x00
    .db 0x30, 0x00
    .db 0x18, 0x00
    .db 0x0C, 0x00
    .db 0x06, 0x00
    .db 0x03, 0x00
    .db 0x01, 0x80
    .db 0x00, 0xC0
_bub_mask_b:                    ;; Row B (#..#): pixels at x+0, x+3
    .db 0x90, 0x00
    .db 0x48, 0x00
    .db 0x24, 0x00
    .db 0x12, 0x00
    .db 0x09, 0x00
    .db 0x04, 0x80
    .db 0x02, 0x40
    .db 0x01, 0x20

    ;; ================================================================
    ;; _sf_scr_addr — screen address for pixel (E, D)
    ;; Input:  E = x, D = y.  Output: HL.  Destroys: A, B.  Keeps DE.
    ;; ================================================================
_sf_scr_addr:
    ld  a, d
    and 0xC0
    rrca
    rrca
    rrca
    ld  b, a
    ld  a, d
    and 0x07
    or  b
    or  0x40
    ld  h, a
    ld  a, d
    and 0x38
    rlca
    rlca
    ld  b, a
    ld  a, e
    rrca
    rrca
    rrca
    and 0x1F
    or  b
    ld  l, a
    ret

    ;; ================================================================
    ;; _sf_proj — signed 8-bit x unsigned 16-bit perspective projection
    ;;
    ;; Computes: (A * recip) >> 8   where A is signed [-128..127]
    ;;           and recip is the 16-bit value stored in _sf_recip.
    ;;
    ;; Uses 8x16→24 bit multiply (unrolled shift-and-add, MSB first).
    ;; Result is the middle two bytes (bits 8-23) of the 24-bit product.
    ;;
    ;; Input:  A = signed coordinate value
    ;;         _sf_recip must hold recip_z[z] for current bubble
    ;; Output: HL = (A * recip) >> 8 (signed 16-bit)
    ;; Destroys: A, BC, DE.  Preserves: IX.
    ;; ================================================================
_sf_proj:
    ld  c, a                    ; C = original value (sign in bit 7)
    or  a
    jp  p, _sf_pj_pos
    neg                         ; A = |val|
_sf_pj_pos:
    ld  hl, (_sf_recip)
    ex  de, hl                  ; DE = recip (E=lo, D=hi), A = |val| preserved
    inc d
    dec d                       ; Z iff recip high byte == 0
    jr  z, _sf_proj_8

    ;; 8x16 multiply: A x DE -> B:HL (24-bit unsigned)
    ld  b, 0
    ld  hl, 0

    add a, a
    jr  nc, _sf_pj_s6
    add hl, de
    jr  nc, _sf_pj_s6
    inc b
_sf_pj_s6:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s5
    add hl, de
    jr  nc, _sf_pj_s5
    inc b
_sf_pj_s5:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s4
    add hl, de
    jr  nc, _sf_pj_s4
    inc b
_sf_pj_s4:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s3
    add hl, de
    jr  nc, _sf_pj_s3
    inc b
_sf_pj_s3:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s2
    add hl, de
    jr  nc, _sf_pj_s2
    inc b
_sf_pj_s2:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s1
    add hl, de
    jr  nc, _sf_pj_s1
    inc b
_sf_pj_s1:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_s0
    add hl, de
    jr  nc, _sf_pj_s0
    inc b
_sf_pj_s0:
    add hl, hl
    rl  b
    add a, a
    jr  nc, _sf_pj_md
    add hl, de
    jr  nc, _sf_pj_md
    inc b
_sf_pj_md:
    ld  l, h
    ld  h, b
    jr  _sf_pj_sign

    ;; ---- Fast 8x8 path (recip fits in 8 bits, D==0) ----
_sf_proj_8:
    ld  h, d                    ; 0
    ld  l, d                    ; 0
    add a, a
    jr  nc, _p8_6
    ld  l, e
_p8_6:
    add hl, hl
    add a, a
    jr  nc, _p8_5
    add hl, de
_p8_5:
    add hl, hl
    add a, a
    jr  nc, _p8_4
    add hl, de
_p8_4:
    add hl, hl
    add a, a
    jr  nc, _p8_3
    add hl, de
_p8_3:
    add hl, hl
    add a, a
    jr  nc, _p8_2
    add hl, de
_p8_2:
    add hl, hl
    add a, a
    jr  nc, _p8_1
    add hl, de
_p8_1:
    add hl, hl
    add a, a
    jr  nc, _p8_0
    add hl, de
_p8_0:
    add hl, hl
    add a, a
    jr  nc, _p8_dn
    add hl, de
_p8_dn:
    ld  l, h
    ld  h, d                    ; D still 0

_sf_pj_sign:
    bit 7, c
    ret z
    ld  a, l
    cpl
    ld  l, a
    ld  a, h
    cpl
    ld  h, a
    inc hl
    ret

    __endasm;
}
