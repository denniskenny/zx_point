/*
 * treasure.c -- Treasure spawning, collection, and rendering
 *
 * PRNG for placement, proximity-based collection.
 * At least one archaeological treasure per level.
 * 32x32 sprites drawn when player shares a cube with a treasure.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/treasure.h"
#include "../include/player.h"
#include "../include/gfx.h"
#include "../include/depth.h"
#include "../include/sound.h"
#include "../include/entity_render.h"
#include "../include/sealine.h"

/* --- Treasure sprite assets (32x32, 2 frames each) --- */
#include "../include/statue.h"
#include "../include/tablet.h"
#include "../include/altar.h"
#include "../include/firstaid.h"
#include "../include/oxygen_tank.h"
#include "../include/map_item.h"
#include "../include/log_item.h"

/* --- Simple PRNG (separate from bubblefield's) --- */
static uint16_t t_lfsr = 0x1337;
static uint16_t t_weyl = 0;

void treasure_seed(uint16_t seed)
{
    t_lfsr = seed | 1;  /* must be non-zero */
    t_weyl = seed >> 3;
}

static uint8_t treasure_rand(void)
{
    uint16_t bit = ((t_lfsr >> 0) ^ (t_lfsr >> 2) ^
                    (t_lfsr >> 3) ^ (t_lfsr >> 5)) & 1;
    t_lfsr = (t_lfsr >> 1) | (bit << 15);
    t_weyl += 0x9E35;
    return (uint8_t)((t_lfsr ^ t_weyl) & 0xFF);
}

/* --- Sprite frame lookup tables (indexed by treasure type, per scale) --- */
static const uint8_t * const trs_f1_32[TREASURE_TYPE_COUNT] = {
    statue_f1, tablet_f1, altar_f1,
    firstaid_f1, oxygen_tank_f1, map_item_f1, log_item_f1
};
static const uint8_t * const trs_f2_32[TREASURE_TYPE_COUNT] = {
    statue_f2, tablet_f2, altar_f2,
    firstaid_f2, oxygen_tank_f2, map_item_f2, log_item_f2
};
static const uint8_t * const trs_f1_16[TREASURE_TYPE_COUNT] = {
    statue_f1_16, tablet_f1_16, altar_f1_16,
    firstaid_f1_16, oxygen_tank_f1_16, map_item_f1_16, log_item_f1_16
};
static const uint8_t * const trs_f2_16[TREASURE_TYPE_COUNT] = {
    statue_f2_16, tablet_f2_16, altar_f2_16,
    firstaid_f2_16, oxygen_tank_f2_16, map_item_f2_16, log_item_f2_16
};
static const uint8_t * const trs_f1_8[TREASURE_TYPE_COUNT] = {
    statue_f1_8, tablet_f1_8, altar_f1_8,
    firstaid_f1_8, oxygen_tank_f1_8, map_item_f1_8, log_item_f1_8
};
static const uint8_t * const trs_f2_8[TREASURE_TYPE_COUNT] = {
    statue_f2_8, tablet_f2_8, altar_f2_8,
    firstaid_f2_8, oxygen_tank_f2_8, map_item_f2_8, log_item_f2_8
};

/* --- Entity pool for draw/erase tracking --- */
static entity_pool_t trs_pool;

/* --- Global data --- */
treasure_t treasures[MAX_TREASURES];
level_t level;

void treasure_spawn(uint8_t level_num)
{
    uint8_t i;
    uint8_t count = level_num;

    if (count > MAX_TREASURES) count = MAX_TREASURES;

    level.number = level_num;
    level.treasure_count = count;
    level.collected_count = 0;
    level.arch_count = 0;
    level.arch_collected = 0;
    level.time_remaining = TIME_LIMIT_FRAMES;

    for (i = 0; i < count; i++) {
        if (i == 0) {
            /* First treasure is always archaeological (depth 3) */
            treasures[i].type = TREASURE_ARCH_FIRST +
                (treasure_rand() % (TREASURE_ARCH_LAST - TREASURE_ARCH_FIRST + 1));
            treasures[i].gy = GRID_D - 1;
            level.arch_count++;
        } else {
            /* Random: archaeological or flotsam */
            if (treasure_rand() & 1) {
                treasures[i].type = TREASURE_ARCH_FIRST +
                    (treasure_rand() % (TREASURE_ARCH_LAST - TREASURE_ARCH_FIRST + 1));
                treasures[i].gy = GRID_D - 1;
                level.arch_count++;
            } else {
                treasures[i].type = TREASURE_FLOT_FIRST +
                    (treasure_rand() % (TREASURE_FLOT_LAST - TREASURE_FLOT_FIRST + 1));
                treasures[i].gy = 0;
            }
        }

        /* Random horizontal position, avoiding grid edges */
        treasures[i].gx = 2 + (treasure_rand() % (GRID_W - 4));
        treasures[i].gz = 2 + (treasure_rand() % (GRID_H - 4));
        treasures[i].collected = 0;
    }
}

void treasure_check_collection(void)
{
    uint8_t i;
    int8_t dx, dz;

    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        if (treasures[i].gy != player.gy) continue;

        dx = (int8_t)(player.gx - treasures[i].gx);
        dz = (int8_t)(player.gz - treasures[i].gz);
        if (dx < 0) dx = -dx;
        if (dz < 0) dz = -dz;

        if (dx < CONTACT_DISTANCE && dz < CONTACT_DISTANCE) {
            treasures[i].collected = 1;
            level.collected_count++;
            level.total_collected++;

            /* Track archaeological treasure collection */
            if (treasures[i].type <= TREASURE_ARCH_LAST)
                level.arch_collected++;

            /* Collection jingle — fast Taps-inspired arpeggio */
            sfx_collect_jingle();

            /* Apply collection effects */
            switch (treasures[i].type) {
                case TREASURE_FIRSTAID:
                    player.health = HEALTH_MAX;
                    break;
                case TREASURE_OXYGEN:
                    player.oxygen = OXYGEN_MAX;
                    break;
                default:
                    break;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Treasure rendering via shared entity renderer                       */
/* ------------------------------------------------------------------ */
void treasure_init_render(void)
{
    entity_pool_init(&trs_pool);
}

static const uint8_t *trs_pick_frame(uint8_t type, uint8_t scale,
                                     uint8_t frame_ctr)
{
    uint8_t f2 = (frame_ctr & 0x08) ? 1 : 0;
    switch (scale) {
    case SCALE_32: return f2 ? trs_f2_32[type] : trs_f1_32[type];
    case SCALE_16: return f2 ? trs_f2_16[type] : trs_f1_16[type];
    case SCALE_8:  return f2 ? trs_f2_8[type]  : trs_f1_8[type];
    default:       return (void *)0;
    }
}

/* Subtract from seafloor_get_y() to produce the sy param for
 * entity_pool_draw at each scale.  Accounts for entity_pool_draw's
 * internal scale_y_offset so that:
 *   SCALE_4  (far)  → sprite bottom sits on the floor line
 *   SCALE_32 (close) → sprite centred on the floor line */
static const uint8_t trs_floor_offset[] = { 16, 28, 30, 32 };

void treasure_erase(void)
{
    entity_pool_erase(&trs_pool);
}

void treasure_draw(uint8_t frame_ctr)
{
    uint8_t i, scale, sx, attr, sy, use_floor, floor_y;
    int8_t dgx, dgz;
    int16_t dx_sub, sy_i;

    attr = depth_get_paper() | 0x07;
    use_floor = (player.gy == GRID_D - 1 && seafloor_is_visible());
    floor_y = use_floor ? seafloor_get_y() : 0;

    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        if (treasures[i].gy != player.gy) continue;

        dgx = (int8_t)(player.gx - treasures[i].gx);
        dgz = (int8_t)(player.gz - treasures[i].gz);
        if (dgx < 0) dgx = -dgx;
        if (dgz < 0) dgz = -dgz;

        if (dgx > TREASURE_VISIBLE_RANGE) continue;

        scale = entity_z_to_scale((uint8_t)dgz);
        if (scale == SCALE_NONE) continue;

        dx_sub = (int16_t)(treasures[i].gx - player.gx) * CUBE_SUB_XY
                 + ((CUBE_SUB_XY / 2) - player.sub_x);
        sx = entity_screen_x(dx_sub);

        if (use_floor) {
            sy_i = (int16_t)floor_y - trs_floor_offset[scale];
            if (sy_i < PRED_Y_MIN) sy_i = PRED_Y_MIN;
            if (sy_i > PRED_Y_MAX) sy_i = PRED_Y_MAX;
            sy = (uint8_t)sy_i;
        } else {
            sy = env_entity_y;
        }

        if (!entity_pool_draw(&trs_pool, sx, sy, scale,
                              trs_pick_frame(treasures[i].type, scale, frame_ctr),
                              attr))
            break;
    }

    entity_pool_cleanup_attrs(&trs_pool);
}

void treasure_hide_all(void)
{
    entity_pool_erase(&trs_pool);
    entity_pool_cleanup_attrs(&trs_pool);
}
