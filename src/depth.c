/*
 * depth.c — Depth layer attribute system
 *
 * Three depth palettes with animated transitions (~50 frames = 1s).
 * Attribute byte format: F_B_PPP_III (flash, bright, paper, ink).
 *
 * Depth 1: Paper=cyan(5), Ink=blue(1), Bright=0   → 0x29  border=5
 * Depth 2: Paper=blue(1), Ink=green(4), Bright=0  → 0x0C  border=1
 * Depth 3: Paper=black(0), Ink=white(7), Bright=0 → 0x07  border=0
 */

#include <string.h>
#include <stdint.h>
#include "../config/game_config.h"
#include "../include/depth.h"

/* --- Transition duration (~0.15 second at 50fps) --- */
#define TRANSITION_FRAMES 8

/* --- Resting attributes per depth (1-indexed, [0] unused) --- */
static const uint8_t depth_attr[4]   = { 0, 0x29, 0x0C, 0x07 };
static const uint8_t depth_border[4] = { 0, 5,    1,    0    };

uint8_t current_depth = 1;

/* --- Border helper (non-static: shared with sound.c for beeper) --- */
uint8_t border_val;

static void set_border(uint8_t c)
{
    border_val = c;
    __asm
        ld  a, (_border_val)
        out (254), a
    __endasm;
}

/* ------------------------------------------------------------------ */
/* Transition lookup tables                                            */
/* ------------------------------------------------------------------ */

/* Depth 1 → 2: 7-step colour cycle */
static const uint8_t t12_attr[]   = { 0x29, 0x21, 0x19, 0x11, 0x0D, 0x0D, 0x0C };
static const uint8_t t12_border[] = {    5,    4,    3,    2,    1,    1,    1 };
#define T12_STEPS 7

/* Depth 2 → 3: 3-step */
static const uint8_t t23_attr[]   = { 0x0C, 0x0F, 0x07 };
static const uint8_t t23_border[] = {    1,    1,    0 };
#define T23_STEPS 3

/* Depth 3 → 2: 3-step */
static const uint8_t t32_attr[]   = { 0x07, 0x0F, 0x0C };
static const uint8_t t32_border[] = {    0,    1,    1 };
#define T32_STEPS 3

/* Depth 2 → 1: 7-step (reverse of 1→2) */
static const uint8_t t21_attr[]   = { 0x0C, 0x0D, 0x0D, 0x11, 0x19, 0x21, 0x29 };
static const uint8_t t21_border[] = {    1,    1,    1,    2,    3,    4,    5 };
#define T21_STEPS 7

/* --- Active transition state --- */
static const uint8_t *trans_attrs;
static const uint8_t *trans_borders;
static uint8_t trans_steps;
static uint8_t trans_step;          /* current step index */
static uint8_t trans_frames_per;    /* frames between step changes */
static uint8_t trans_frame_ctr;     /* countdown to next step */
static uint8_t trans_active;
static uint8_t trans_target;        /* destination depth layer */

/* ------------------------------------------------------------------ */
/* Re-apply the current border (call after beeper corrupts port 254)   */
/* ------------------------------------------------------------------ */
void depth_restore_border(void)
{
    set_border(border_val);
}

/* ------------------------------------------------------------------ */
/* Immediate depth set                                                 */
/* ------------------------------------------------------------------ */
void depth_set(uint8_t layer)
{
    current_depth = layer;
    memset(ATTR, depth_attr[layer], ATTR_SZ);
    set_border(depth_border[layer]);
}

uint8_t depth_get_attr(uint8_t layer)
{
    return depth_attr[layer];
}

uint8_t depth_get_border(uint8_t layer)
{
    return depth_border[layer];
}

uint8_t depth_get_paper(void)
{
    /* Read a cell that is always set by depth_set / transition tick.
     * Return just the paper+bright bits (mask 0x78) so callers can
     * OR in their own ink colour. */
    return ATTR[0] & 0x78;
}

/* ------------------------------------------------------------------ */
/* Start a non-blocking transition                                     */
/* ------------------------------------------------------------------ */
void depth_start_transition(uint8_t from, uint8_t to)
{
    if (from == 1 && to == 2) {
        trans_attrs = t12_attr;   trans_borders = t12_border;
        trans_steps = T12_STEPS;
    } else if (from == 2 && to == 3) {
        trans_attrs = t23_attr;   trans_borders = t23_border;
        trans_steps = T23_STEPS;
    } else if (from == 3 && to == 2) {
        trans_attrs = t32_attr;   trans_borders = t32_border;
        trans_steps = T32_STEPS;
    } else if (from == 2 && to == 1) {
        trans_attrs = t21_attr;   trans_borders = t21_border;
        trans_steps = T21_STEPS;
    } else {
        /* Invalid transition — just snap */
        depth_set(to);
        return;
    }

    trans_target = to;
    trans_step = 0;
    trans_frames_per = TRANSITION_FRAMES / (trans_steps - 1);
    if (trans_frames_per < 1) trans_frames_per = 1;
    trans_frame_ctr = trans_frames_per;
    trans_active = 1;

    /* Apply the first step immediately */
    memset(ATTR, trans_attrs[0], ATTR_SZ);
    set_border(trans_borders[0]);
}

/* ------------------------------------------------------------------ */
/* Advance transition by one frame                                     */
/* ------------------------------------------------------------------ */
uint8_t depth_transition_tick(void)
{
    if (!trans_active) return 1;

    if (--trans_frame_ctr == 0) {
        trans_step++;
        if (trans_step >= trans_steps) {
            /* Transition complete — snap to destination depth */
            depth_set(trans_target);
            trans_active = 0;
            return 1;
        }
        memset(ATTR, trans_attrs[trans_step], ATTR_SZ);
        set_border(trans_borders[trans_step]);
        trans_frame_ctr = trans_frames_per;
    }

    return 0;
}

uint8_t depth_is_transitioning(void)
{
    return trans_active;
}
