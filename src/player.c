/*
 * player.c -- Player movement & cube traversal
 *
 * Sub-cube position accumulation with grid boundary detection.
 * Depth changes trigger the depth transition system.
 */

#include <stdint.h>
#include "../config/game_config.h"
#include "../include/player.h"
#include "../include/depth.h"

player_t player;

void player_init(void)
{
    player.gx = GRID_W / 2;        /* centre of grid */
    player.gz = GRID_H / 2;
    player.gy = 0;                  /* depth 1 (shallowest) */
    player.sub_x = CUBE_SUB_XY / 2;
    player.sub_y = CUBE_SUB_XY / 2;
    player.sub_z = CUBE_SUB_Z / 2;
    player.health = HEALTH_MAX;
    player.oxygen = OXYGEN_MAX;
    player.invuln_timer = 0;
}

uint8_t player_update(int8_t vx, int8_t vy, int8_t vz)
{
    uint8_t old_gy = player.gy;

    /* --- Horizontal movement (X axis: O=right, P=left) --- */
    player.sub_x += vx;
    if (player.sub_x >= CUBE_SUB_XY) {
        player.sub_x -= CUBE_SUB_XY;
        if (player.gx < GRID_W - 1) player.gx++;
        else player.sub_x = CUBE_SUB_XY - 1;
    } else if (player.sub_x < 0) {
        player.sub_x += CUBE_SUB_XY;
        if (player.gx > 0) player.gx--;
        else player.sub_x = 0;
    }

    /* --- Vertical movement (Y axis: Q=up, A=down) --- */
    player.sub_y += vy;
    if (player.sub_y >= CUBE_SUB_XY) {
        player.sub_y -= CUBE_SUB_XY;
        if (player.gz < GRID_H - 1) player.gz++;
        else player.sub_y = CUBE_SUB_XY - 1;
    } else if (player.sub_y < 0) {
        player.sub_y += CUBE_SUB_XY;
        if (player.gz > 0) player.gz--;
        else player.sub_y = 0;
    }

    /* --- Depth movement (Z axis: W=dive deeper, S=ascend) --- */
    /* W key -> vz negative -> sub_z increases (diving) */
    player.sub_z -= vz;
    if (player.sub_z >= CUBE_SUB_Z) {
        player.sub_z -= CUBE_SUB_Z;
        if (player.gy < GRID_D - 1) {
            player.gy++;
            depth_start_transition(old_gy + 1, player.gy + 1);
        } else {
            player.sub_z = CUBE_SUB_Z - 1;
        }
    } else if (player.sub_z < 0) {
        player.sub_z += CUBE_SUB_Z;
        if (player.gy > 0) {
            player.gy--;
            depth_start_transition(old_gy + 1, player.gy + 1);
        } else {
            player.sub_z = 0;
        }
    }

    /* Decrement invulnerability timer */
    if (player.invuln_timer > 0) player.invuln_timer--;

    return (player.gy != old_gy) ? 1 : 0;
}
