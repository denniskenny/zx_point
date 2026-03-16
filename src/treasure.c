/*
 * treasure.c -- Treasure spawning and collection
 *
 * PRNG for placement, proximity-based collection.
 * At least one archaeological treasure per level.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/treasure.h"
#include "../include/player.h"

/* --- Simple PRNG (separate from starfield's) --- */
static uint16_t t_lfsr = 0x1337;
static uint16_t t_weyl = 0;

static uint8_t treasure_rand(void)
{
    uint16_t bit = ((t_lfsr >> 0) ^ (t_lfsr >> 2) ^
                    (t_lfsr >> 3) ^ (t_lfsr >> 5)) & 1;
    t_lfsr = (t_lfsr >> 1) | (bit << 15);
    t_weyl += 0x9E35;
    return (uint8_t)((t_lfsr ^ t_weyl) & 0xFF);
}

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
    level.time_remaining = TIME_LIMIT_FRAMES;

    for (i = 0; i < count; i++) {
        if (i == 0) {
            /* First treasure is always archaeological (depth 3) */
            treasures[i].type = TREASURE_ARCH_FIRST +
                (treasure_rand() % (TREASURE_ARCH_LAST - TREASURE_ARCH_FIRST + 1));
            treasures[i].gy = GRID_D - 1;
        } else {
            /* Random: archaeological or flotsam */
            if (treasure_rand() & 1) {
                treasures[i].type = TREASURE_ARCH_FIRST +
                    (treasure_rand() % (TREASURE_ARCH_LAST - TREASURE_ARCH_FIRST + 1));
                treasures[i].gy = GRID_D - 1;
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

        /* X/Y proximity check (depth not checked per design) */
        dx = (int8_t)(player.gx - treasures[i].gx);
        dz = (int8_t)(player.gz - treasures[i].gz);
        if (dx < 0) dx = -dx;
        if (dz < 0) dz = -dz;

        if (dx < CONTACT_DISTANCE && dz < CONTACT_DISTANCE) {
            treasures[i].collected = 1;
            level.collected_count++;

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
