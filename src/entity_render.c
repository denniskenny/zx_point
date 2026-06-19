#include <stdint.h>
#include "../config/game_config.h"
#include "../include/entity_render.h"
#include "../include/gfx.h"
#include "../include/depth.h"

uint8_t env_entity_y;

static const uint8_t scale_y_offset[] = { 0, 16, 24, 28 };
static const uint8_t scale_x_offset[] = { 0, 8, 12, 14 };

static const uint8_t scale_attr_w[] = { 4, 2, 1, 1 };
static const uint8_t scale_attr_h[] = { 4, 2, 1, 1 };

void entity_pool_init(entity_pool_t *pool)
{
    uint8_t i;
    for (i = 0; i < MAX_ENTITY_SLOTS; i++)
        pool->slots[i].drawn = 0;
    pool->count = 0;
    pool->prev_count = 0;
}

static void xor_slot(entity_slot_t *s)
{
    switch (s->scale) {
    case SCALE_32:
        xor32_spr = s->frame;
        xor32_x = s->x;
        xor32_y = s->y;
        xor32_attr = s->attr;
        xor_sprite_32_fast();
        break;
    case SCALE_16:
        xor16_spr = s->frame;
        xor16_x = s->x;
        xor16_y = s->y;
        xor16_attr = s->attr;
        xor_sprite_16();
        break;
    case SCALE_8:
        xor8_spr = s->frame;
        xor8_x = s->x;
        xor8_y = s->y;
        xor8_attr = s->attr;
        xor_sprite_8();
        break;
    case SCALE_4:
        xor4_x = s->x;
        xor4_y = s->y;
        xor4_attr = s->attr;
        xor_block_4();
        break;
    }
}

void entity_pool_erase(entity_pool_t *pool)
{
    uint8_t i;

    pool->prev_count = pool->count;
    for (i = 0; i < pool->count; i++) {
        pool->prev_x[i] = pool->slots[i].x;
        pool->prev_y[i] = pool->slots[i].y;
        pool->prev_scale[i] = pool->slots[i].scale;
    }

    for (i = 0; i < pool->count; i++) {
        if (pool->slots[i].drawn) {
            xor_slot(&pool->slots[i]);
            pool->slots[i].drawn = 0;
        }
    }
    pool->count = 0;
}

void entity_pool_cleanup_attrs(entity_pool_t *pool)
{
    uint8_t i;
    uint8_t bg_attr = depth_get_paper() | (ATTR[0] & 0x07);

    for (i = 0; i < pool->prev_count; i++) {
        set_attr_rect(pool->prev_x[i] >> 3, pool->prev_y[i] >> 3,
                      scale_attr_w[pool->prev_scale[i]],
                      scale_attr_h[pool->prev_scale[i]], bg_attr);
    }
    pool->prev_count = 0;
}

uint8_t entity_pool_draw(entity_pool_t *pool, uint8_t sx, uint8_t sy,
                         uint8_t scale, const uint8_t *frame, uint8_t attr)
{
    entity_slot_t *s;
    uint8_t adj_x, adj_y;

    if (pool->count >= MAX_ENTITY_SLOTS)
        return 0;

    adj_x = (sx + scale_x_offset[scale]) & 0xF8;
    adj_y = sy + scale_y_offset[scale];

    if (adj_y >= VIEW_H)
        return 0;

    s = &pool->slots[pool->count];
    s->x = adj_x;
    s->y = adj_y;
    s->scale = scale;
    s->attr = attr;
    s->frame = frame;
    s->drawn = 1;

    xor_slot(s);

    pool->count++;
    return 1;
}

uint8_t entity_z_to_scale(uint8_t z_dist)
{
    if (z_dist <= 1) return SCALE_4;
    if (z_dist == 2) return SCALE_8;
    if (z_dist == 3) return SCALE_16;
    if (z_dist == 4) return SCALE_32;
    return SCALE_NONE;
}

uint8_t entity_screen_x(int16_t dx_sub)
{
    int16_t sx = (DIVER_X - 8) - (dx_sub * 3 / 4);
    if (sx < PRED_X_MIN) sx = PRED_X_MIN;
    if (sx > PRED_X_MAX) sx = PRED_X_MAX;
    return (uint8_t)sx & 0xF8;
}
