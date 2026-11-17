/*
 * player.c -- Player movement & cube traversal
 *
 * Sub-cube position accumulation with grid boundary detection.
 * Depth changes trigger the depth transition system.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/player.h"
#include "../include/grid.h"
#include "../include/depth.h"

player_t player;

void player_init(void)
{
    player.gx = START_GX;           /* centre of minimap cell (1,1) */
    player.gz = START_GZ;
    player.gy = 0;                  /* depth 1 (shallowest) */
    player.sub_x = CUBE_SUB_XY / 2;
    player.sub_y = CUBE_SUB_XY / 2;
    player.sub_z = CUBE_SUB_Z - 1; /* bottom of sub-cube (buoyancy floats up) */
    player.health = HEALTH_MAX;
    player.oxygen = OXYGEN_MAX;
    player.invuln_timer = 0;
}

uint8_t player_update(int8_t vx, int8_t vy, int8_t vz)
{
    uint8_t old_gy = player.gy;

    /* --- Horizontal movement (X axis: O=right, P=left) --- */
    player.sub_x += vx;
    player.at_bound_x = grid_cube_step(&player.sub_x, &player.gx, GRID_W - 1, CUBE_SUB_XY);

    /* --- Forward/backward movement (Q/A via vz → grid Z axis) --- */
    player.sub_y -= vz;
    player.at_bound_z = grid_cube_step(&player.sub_y, &player.gz, GRID_H - 1, CUBE_SUB_XY);

    /* --- Depth movement (Z=descend, buoyancy=ascend via vy) --- */
    /* Z key -> vy negative -> sub_z increases (diving) */
    player.sub_z -= vy;
    player.at_bound_y = grid_cube_step(&player.sub_z, &player.gy, GRID_D - 1, CUBE_SUB_Z);
    if (player.gy != old_gy)
        depth_start_transition(old_gy + 1, player.gy + 1);

    /* Decrement invulnerability timer */
    if (player.invuln_timer > 0) player.invuln_timer--;

    return (player.gy != old_gy) ? 1 : 0;
}
