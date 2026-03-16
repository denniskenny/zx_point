/*
 * predators.c -- Predator AI, spawning, rendering, collision
 *
 * Rays (depth 1): diagonal bounce within grid, direct-draw when visible.
 * Sharks (depth 2): diagonal default, pursue player when adjacent.
 * GOOs (depth 3): slow random grid movement, grid-based instant death.
 *
 * Rendering uses direct screen writes (write_sprite_32 / erase_sprite_32).
 * Up to MAX_VISIBLE_PREDS predators drawn simultaneously.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/predators.h"
#include "../include/player.h"
#include "../include/gfx.h"
#include "../include/ray.h"
#include "../include/shark.h"

/* --- PRNG (separate seed from starfield & treasure) --- */
static uint16_t p_lfsr = 0xBEEF;
static uint16_t p_weyl = 0;

static uint8_t pred_rand(void)
{
    uint16_t bit = ((p_lfsr >> 0) ^ (p_lfsr >> 2) ^
                    (p_lfsr >> 3) ^ (p_lfsr >> 5)) & 1;
    p_lfsr = (p_lfsr >> 1) | (bit << 15);
    p_weyl += 0x9E35;
    return (uint8_t)((p_lfsr ^ p_weyl) & 0xFF);
}

/* --- Predator data --- */
predator_t predators[MAX_PREDATORS];
uint8_t predator_count;

/* --- Previous draw position tracking for erase --- */
static uint8_t prev_draw_x[MAX_VISIBLE_PREDS];
static uint8_t prev_draw_y[MAX_VISIBLE_PREDS];
static uint8_t prev_drawn[MAX_VISIBLE_PREDS];
static uint8_t prev_pool_count;

/* --- Grid-move intervals indexed by predator type --- */
static const uint16_t grid_intervals[3] = {
    PRED_RAY_GRID_INTERVAL,
    PRED_SHARK_GRID_INTERVAL,
    PRED_GOO_GRID_INTERVAL
};

/* ------------------------------------------------------------------ */
/* Depth implied by predator type (0-indexed)                          */
/* ------------------------------------------------------------------ */
static uint8_t pred_depth(uint8_t type)
{
    if (type == PRED_RAY)   return 0;
    if (type == PRED_SHARK) return 1;
    return 2;   /* PRED_GOO */
}

/* ------------------------------------------------------------------ */
/* Initialise predator draw tracking (call once at game start)         */
/* ------------------------------------------------------------------ */
void predators_init(void)
{
    uint8_t i;
    for (i = 0; i < MAX_VISIBLE_PREDS; i++)
        prev_drawn[i] = 0;
    prev_pool_count = 0;
}

/* ------------------------------------------------------------------ */
/* Spawn a single predator into the array                              */
/* ------------------------------------------------------------------ */
static void spawn_one(uint8_t idx, uint8_t type)
{
    predator_t *p = &predators[idx];

    p->type   = type;
    p->gx     = 2 + (pred_rand() % (GRID_W - 4));
    p->gz     = 2 + (pred_rand() % (GRID_H - 4));
    p->sx     = PRED_X_MIN + (pred_rand() % (PRED_X_MAX - PRED_X_MIN));
    p->sy     = PRED_Y_MIN + (pred_rand() % (PRED_Y_MAX - PRED_Y_MIN));
    p->sdx    = (pred_rand() & 1) ? 1 : -1;
    p->sdy    = (pred_rand() & 1) ? 1 : -1;
    p->gdx    = (pred_rand() & 1) ? 1 : -1;
    p->gdz    = (pred_rand() & 1) ? 1 : -1;
    p->active = 1;
    p->visible = 0;
    p->anim_ctr = pred_rand();
    p->grid_move_ctr = grid_intervals[type];
}

/* ------------------------------------------------------------------ */
/* Spawn predators for a level                                         */
/* ------------------------------------------------------------------ */
void predators_spawn(uint8_t level_num)
{
    uint8_t i, idx;
    uint8_t n_rays, n_sharks, n_goos;

    predator_count = 0;
    if (level_num < 2) return;   /* Level 1: no predators */

    n_rays   = level_num * RAY_CONSTANT;
    n_sharks = (level_num >= 3) ? (level_num - 2) : 0;
    n_goos   = (level_num >= 5) ? (level_num - 4) : 0;

    /* Cap to MAX_PREDATORS */
    if (n_rays + n_sharks + n_goos > MAX_PREDATORS)
        n_rays = MAX_PREDATORS - n_sharks - n_goos;

    idx = 0;
    for (i = 0; i < n_rays && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_RAY);
    for (i = 0; i < n_sharks && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_SHARK);
    for (i = 0; i < n_goos && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_GOO);

    predator_count = idx;
}

/* ------------------------------------------------------------------ */
/* Update all predators (called once per frame)                        */
/* ------------------------------------------------------------------ */
void predators_update(void)
{
    uint8_t i;
    predator_t *p;
    int8_t dx, dz;

    for (i = 0; i < predator_count; i++) {
        p = &predators[i];
        if (!p->active) continue;

        /* --- Grid movement (every N frames) --- */
        if (--p->grid_move_ctr == 0) {
            p->grid_move_ctr = grid_intervals[p->type];

            /* Shark AI: pursue player when within range */
            if (p->type == PRED_SHARK && player.gy == 1) {
                dx = (int8_t)player.gx - (int8_t)p->gx;
                dz = (int8_t)player.gz - (int8_t)p->gz;
                if (dx < 0) dx = -dx;
                if (dz < 0) dz = -dz;
                if (dx <= PRED_SHARK_PURSUE_RANGE &&
                    dz <= PRED_SHARK_PURSUE_RANGE) {
                    if (player.gx > p->gx) p->gdx = 1;
                    else if (player.gx < p->gx) p->gdx = -1;
                    if (player.gz > p->gz) p->gdz = 1;
                    else if (player.gz < p->gz) p->gdz = -1;
                }
            }

            /* Move and bounce on grid X */
            if (p->gdx > 0) {
                if (p->gx < GRID_W - 2) p->gx++;
                else p->gdx = -1;
            } else {
                if (p->gx > 1) p->gx--;
                else p->gdx = 1;
            }

            /* Move and bounce on grid Z */
            if (p->gdz > 0) {
                if (p->gz < GRID_H - 2) p->gz++;
                else p->gdz = -1;
            } else {
                if (p->gz > 1) p->gz--;
                else p->gdz = 1;
            }
        }

        /* --- Visibility: same cube + same depth, not GOO --- */
        p->visible = (p->type != PRED_GOO &&
                      pred_depth(p->type) == player.gy &&
                      p->gx == player.gx &&
                      p->gz == player.gz) ? 1 : 0;

        /* --- Screen-space bounce (only when visible) --- */
        if (p->visible) {
            if (p->sx <= PRED_X_MIN) p->sdx = 1;
            else if (p->sx >= PRED_X_MAX) p->sdx = -1;
            p->sx += p->sdx;

            if (p->sy <= PRED_Y_MIN) p->sdy = 1;
            else if (p->sy >= PRED_Y_MAX) p->sdy = -1;
            p->sy += p->sdy;

            p->anim_ctr++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Render visible predators using direct screen writes                  */
/* ------------------------------------------------------------------ */
void predators_render(void)
{
    uint8_t i, pool_idx;
    predator_t *p;
    const uint8_t *frame_data;
    uint8_t draw_x;
    uint8_t attr;

    /* Erase previous frame's sprites */
    for (i = 0; i < prev_pool_count; i++) {
        if (prev_drawn[i])
            erase_sprite_32(SCREEN, prev_draw_x[i], prev_draw_y[i]);
    }

    pool_idx = 0;

    for (i = 0; i < predator_count && pool_idx < MAX_VISIBLE_PREDS; i++) {
        p = &predators[i];
        if (!p->visible) continue;

        /* Select frame data (toggle every 8 animation ticks) */
        if (p->type == PRED_RAY) {
            frame_data = (p->anim_ctr & 0x08) ? ray_f2 : ray_f1;
            attr = 0x46;  /* bright yellow on black */
        } else {
            frame_data = (p->anim_ctr & 0x08) ? shark_f2 : shark_f1;
            attr = 0x42;  /* bright red on black */
        }

        /* Byte-align X for drawing (8-pixel steps) */
        draw_x = p->sx & 0xF8;

        /* Draw sprite */
        write_sprite_32(SCREEN, frame_data, draw_x, p->sy);

        /* Set attributes (4x4 character cells) */
        set_attr_rect(draw_x >> 3, p->sy >> 3, 4, 4, attr);

        /* Track for erase next frame */
        prev_draw_x[pool_idx] = draw_x;
        prev_draw_y[pool_idx] = p->sy;
        prev_drawn[pool_idx] = 1;

        pool_idx++;
    }

    /* Mark remaining pool slots as not drawn */
    for (; pool_idx < MAX_VISIBLE_PREDS; pool_idx++)
        prev_drawn[pool_idx] = 0;

    prev_pool_count = pool_idx;
}

/* ------------------------------------------------------------------ */
/* Check player collision with predators                               */
/* Returns: 0=none, 1=damage (ray/shark), 255=instant death (GOO)    */
/* ------------------------------------------------------------------ */
uint8_t predators_check_collision(void)
{
    uint8_t i;
    predator_t *p;

    for (i = 0; i < predator_count; i++) {
        p = &predators[i];
        if (!p->active) continue;

        /* GOO: grid-based collision (same cube at depth 3) */
        if (p->type == PRED_GOO) {
            if (player.gy == 2 &&
                player.gx == p->gx && player.gz == p->gz)
                return 255;
            continue;
        }

        /* Ray/Shark: screen-space bounding box */
        if (!p->visible) continue;

        /* Player: (DIVER_X, DIVER_Y) size 16x16 */
        /* Pred:   (sx, sy) size 32x32 */
        if (p->sx + 32 > DIVER_X && p->sx < DIVER_X + 16 &&
            p->sy + 32 > DIVER_Y && p->sy < DIVER_Y + 16)
            return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Hide all currently drawn predator sprites                           */
/* ------------------------------------------------------------------ */
void predators_hide_all(void)
{
    uint8_t i;
    for (i = 0; i < prev_pool_count; i++) {
        if (prev_drawn[i]) {
            erase_sprite_32(SCREEN, prev_draw_x[i], prev_draw_y[i]);
            prev_drawn[i] = 0;
        }
    }
    prev_pool_count = 0;
}
