/*
 * predators.c -- Predator AI, spawning, rendering, collision
 *
 * Rays (depth 1): diagonal bounce within grid, direct-draw when visible.
 * Sharks (depth 2): diagonal default, pursue player when adjacent.
 * GOOs (depth 3): slow random grid movement, grid-based instant death.
 *
 * Rendering uses XOR sprites via the shared entity_render pool.
 * Up to MAX_VISIBLE_PREDS predators drawn simultaneously.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/predators.h"
#include "../include/player.h"
#include "../include/gfx.h"
#include "../include/depth.h"
#include "../include/entity_render.h"
#include "../include/ray.h"
#include "../include/shark.h"

/* --- PRNG (separate seed from bubblefield & treasure) --- */
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
uint8_t pred_ray_count;
uint8_t pred_shark_count;
uint8_t pred_goo_count;

/* --- Entity pool for draw/erase tracking --- */
static entity_pool_t pred_pool;

/* --- Downscaled predator frame tables --- */
static const uint8_t * const ray_f_16[] = { ray_f1_16, ray_f2_16 };
static const uint8_t * const ray_f_8[]  = { ray_f1_8,  ray_f2_8 };
static const uint8_t * const shark_f_16[] = { shark_f1_16, shark_f2_16 };
static const uint8_t * const shark_f_8[]  = { shark_f1_8,  shark_f2_8 };

/* --- Grid-move intervals indexed by predator type --- */
static const uint16_t grid_intervals[3] = {
    PRED_RAY_GRID_INTERVAL,
    PRED_SHARK_GRID_INTERVAL,
    PRED_GOO_GRID_INTERVAL
};

/* Depth implied by predator type: PRED_RAY=0, PRED_SHARK=1, PRED_GOO=2 */
#define pred_depth(t) (t)

/* ------------------------------------------------------------------ */
/* Initialise predator draw tracking (call once at game start)         */
/* ------------------------------------------------------------------ */
void predators_init(void)
{
    entity_pool_init(&pred_pool);
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
    p->gdx    = (pred_rand() & 1) ? 1 : -1;
    p->gdz    = (pred_rand() & 1) ? 1 : -1;
    p->active = 1;
    p->visible = 0;
    p->anim_ctr = pred_rand();
    p->proximity_ctr = 0;
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
    /* TEMP: disabled for GOO testing */
    /* if (level_num < 2) return; */

    n_rays   = level_num * RAY_CONSTANT;
    n_sharks = (level_num >= 3) ? (level_num - 2) : 0;
    n_goos   = (level_num >= 5) ? (level_num - 4) : 0;
    /* TEMP: spawn one GOO on level 1 for testing */
    if (n_goos == 0) n_goos = 1;

    /* Cap to MAX_PREDATORS */
    if (n_rays + n_sharks + n_goos > MAX_PREDATORS)
        n_rays = MAX_PREDATORS - n_sharks - n_goos;

    idx = 0;
    for (i = 0; i < n_rays && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_RAY);
    pred_ray_count = i;
    for (i = 0; i < n_sharks && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_SHARK);
    pred_shark_count = i;
    for (i = 0; i < n_goos && idx < MAX_PREDATORS; i++, idx++)
        spawn_one(idx, PRED_GOO);
    pred_goo_count = i;

    predator_count = idx;
}

/* ------------------------------------------------------------------ */
/* Update all predators (called once per frame)                        */
/* ------------------------------------------------------------------ */
void predators_update(void)
{
    predator_t *p;
    predator_t *end = predators + predator_count;
    int8_t dx, dz;

    for (p = predators; p < end; p++) {
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

        /* --- Visibility: within range + same depth, not GOO --- */
        if (p->type != PRED_GOO && p->type == player.gy) {
            dx = (int8_t)(player.gx - p->gx);
            dz = (int8_t)(player.gz - p->gz);
            if (dx < 0) dx = -dx;
            if (dz < 0) dz = -dz;
            p->visible = (dx <= PRED_VISIBLE_RANGE &&
                          dz <= PRED_VISIBLE_RANGE) ? 1 : 0;
        } else {
            p->visible = 0;
        }

        if (p->visible)
            p->anim_ctr++;
    }
}

/* ------------------------------------------------------------------ */
/* XOR-erase predators drawn last frame (call BEFORE background update)*/
/* ------------------------------------------------------------------ */
void predators_erase(void)
{
    entity_pool_erase(&pred_pool);
}

/* ------------------------------------------------------------------ */
/* Select predator frame at the given scale                            */
/* ------------------------------------------------------------------ */
static const uint8_t *pred_pick_frame(uint8_t type, uint8_t scale,
                                      uint8_t frame_ctr)
{
    uint8_t fi = (frame_ctr & 0x08) ? 1 : 0;
    if (type == PRED_RAY) {
        switch (scale) {
        case SCALE_32: return fi ? ray_f2 : ray_f1;
        case SCALE_16: return ray_f_16[fi];
        case SCALE_8:  return ray_f_8[fi];
        default:       return (void *)0;
        }
    } else {
        switch (scale) {
        case SCALE_32: return fi ? shark_f2 : shark_f1;
        case SCALE_16: return shark_f_16[fi];
        case SCALE_8:  return shark_f_8[fi];
        default:       return (void *)0;
        }
    }
}

/* ------------------------------------------------------------------ */
/* XOR-draw visible predators (call AFTER background update)           */
/* ------------------------------------------------------------------ */
void predators_draw(uint8_t frame_ctr)
{
    uint8_t i, scale;
    predator_t *p;
    int8_t dgz;
    int16_t dx_sub;
    uint8_t sx, attr;

    attr = depth_get_paper() | 0x04;

    for (i = 0; i < predator_count; i++) {
        p = &predators[i];
        if (!p->visible) continue;

        dgz = (int8_t)(player.gz - p->gz);
        if (dgz < 0) dgz = -dgz;
        scale = entity_z_to_scale((uint8_t)dgz);
        if (scale == SCALE_NONE) continue;

        dx_sub = (int16_t)(p->gx - player.gx) * CUBE_SUB_XY
                 + ((CUBE_SUB_XY / 2) - player.sub_x);
        sx = entity_screen_x(dx_sub);

        if (!entity_pool_draw(&pred_pool, sx, env_entity_y, scale,
                              pred_pick_frame(p->type, scale, frame_ctr),
                              attr))
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Reset old-position attrs after drawing at new position              */
/* ------------------------------------------------------------------ */
void predators_cleanup_attrs(void)
{
    entity_pool_cleanup_attrs(&pred_pool);
}

/* ------------------------------------------------------------------ */
/* Check player proximity with predators.                              */
/* Proximity-based damage: 1 HP lost per second of continuous contact. */
/* Returns: 0=none, 1=damage, 255=instant death (GOO)                */
/* ------------------------------------------------------------------ */
uint8_t predators_check_collision(void)
{
    predator_t *p;
    predator_t *end = predators + predator_count;
    int8_t dx, dz;
    uint8_t result = 0;

    for (p = predators; p < end; p++) {
        if (!p->active) continue;

        /* GOO: grid-based collision (same cube at depth 3) */
        if (p->type == PRED_GOO) {
            if (player.gy == 2 && player.gx == p->gx && player.gz == p->gz)
                return 255;
            continue;
        }

        /* Must be at same depth */
        if (p->type != player.gy) {
            p->proximity_ctr = 0;
            continue;
        }

        /* Grid proximity check */
        dx = (int8_t)(player.gx - p->gx);
        dz = (int8_t)(player.gz - p->gz);
        if (dx < 0) dx = -dx;
        if (dz < 0) dz = -dz;

        if (dx < CONTACT_DISTANCE && dz < CONTACT_DISTANCE) {
            p->proximity_ctr++;
            if (p->proximity_ctr >= PRED_PROXIMITY_FRAMES) {
                p->proximity_ctr = 0;
                result = 1;
            }
        } else {
            p->proximity_ctr = 0;
        }
    }

    return result;
}

/* ------------------------------------------------------------------ */
/* Hide all currently drawn predator sprites                           */
/* ------------------------------------------------------------------ */
void predators_hide_all(void)
{
    entity_pool_erase(&pred_pool);
    entity_pool_cleanup_attrs(&pred_pool);
}
