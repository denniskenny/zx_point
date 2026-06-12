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

/* --- Sprite frame lookup tables (indexed by treasure type) --- */
static const uint8_t * const trs_frame1[TREASURE_TYPE_COUNT] = {
    statue_f1, tablet_f1, altar_f1,
    firstaid_f1, oxygen_tank_f1, map_item_f1, log_item_f1
};
static const uint8_t * const trs_frame2[TREASURE_TYPE_COUNT] = {
    statue_f2, tablet_f2, altar_f2,
    firstaid_f2, oxygen_tank_f2, map_item_f2, log_item_f2
};

/* --- Previous draw position tracking for erase --- */
#define MAX_VISIBLE_TREASURES 2
static uint8_t trs_prev_x[MAX_VISIBLE_TREASURES];
static uint8_t trs_prev_y[MAX_VISIBLE_TREASURES];
static const uint8_t *trs_prev_frame[MAX_VISIBLE_TREASURES];
static uint8_t trs_prev_drawn[MAX_VISIBLE_TREASURES];
static uint8_t trs_prev_count;

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
/* Treasure rendering (direct screen writes, same pattern as predators) */
/* ------------------------------------------------------------------ */
void treasure_init_render(void)
{
    uint8_t i;
    for (i = 0; i < MAX_VISIBLE_TREASURES; i++)
        trs_prev_drawn[i] = 0;
    trs_prev_count = 0;
}

void treasure_render(uint8_t frame_ctr)
{
    uint8_t i, pool_idx;
    const uint8_t *frame_data;
    uint8_t draw_x, attr;
    int16_t dx_sub, dz_sub, sx, sy;

    /* XOR-erase previous frame's treasure sprites */
    for (i = 0; i < trs_prev_count; i++) {
        if (trs_prev_drawn[i]) {
            xor_sprite_32(SCREEN, trs_prev_frame[i],
                          trs_prev_x[i], trs_prev_y[i]);
            set_attr_rect(trs_prev_x[i] >> 3, trs_prev_y[i] >> 3,
                          4, 4, depth_get_paper() | (ATTR[0] & 0x07));
        }
    }

    pool_idx = 0;

    for (i = 0; i < level.treasure_count && pool_idx < MAX_VISIBLE_TREASURES; i++) {
        if (treasures[i].collected) continue;

        /* Visible when within TREASURE_VISIBLE_RANGE and same depth */
        {
            int8_t dgx = (int8_t)(player.gx - treasures[i].gx);
            int8_t dgz = (int8_t)(player.gz - treasures[i].gz);
            if (dgx < 0) dgx = -dgx;
            if (dgz < 0) dgz = -dgz;
            if (dgx > TREASURE_VISIBLE_RANGE ||
                dgz > TREASURE_VISIBLE_RANGE ||
                treasures[i].gy != player.gy)
                continue;
        }

        /* Screen position from grid distance + sub-cube offset.
         * Centre a 32px sprite on the 16px diver when at same cell. */
        dx_sub = (int16_t)(treasures[i].gx - player.gx) * CUBE_SUB_XY
                 + ((CUBE_SUB_XY / 2) - player.sub_x);
        dz_sub = (int16_t)(treasures[i].gz - player.gz) * CUBE_SUB_XY
                 + ((CUBE_SUB_XY / 2) - player.sub_y);

        sx = (DIVER_X - 8) - (dx_sub * 3 / 4);
        sy = (DIVER_Y - 8) - (dz_sub * 3 / 4);

        /* Clamp to play area */
        if (sx < PRED_X_MIN) sx = PRED_X_MIN;
        if (sx > PRED_X_MAX) sx = PRED_X_MAX;
        if (sy < PRED_Y_MIN) sy = PRED_Y_MIN;
        if (sy > PRED_Y_MAX) sy = PRED_Y_MAX;

        /* Select frame: alternate normal/shimmer every 8 frames */
        frame_data = (frame_ctr & 0x08)
            ? trs_frame2[treasures[i].type]
            : trs_frame1[treasures[i].type];

        attr = depth_get_paper() | 0x07;

        /* Byte-align X for drawing */
        draw_x = (uint8_t)sx & 0xF8;

        xor_sprite_32(SCREEN, frame_data, draw_x, (uint8_t)sy);
        set_attr_rect(draw_x >> 3, (uint8_t)sy >> 3, 4, 4, attr);

        trs_prev_x[pool_idx] = draw_x;
        trs_prev_y[pool_idx] = (uint8_t)sy;
        trs_prev_frame[pool_idx] = frame_data;
        trs_prev_drawn[pool_idx] = 1;

        pool_idx++;
    }

    /* Mark remaining pool slots as not drawn */
    for (; pool_idx < MAX_VISIBLE_TREASURES; pool_idx++)
        trs_prev_drawn[pool_idx] = 0;

    trs_prev_count = pool_idx;
}

void treasure_hide_all(void)
{
    uint8_t i;
    for (i = 0; i < trs_prev_count; i++) {
        if (trs_prev_drawn[i]) {
            xor_sprite_32(SCREEN, trs_prev_frame[i],
                          trs_prev_x[i], trs_prev_y[i]);
            set_attr_rect(trs_prev_x[i] >> 3, trs_prev_y[i] >> 3,
                          4, 4, depth_get_paper() | (ATTR[0] & 0x07));
            trs_prev_drawn[i] = 0;
        }
    }
    trs_prev_count = 0;
}

uint8_t treasure_nearest_distance(void)
{
    uint8_t i, best = 255;
    int8_t dx, dz;
    uint8_t d;

    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        dx = (int8_t)(player.gx - treasures[i].gx);
        dz = (int8_t)(player.gz - treasures[i].gz);
        if (dx < 0) dx = -dx;
        if (dz < 0) dz = -dz;
        d = (uint8_t)dx > (uint8_t)dz ? (uint8_t)dx : (uint8_t)dz;
        if (d < best) best = d;
    }
    return best;
}
