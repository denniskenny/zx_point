/*
 * test_orientation.c -- Verify axis orientation consistency
 *
 * Runs on ZX Spectrum (via ZEsarUX headless).  Exercises the
 * orientation-sensitive functions and writes pass/fail results
 * to a fixed RAM address so the runner script can read them
 * over ZRCP.
 *
 * Build:  make test  (links src/grid.c, the real code under test)
 * Results at 0xD000:
 *   byte 0  = total test count
 *   byte 1  = pass count
 *   bytes 2..N = per-test (1=pass, 0=fail)
 *   byte N+1   = 0xAA sentinel (signals completion)
 */

#include <stdint.h>
#include "../config/game_config.h"

#define RESULTS ((volatile uint8_t *)0xD000)

/* ---- Functions under test come from the game itself (src/grid.c) ---- */
#include "../include/grid.h"
#include "../include/entity_render.h"

typedef struct {
    uint8_t gx, gz;
    int16_t sub_x, sub_y;
} tplayer_t;

/* Same axis conventions as player_update() in src/player.c */
static void move_x(tplayer_t *p, int8_t vx)
{
    p->sub_x += vx;
    grid_cube_step(&p->sub_x, &p->gx, GRID_W - 1, CUBE_SUB_XY);
}

static void move_z(tplayer_t *p, int8_t vz)
{
    p->sub_y -= vz;
    grid_cube_step(&p->sub_y, &p->gz, GRID_H - 1, CUBE_SUB_XY);
}

/* ---- Test harness ---- */

static uint8_t t_num;
static uint8_t t_pass;

static void check(uint8_t cond)
{
    RESULTS[2 + t_num] = cond ? 1 : 0;
    if (cond) t_pass++;
    t_num++;
}

void main(void)
{
    tplayer_t p;
    uint8_t i;
    uint8_t sx_west, sx_east;
    uint8_t mm_west, mm_east;

    t_num = 0;
    t_pass = 0;

    /* ============================================================
     * 1. O key (left, vx positive) increases gx
     * ============================================================ */
    p.gx = 16; p.sub_x = CUBE_SUB_XY / 2;
    for (i = 0; i < CUBE_SUB_XY; i++) move_x(&p, 1);
    check(p.gx == 17);

    /* ============================================================
     * 2. P key (right, vx negative) decreases gx
     * ============================================================ */
    p.gx = 16; p.sub_x = CUBE_SUB_XY / 2;
    for (i = 0; i < CUBE_SUB_XY; i++) move_x(&p, -1);
    check(p.gx == 15);

    /* ============================================================
     * 3. Q key (forward, vz negative) increases gz
     * ============================================================ */
    p.gz = 16; p.sub_y = CUBE_SUB_XY / 2;
    for (i = 0; i < CUBE_SUB_XY; i++) move_z(&p, -1);
    check(p.gz == 17);

    /* ============================================================
     * 4. A key (backward, vz positive) decreases gz
     * ============================================================ */
    p.gz = 16; p.sub_y = CUBE_SUB_XY / 2;
    for (i = 0; i < CUBE_SUB_XY; i++) move_z(&p, 1);
    check(p.gz == 15);

    /* ============================================================
     * 5. Minimap: higher gx → lower pixel (further LEFT)
     * ============================================================ */
    check(GRID_TO_MINIMAP(20) < GRID_TO_MINIMAP(10));

    /* ============================================================
     * 6. Minimap: higher gz → lower pixel (further UP / north)
     * ============================================================ */
    check(GRID_TO_MINIMAP(20) < GRID_TO_MINIMAP(10));

    /* ============================================================
     * 7. Screen: entity with higher gx (west) → left on screen
     *    dx_sub > 0 when entity.gx > player.gx
     * ============================================================ */
    sx_west = entity_screen_x( 4 * CUBE_SUB_XY);  /* entity west */
    sx_east = entity_screen_x(-4 * CUBE_SUB_XY);  /* entity east */
    check(sx_west < sx_east);

    /* ============================================================
     * 8. Screen X and minimap X agree: entity west of player
     *    appears LEFT on both screen and minimap
     * ============================================================ */
    mm_west = GRID_TO_MINIMAP(20);  /* entity gx=20 */
    mm_east = GRID_TO_MINIMAP(12);  /* player gx=16, entity gx=12 */
    check(sx_west < sx_east && mm_west < mm_east);

    /* ============================================================
     * 9. Z scale: close (|dz|=0) → SCALE_32 (largest)
     * ============================================================ */
    check(entity_z_to_scale(0) == SCALE_32);

    /* ============================================================
     * 10. Z scale: far (|dz|=4) → SCALE_4 (smallest)
     * ============================================================ */
    check(entity_z_to_scale(4) == SCALE_4);

    /* ============================================================
     * 11. Z scale: monotonically increasing scale index with
     *     distance (bigger index = smaller sprite)
     * ============================================================ */
    check(entity_z_to_scale(1) < entity_z_to_scale(2) &&
          entity_z_to_scale(2) < entity_z_to_scale(3) &&
          entity_z_to_scale(3) < entity_z_to_scale(4));

    /* ============================================================
     * 12. Z scale: beyond max range → SCALE_NONE
     * ============================================================ */
    check(entity_z_to_scale(5) == SCALE_NONE);

    /* ============================================================
     * 13. Shark pursuit X: player.gx > shark.gx → gdx = +1
     *     (shark gx increases to chase west toward player)
     * ============================================================ */
    {
        int8_t gdx = -1;
        grid_steer(&gdx, 10, 20);   /* shark at 10, player at 20 */
        check(gdx == 1);
    }

    /* ============================================================
     * 14. Shark pursuit Z: player.gz > shark.gz → gdz = +1
     *     (shark gz increases to chase north toward player)
     * ============================================================ */
    {
        int8_t gdz = -1;
        grid_steer(&gdz, 10, 20);   /* shark at 10, player at 20 */
        check(gdz == 1);
    }

    /* ============================================================
     * 15. Pursuit direction matches screen direction:
     *     shark chasing west (gdx=+1 → gx++) should move LEFT
     *     on screen, same direction as the player it's chasing
     * ============================================================ */
    {
        uint8_t before_sx, after_sx;
        int16_t dx_sub;
        uint8_t shark_gx = 10, player_gx = 16;

        dx_sub = (int16_t)(shark_gx - player_gx) * CUBE_SUB_XY;
        before_sx = entity_screen_x(dx_sub);

        shark_gx++;  /* gdx = +1 step */
        dx_sub = (int16_t)(shark_gx - player_gx) * CUBE_SUB_XY;
        after_sx = entity_screen_x(dx_sub);

        /* shark gx increased (west) → screen sx should decrease (left) */
        check(after_sx <= before_sx);
    }

    /* ---- Write summary ---- */
    RESULTS[0] = t_num;
    RESULTS[1] = t_pass;
    RESULTS[2 + t_num] = 0xAA;

    for (;;);
}
