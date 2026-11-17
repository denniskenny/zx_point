/*
 * predators.c -- Predator AI, spawning, rendering, collision
 *
 * Rays (depth 1): diagonal bounce within grid, direct-draw when visible.
 * Sharks (depth 2): diagonal default, pursue player when adjacent.
 * GOOs (depth 3): slow random grid movement, grid-based instant death.
 *
 * Rendering uses XOR sprites via the shared entity_render pool.
 * Up to MAX_VISIBLE_ENTITIES predators drawn simultaneously.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/prng.h"
#include "../include/predators.h"
#include "../include/player.h"
#include "../include/gfx.h"
#include "../include/depth.h"
#include "../include/entity_render.h"
#include "../include/grid.h"
#include "../include/sprites_packed.h"

/* --- PRNG (shared algorithm, separate stream from bubblefield & treasure) --- */
static prng_t p_rng = { 0xBEEF, 0 };

static uint8_t pred_rand(void)
{
    return (uint8_t)prng_next(&p_rng);
}

/* --- Predator data --- */
predator_t predators[MAX_PREDATORS];
uint8_t predator_count;
uint8_t pred_ray_count;
uint8_t pred_shark_count;
uint8_t pred_goo_count;

/* --- Predator sprite renderer (forked from entity_pool) ---------------
 * Predators keep their own on-screen slot list instead of the shared
 * entity_pool used by treasures, so they can be redrawn flicker-free:
 *   - erase + redraw happen back-to-back in predators_render(), not split
 *     across the frame with the bubble field drawn in between;
 *   - old attribute cells are restored BEFORE new sprites set theirs, so an
 *     overlapping old position can't wipe the new sprite's colour;
 *   - sprites whose position, scale, frame and colour are unchanged are
 *     left on screen untouched (no erase/redraw at all). */
typedef struct {
    uint8_t x, y, scale, attr;
    const uint8_t *frame;
} pred_slot_t;

static pred_slot_t pred_on[MAX_VISIBLE_ENTITIES];   /* currently on screen */
static uint8_t     pred_on_n;
static pred_slot_t pred_new[MAX_VISIBLE_ENTITIES];  /* this frame's targets */
static uint8_t     pred_new_n;
static uint8_t     pred_keep[MAX_VISIBLE_ENTITIES]; /* new slot already shown */

/* Same layout rules as entity_render.c: centre smaller scales in 32 px */
static const uint8_t pscale_x_off[] = { 0, 8, 12, 14 };
static const uint8_t pscale_y_off[] = { 0, 16, 24, 28 };
static const uint8_t pscale_attr_wh[] = { 4, 2, 1, 1 };

/* Ink colour per predator type; paper comes from the depth layer */
static const uint8_t pred_ink[3] = {
    1,      /* PRED_RAY   — blue */
    7,      /* PRED_SHARK — white */
    1       /* PRED_GOO   — (never rendered) */
};

/* --- Downscaled predator frame tables --- */
/* Index = face_left * 2 + anim frame. Art faces right; _m = mirrored. */
static const uint8_t * const ray_f_32[]   = { ray_f1,    ray_f2,    ray_f1_m,    ray_f2_m };
static const uint8_t * const ray_f_16[]   = { ray_f1_16, ray_f2_16, ray_f1_16_m, ray_f2_16_m };
static const uint8_t * const ray_f_8[]    = { ray_f1_8,  ray_f2_8,  ray_f1_8_m,  ray_f2_8_m };
static const uint8_t * const shark_f_32[] = { shark_f1,    shark_f2,    shark_f1_m,    shark_f2_m };
static const uint8_t * const shark_f_16[] = { shark_f1_16, shark_f2_16, shark_f1_16_m, shark_f2_16_m };
static const uint8_t * const shark_f_8[]  = { shark_f1_8,  shark_f2_8,  shark_f1_8_m,  shark_f2_8_m };

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
    pred_on_n = 0;
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
    p->face_left = 0;
    p->grid_move_ctr = grid_intervals[type];
}

/* ------------------------------------------------------------------ */
/* Spawn predators for a level                                         */
/* ------------------------------------------------------------------ */
/* Counts come from the LEVEL_* tables in config/game_config.h.       */
void predators_spawn(uint8_t n_rays, uint8_t n_sharks, uint8_t n_goos)
{
    uint8_t i, idx;

    predator_count = 0;

    /* Cap to MAX_PREDATORS, dropping rays first */
    if ((uint16_t)n_sharks + n_goos >= MAX_PREDATORS)
        n_rays = 0;
    else if ((uint16_t)n_rays + n_sharks + n_goos > MAX_PREDATORS)
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
/* One grid step along an axis, bouncing between 1 and hi */
static void grid_bounce(uint8_t *g, int8_t *d, uint8_t hi)
{
    if (*d > 0) {
        if (*g < hi) (*g)++;
        else *d = -1;
    } else {
        if (*g > 1) (*g)--;
        else *d = 1;
    }
}

void predators_update(void)
{
    predator_t *p;
    predator_t *end = predators + predator_count;

    for (p = predators; p < end; p++) {
        if (!p->active) continue;

        /* --- Grid movement (every N frames) --- */
        if (--p->grid_move_ctr == 0) {
            p->grid_move_ctr = grid_intervals[p->type];

            /* Shark AI: pursue player when within range */
            if (p->type == PRED_SHARK && player.gy == 1) {
                if (grid_dist(player.gx, player.gz, p->gx, p->gz)
                        <= PRED_SHARK_PURSUE_RANGE) {
                    grid_steer(&p->gdx, p->gx, player.gx);
                    grid_steer(&p->gdz, p->gz, player.gz);
                }
            }

            /* Move diagonally, bouncing off the grid edges */
            grid_bounce(&p->gx, &p->gdx, GRID_W - 2);
            grid_bounce(&p->gz, &p->gdz, GRID_H - 2);
        }

        /* --- Visibility: within range + same depth, not GOO --- */
        if (p->type != PRED_GOO && p->type == player.gy) {
            p->visible = grid_dist(player.gx, player.gz, p->gx, p->gz)
                         <= PRED_VISIBLE_RANGE;
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
static void pred_xor(const pred_slot_t *sl)
{
    switch (sl->scale) {
    case SCALE_32:
        xor32_spr = sl->frame; xor32_x = sl->x; xor32_y = sl->y;
        xor32_attr = sl->attr;
        xor_sprite_32_fast();
        break;
    case SCALE_16:
        xor16_spr = sl->frame; xor16_x = sl->x; xor16_y = sl->y;
        xor16_attr = sl->attr;
        xor_sprite_16();
        break;
    case SCALE_8:
        xor8_spr = sl->frame; xor8_x = sl->x; xor8_y = sl->y;
        xor8_attr = sl->attr;
        xor_sprite_8();
        break;
    case SCALE_4:
        xor4_x = sl->x; xor4_y = sl->y; xor4_attr = sl->attr;
        xor_block_4();
        break;
    }
}

static void pred_set_attr(const pred_slot_t *sl, uint8_t attr)
{
    uint8_t wh = pscale_attr_wh[sl->scale];
    set_attr_rect(sl->x >> 3, sl->y >> 3, wh, wh, attr);
}

/* ------------------------------------------------------------------ */
/* Select predator frame at the given scale                            */
/* ------------------------------------------------------------------ */
static const uint8_t *pred_pick_frame(uint8_t type, uint8_t scale,
                                      uint8_t frame_ctr, uint8_t face_left)
{
    uint8_t fi = ((frame_ctr & 0x08) ? 1 : 0) | (face_left << 1);
    if (type == PRED_RAY) {
        switch (scale) {
        case SCALE_32: return ray_f_32[fi];
        case SCALE_16: return ray_f_16[fi];
        case SCALE_8:  return ray_f_8[fi];
        default:       return (void *)0;
        }
    } else {
        switch (scale) {
        case SCALE_32: return shark_f_32[fi];
        case SCALE_16: return shark_f_16[fi];
        case SCALE_8:  return shark_f_8[fi];
        default:       return (void *)0;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Erase old + draw new predator sprites in one tight pass.            */
/* show=0 hides them all (e.g. during a depth transition).            */
/* ------------------------------------------------------------------ */
void predators_render(uint8_t frame_ctr, uint8_t show)
{
    uint8_t i, j, scale;
    predator_t *p;
    int16_t dx_sub;
    uint8_t sx, ax, ay, attr, bg_attr;
    pred_slot_t *n;

    /* --- 1. Work out this frame's sprites (no screen writes yet) --- */
    pred_new_n = 0;
    attr = depth_get_paper();
    for (i = 0; show && i < predator_count &&
                pred_new_n < MAX_VISIBLE_ENTITIES; i++) {
        p = &predators[i];
        if (!p->visible) continue;

        scale = entity_z_to_scale(grid_absdiff(player.gz, p->gz));
        if (scale == SCALE_NONE) continue;

        dx_sub = (int16_t)(p->gx - player.gx) * CUBE_SUB_XY
                 + ((CUBE_SUB_XY / 2) - player.sub_x);
        sx = entity_screen_x(dx_sub);

        /* Face the diver: sprite centre is sx + 16 at every scale */
        {
            uint8_t cx = sx + 16;
            if (cx > 128 + PRED_FACE_HYSTERESIS)      p->face_left = 1;
            else if (cx < 128 - PRED_FACE_HYSTERESIS) p->face_left = 0;
        }

        ax = (sx + pscale_x_off[scale]) & 0xF8;
        ay = env_entity_y + pscale_y_off[scale];
        if (ay >= VIEW_H) continue;

        n = &pred_new[pred_new_n++];
        n->x = ax; n->y = ay; n->scale = scale;
        n->attr = attr | pred_ink[p->type];
        n->frame = pred_pick_frame(p->type, scale, frame_ctr, p->face_left);
    }

    /* --- 2. Erase sprites that changed; restore their attr cells --- */
    bg_attr = depth_get_paper() | (ATTR[0] & 0x07);
    for (j = 0; j < pred_new_n; j++) pred_keep[j] = 0;
    for (i = 0; i < pred_on_n; i++) {
        pred_slot_t *o = &pred_on[i];
        for (j = 0; j < pred_new_n; j++) {
            n = &pred_new[j];
            if (!pred_keep[j] && n->x == o->x && n->y == o->y &&
                n->scale == o->scale && n->frame == o->frame &&
                n->attr == o->attr)
                break;
        }
        if (j < pred_new_n) { pred_keep[j] = 1; continue; }
        pred_xor(o);
        pred_set_attr(o, bg_attr);
    }

    /* --- 3. Draw new sprites; re-assert attrs of unchanged ones --- */
    for (j = 0; j < pred_new_n; j++) {
        n = &pred_new[j];
        if (pred_keep[j]) pred_set_attr(n, n->attr);
        else              pred_xor(n);
        pred_on[j] = *n;
    }
    pred_on_n = pred_new_n;
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
        if (grid_dist(player.gx, player.gz, p->gx, p->gz) < CONTACT_DISTANCE) {
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
    uint8_t i;
    uint8_t bg_attr = depth_get_paper() | (ATTR[0] & 0x07);
    for (i = 0; i < pred_on_n; i++) {
        pred_xor(&pred_on[i]);
        pred_set_attr(&pred_on[i], bg_attr);
    }
    pred_on_n = 0;
}
