/*
 * bubblefield.c — PRNG, bubble_t, init/update/draw bubbles
 *
 * Public API:
 *   init_bubbles()                         — seed all bubbles
 *   bubbles_set_count(n)                   — change active bubble count
 *   bubbles_erase_all()                    — erase visible bubbles from screen
 *   update_and_draw_bubbles(vx, vy, vz)    — one frame update (Z80 asm)
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/gfx.h"

/* --- Bubble type --- */
typedef struct {
    int16_t x, y, z;
    uint8_t psx, psy;   /* previous screen coords (psy=255 = not visible) */
    uint8_t pclose;      /* previous frame was 4x4 bubble */
} bubble_t;

#define PSY_NONE 255

static bubble_t bubbles[NUM_BUBBLES];
static uint8_t bubble_count = NUM_BUBBLES;  /* active bubbles (can be < NUM_BUBBLES) */

/* ------------------------------------------------------------------ */
/* 16-bit LFSR PRNG                                                    */
/* ------------------------------------------------------------------ */
static uint16_t lfsr = 0xACE1;
static uint16_t weyl = 0;

static uint16_t rng(void)
{
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^
                    (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);
    weyl += 0x9E35;
    return lfsr ^ weyl;
}

/* ------------------------------------------------------------------ */
/* Random coordinate helpers                                           */
/* ------------------------------------------------------------------ */
static int16_t rand_xy(void)
{
    return (int16_t)(rng() & 0xFF) - XY_HALF;
}

static int16_t rand_z(void)
{
    int16_t v = (int16_t)(rng() & 0xFF);
    return v ? v : 1;
}

/* ------------------------------------------------------------------ */
/* Erase a bubble at its previous screen position                      */
/* ------------------------------------------------------------------ */
static void erase_bubble(uint8_t *buf, uint8_t px, uint8_t py, uint8_t pclose)
{
    if (pclose) {
        unplot(buf, px + 1, py);
        unplot(buf, px + 2, py);
        unplot(buf, px,     py + 1);
        unplot(buf, px + 3, py + 1);
        unplot(buf, px,     py + 2);
        unplot(buf, px + 3, py + 2);
        unplot(buf, px + 1, py + 3);
        unplot(buf, px + 2, py + 3);
    } else {
        unplot(buf, px, py);
    }
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
        if (bubbles[i].psy != PSY_NONE)
            erase_bubble(SCREEN, bubbles[i].psx, bubbles[i].psy,
                       bubbles[i].pclose);
        bubbles[i].psy = PSY_NONE;
    }

    bubble_count = count;
}

/* ------------------------------------------------------------------ */
/* Erase all currently visible bubbles from the screen                 */
/* ------------------------------------------------------------------ */
void bubbles_erase_all(void)
{
    uint8_t i;
    for (i = 0; i < bubble_count; i++) {
        if (bubbles[i].psy != PSY_NONE) {
            erase_bubble(SCREEN, bubbles[i].psx, bubbles[i].psy,
                       bubbles[i].pclose);
            bubbles[i].psy = PSY_NONE;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Initialise all active bubbles to random positions                   */
/* ------------------------------------------------------------------ */
void init_bubbles(void)
{
    uint8_t i;
    for (i = 0; i < bubble_count; i++) {
        bubbles[i].x = rand_xy();
        bubbles[i].y = rand_xy();
        bubbles[i].z = rand_z();
        bubbles[i].psy = PSY_NONE;
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
static uint8_t sf_parity;    /* per-bubble: toggles 0/1 each iteration */
uint8_t sf_cull_y;           /* bubbles with sy < this are culled (0 = no culling) */
uint8_t sf_cull_y_floor;     /* bubbles with sy > this are culled (0 = no culling) */

/* ------------------------------------------------------------------ */
/* Update positions and draw all active bubbles for one frame.         */
/* Full Z80 assembly inner loop — replaces the C version for speed.    */
/*                                                                     */
/* Bubble struct layout (9 bytes, little-endian):                      */
/*   +0,+1: x (int16_t)   +2,+3: y (int16_t)   +4,+5: z (int16_t)   */
/*   +6: psx (uint8_t)    +7: psy (uint8_t)     +8: pclose (uint8_t)  */
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
    ld  (_sf_parity), a         ; first bubble uses this phase value

    ;; Save IX (callee-saved in SDCC convention)
    push ix
    ld  a, (_bubble_count)
    ld  (_sf_count), a
    ld  ix, _bubbles

    ;; ================================================================
    ;; Main loop — one iteration per bubble
    ;; IX points to current bubble_t throughout
    ;; Split-frame: only project+draw bubbles whose parity matches sf_phase
    ;; ================================================================
_sf_loop:

    ;; ======== SPLIT-FRAME CHECK (first — off-phase bubbles skip entirely) ========
    ld  a, (_sf_parity)
    xor 1
    ld  (_sf_parity), a
    ld  b, a
    ld  a, (_sf_phase)
    cp  b
    jp  nz, _sf_next

    ;; ======== UPDATE POSITION ========

    ;; x += sign_extend(vx)
    ld  l, (ix+0)
    ld  h, (ix+1)
    ld  a, (_sf_vx)
    ld  e, a
    rla                         ; sign bit → carry
    sbc a, a                    ; A = 0xFF if neg, 0x00 if pos
    ld  d, a                    ; DE = sign-extended vx
    add hl, de
    ld  (ix+0), l
    ld  (ix+1), h

    ;; X wrap: keep x in [-128, +127]
    ld  a, h
    or  a
    jr  z, _sf_xh0
    inc a
    jr  nz, _sf_x_done         ; h not 0x00 or 0xFF — skip
    ;; h == 0xFF: x might be < -128
    bit 7, l
    jr  nz, _sf_x_done         ; l >= 0x80 → x in [-128,-1], OK
    ld  (ix+1), 0               ; x < -128 → wrap up (+256)
    jr  _sf_x_done
_sf_xh0:
    ;; h == 0x00: x might be >= 128
    bit 7, l
    jr  z, _sf_x_done          ; l < 0x80 → x in [0,127], OK
    ld  (ix+1), 0xFF            ; x >= 128 → wrap down (-256)
_sf_x_done:

    ;; y += sign_extend(vy)
    ld  l, (ix+2)
    ld  h, (ix+3)
    ld  a, (_sf_vy)
    ld  e, a
    rla
    sbc a, a
    ld  d, a
    add hl, de
    ld  (ix+2), l
    ld  (ix+3), h

    ;; Y wrap
    ld  a, h
    or  a
    jr  z, _sf_yh0
    inc a
    jr  nz, _sf_y_done
    bit 7, l
    jr  nz, _sf_y_done
    ld  (ix+3), 0
    jr  _sf_y_done
_sf_yh0:
    bit 7, l
    jr  z, _sf_y_done
    ld  (ix+3), 0xFF
_sf_y_done:

    ;; z += sign_extend(vz)
    ld  l, (ix+4)
    ld  h, (ix+5)
    ld  a, (_sf_vz)
    ld  e, a
    rla
    sbc a, a
    ld  d, a
    add hl, de
    ld  (ix+4), l
    ld  (ix+5), h

    ;; ======== Z BOUNDS CHECK ========
    ;; z must be in [1, 255]. HL = updated z.
    ld  a, h
    or  a
    jr  z, _sf_z_chk_lo
    bit 7, a
    jr  nz, _sf_z_low          ; h < 0 → z negative
    ;; h > 0: z > 255 → respawn at Z_MIN
    ld  (ix+4), 1
    ld  (ix+5), 0
    jr  _sf_z_respawn
_sf_z_chk_lo:
    ld  a, l
    or  a
    jr  nz, _sf_z_ok           ; z in [1,255] → skip respawn
    ;; z == 0 → treat as too low
_sf_z_low:
    ld  (ix+4), 255
    ld  (ix+5), 0
_sf_z_respawn:
    call _rand_xy
    ld  (ix+0), l
    ld  (ix+1), h
    call _rand_xy
    ld  (ix+2), l
    ld  (ix+3), h

_sf_z_ok:

    ;; ======== ERASE previous screen position ========
    ld  a, (ix+7)
    inc a                       ; psy was 255 (PSY_NONE)? → A wraps to 0
    jr  z, _sf_no_erase
    ld  a, (ix+8)               ; pclose
    or  a
    jr  nz, _sf_erase_bubble

    ;; Far bubble: single pixel
    ld  d, (ix+7)
    ld  e, (ix+6)
    call _sf_unplot
    jr  _sf_no_erase

_sf_erase_bubble:
    ;; 4x4 bubble erase
    ld  e, (ix+6)
    ld  d, (ix+7)
    call _sf_erase_bubble_fn

_sf_no_erase:

    ;; ======== PROJECTION ========

    ;; Pre-screen: if |x| > z or |y| > z, bubble is off-screen.
    ;; Avoids the expensive multiply for ~40-60% of bubbles.
    ld  c, (ix+4)               ; C = z (1-255)

    ld  a, (ix+0)               ; x low byte (signed)
    or  a
    jp  p, _sf_psx_pos
    neg
_sf_psx_pos:
    cp  c
    jp  nc, _sf_offscr          ; |x| >= z → off-screen

    ld  a, (ix+2)               ; y low byte (signed)
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
    ld  a, (ix+0)               ; x low byte (signed, range [-128,127])
    call _sf_proj               ; HL = (x * recip) >> 8
    ld  de, 128
    add hl, de
    ;; Check sx: must have H==0 (sx in [0,255])
    ld  a, h
    or  a
    jp  nz, _sf_offscr
    ld  a, l
    ld  (_sf_sx), a

    ;; Project Y: sy = (y * recip) >> 8 + 96
    ld  a, (ix+2)               ; y low byte (signed)
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
    ld  d, l                    ; D = sy
    ld  a, (_sf_sx)
    ld  e, a                    ; E = sx
    ld  (ix+6), e               ; psx = sx
    ld  (ix+7), d               ; psy = sy

    ;; Close bubble depth cue: z < 85 → draw 4x4 bubble
    ld  a, (ix+4)
    cp  85
    jr  nc, _sf_far_plot

    ld  (ix+8), 1               ; pclose = 1

    ;; 4x4 bubble plot — row-at-a-time with precomputed masks
    ;; E = sx, D = sy (already loaded above)
    call _sf_plot_bubble_fn

    jr  _sf_next

_sf_far_plot:
    ld  (ix+8), 0               ; pclose = 0
    ld  d, (ix+7)
    ld  e, (ix+6)
    call _sf_plot
    jr  _sf_next

_sf_offscr:
    ld  (ix+7), 255             ; psy = PSY_NONE

_sf_next:
    ;; Advance IX to next bubble (+9 bytes)
    ld  de, 9
    add ix, de
    ld  a, (_sf_count)
    dec a
    ld  (_sf_count), a
    jp  nz, _sf_loop

    pop ix
    ret

    ;; ================================================================
    ;; _sf_plot — set pixel at (E, D) on screen
    ;; Input:  E = x (0-255), D = y (0-191)
    ;; Destroys: A, BC, HL.  Preserves: DE, IX.
    ;; ================================================================
_sf_plot:
    ;; Screen address high byte: 0x40 | ((y>>3)&0x18) | (y&7)
    ld  a, d
    and 0xC0
    rrca
    rrca
    rrca                        ; (y & 0xC0) >> 3
    ld  b, a
    ld  a, d
    and 0x07
    or  b
    or  0x40
    ld  h, a

    ;; Screen address low byte: ((y&0x38)<<2) | (x>>3)
    ld  a, d
    and 0x38
    rlca
    rlca                        ; (y & 0x38) << 2
    ld  b, a
    ld  a, e
    rrca
    rrca
    rrca
    and 0x1F                    ; x >> 3
    or  b
    ld  l, a

    ;; Bitmask: bit_mask[x & 7]
    ld  a, e
    and 0x07
    ld  c, a
    ld  b, 0
    push hl
    ld  hl, _bit_mask
    add hl, bc
    ld  a, (hl)
    pop hl

    ;; Set pixel
    or  (hl)
    ld  (hl), a
    ret

    ;; ================================================================
    ;; _sf_unplot — clear pixel at (E, D) on screen
    ;; Input:  E = x (0-255), D = y (0-191)
    ;; Destroys: A, BC, HL.  Preserves: DE, IX.
    ;; ================================================================
_sf_unplot:
    ;; Screen address high byte
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

    ;; Screen address low byte
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

    ;; Bitmask (inverted for clear)
    ld  a, e
    and 0x07
    ld  c, a
    ld  b, 0
    push hl
    ld  hl, _bit_mask
    add hl, bc
    ld  a, (hl)
    pop hl

    cpl
    and (hl)
    ld  (hl), a
    ret

    ;; ================================================================
    ;; Bubble row mask tables.
    ;; Pattern:  .##. / #..# / #..# / .##.
    ;; Indexed by (x & 7).  Two bytes per entry: [left_byte, right_byte].
    ;; Bits are MSB-first (bit 7 = leftmost pixel).
    ;; ================================================================

    ;; Row A (.##.):  pixels at x+1, x+2
_bub_mask_a:
    .db 0x60, 0x00   ;; x&7=0
    .db 0x30, 0x00   ;; x&7=1
    .db 0x18, 0x00   ;; x&7=2
    .db 0x0C, 0x00   ;; x&7=3
    .db 0x06, 0x00   ;; x&7=4
    .db 0x03, 0x00   ;; x&7=5
    .db 0x01, 0x80   ;; x&7=6
    .db 0x00, 0xC0   ;; x&7=7

    ;; Row B (#..#):  pixels at x+0, x+3
_bub_mask_b:
    .db 0x90, 0x00   ;; x&7=0
    .db 0x48, 0x00   ;; x&7=1
    .db 0x24, 0x00   ;; x&7=2
    .db 0x12, 0x00   ;; x&7=3
    .db 0x09, 0x00   ;; x&7=4
    .db 0x04, 0x80   ;; x&7=5
    .db 0x02, 0x40   ;; x&7=6
    .db 0x01, 0x20   ;; x&7=7

_sf_bub_off: .ds 1   ;; scratch: (x & 7) * 2

    ;; ================================================================
    ;; _sf_scr_addr — compute screen address for pixel (E, D)
    ;; Input:  E = x, D = y
    ;; Output: HL = screen address
    ;; Destroys: A, B
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
    ;; _sf_next_row — advance HL to the next pixel row
    ;; ================================================================
_sf_next_row:
    inc h
    ld  a, h
    and 0x07
    ret nz
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
    ;; _sf_plot_bubble_fn — draw 4x4 bubble at (E=x, D=y)
    ;; Input:  E = x, D = y
    ;; Destroys: A, B, C, D, E, H, L.  Preserves: IX.
    ;; ================================================================
_sf_plot_bubble_fn:
    ld  a, e
    and 0x07
    add a, a
    ld  (_sf_bub_off), a

    call _sf_scr_addr           ; HL = screen addr for (x, y)

    ;; --- Row 0: type A (.##.) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_a
    add a, e
    ld  e, a
    jr  nc, _pb_r0n
    inc d
_pb_r0n:
    ld  a, (de)
    or  a
    jr  z, _pb_r0r
    or  (hl)
    ld  (hl), a
_pb_r0r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _pb_r0d
    inc l
    or  (hl)
    ld  (hl), a
    dec l
_pb_r0d:
    call _sf_next_row

    ;; --- Row 1: type B (#..#) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_b
    add a, e
    ld  e, a
    jr  nc, _pb_r1n
    inc d
_pb_r1n:
    ld  a, (de)
    or  a
    jr  z, _pb_r1r
    or  (hl)
    ld  (hl), a
_pb_r1r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _pb_r1d
    inc l
    or  (hl)
    ld  (hl), a
    dec l
_pb_r1d:
    call _sf_next_row

    ;; --- Row 2: type B (#..#) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_b
    add a, e
    ld  e, a
    jr  nc, _pb_r2n
    inc d
_pb_r2n:
    ld  a, (de)
    or  a
    jr  z, _pb_r2r
    or  (hl)
    ld  (hl), a
_pb_r2r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _pb_r2d
    inc l
    or  (hl)
    ld  (hl), a
    dec l
_pb_r2d:
    call _sf_next_row

    ;; --- Row 3: type A (.##.) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_a
    add a, e
    ld  e, a
    jr  nc, _pb_r3n
    inc d
_pb_r3n:
    ld  a, (de)
    or  a
    jr  z, _pb_r3r
    or  (hl)
    ld  (hl), a
_pb_r3r:
    inc de
    ld  a, (de)
    or  a
    ret z
    inc l
    or  (hl)
    ld  (hl), a
    ret

    ;; ================================================================
    ;; _sf_erase_bubble_fn — erase 4x4 bubble at (E=x, D=y)
    ;; Input:  E = x, D = y
    ;; Destroys: A, B, C, D, E, H, L.  Preserves: IX.
    ;; ================================================================
_sf_erase_bubble_fn:
    ld  a, e
    and 0x07
    add a, a
    ld  (_sf_bub_off), a

    call _sf_scr_addr

    ;; --- Row 0: type A (.##.) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_a
    add a, e
    ld  e, a
    jr  nc, _eb_r0n
    inc d
_eb_r0n:
    ld  a, (de)
    or  a
    jr  z, _eb_r0r
    cpl
    and (hl)
    ld  (hl), a
_eb_r0r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _eb_r0d
    inc l
    cpl
    and (hl)
    ld  (hl), a
    dec l
_eb_r0d:
    call _sf_next_row

    ;; --- Row 1: type B (#..#) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_b
    add a, e
    ld  e, a
    jr  nc, _eb_r1n
    inc d
_eb_r1n:
    ld  a, (de)
    or  a
    jr  z, _eb_r1r
    cpl
    and (hl)
    ld  (hl), a
_eb_r1r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _eb_r1d
    inc l
    cpl
    and (hl)
    ld  (hl), a
    dec l
_eb_r1d:
    call _sf_next_row

    ;; --- Row 2: type B (#..#) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_b
    add a, e
    ld  e, a
    jr  nc, _eb_r2n
    inc d
_eb_r2n:
    ld  a, (de)
    or  a
    jr  z, _eb_r2r
    cpl
    and (hl)
    ld  (hl), a
_eb_r2r:
    inc de
    ld  a, (de)
    or  a
    jr  z, _eb_r2d
    inc l
    cpl
    and (hl)
    ld  (hl), a
    dec l
_eb_r2d:
    call _sf_next_row

    ;; --- Row 3: type A (.##.) ---
    ld  a, (_sf_bub_off)
    ld  de, _bub_mask_a
    add a, e
    ld  e, a
    jr  nc, _eb_r3n
    inc d
_eb_r3n:
    ld  a, (de)
    or  a
    jr  z, _eb_r3r
    cpl
    and (hl)
    ld  (hl), a
_eb_r3r:
    inc de
    ld  a, (de)
    or  a
    ret z
    inc l
    cpl
    and (hl)
    ld  (hl), a
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
