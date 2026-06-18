#ifndef _ENTITY_RENDER_H_
#define _ENTITY_RENDER_H_

#include <stdint.h>

/* Sprite scale levels (Z distance → visual size) */
#define SCALE_32    0
#define SCALE_16    1
#define SCALE_8     2
#define SCALE_4     3
#define SCALE_NONE  0xFF

#define MAX_ENTITY_SLOTS 2

typedef struct {
    uint8_t x, y, scale, attr, drawn;
    const uint8_t *frame;
} entity_slot_t;

typedef struct {
    entity_slot_t slots[MAX_ENTITY_SLOTS];
    uint8_t count;
    uint8_t prev_count;
    uint8_t prev_x[MAX_ENTITY_SLOTS];
    uint8_t prev_y[MAX_ENTITY_SLOTS];
    uint8_t prev_scale[MAX_ENTITY_SLOTS];
} entity_pool_t;

/* Environment Y anchor for entities at the current depth level.
 * Set each frame in state.c before entity rendering. */
extern uint8_t env_entity_y;

void entity_pool_init(entity_pool_t *pool);
void entity_pool_erase(entity_pool_t *pool);
uint8_t entity_pool_draw(entity_pool_t *pool, uint8_t sx, uint8_t sy,
                         uint8_t scale, const uint8_t *frame, uint8_t attr);

void entity_pool_cleanup_attrs(entity_pool_t *pool);

uint8_t entity_z_to_scale(uint8_t z_dist);
uint8_t entity_screen_x(int16_t dx_sub);

#endif /* _ENTITY_RENDER_H_ */
