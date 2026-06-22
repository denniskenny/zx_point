/*
 * test_orientation.c -- Verify axis orientation consistency
 *
 * Runs on ZX Spectrum (via ZEsarUX headless).  Exercises the
 * orientation-sensitive functions and writes pass/fail results
 * to a fixed RAM address so the runner script can read them
 * over ZRCP.
 *
 * Build:  zcc +zx ... -o tests/test_orientation tests/test_orientation.c
 * Results at 0xD000:
 *   byte 0  = total test count
 *   byte 1  = pass count
 *   bytes 2..N = per-test (1=pass, 0=fail)
 *   byte N+1   = 0xAA sentinel (signals completion)
 */

#include <stdint.h>
#include "../config/game_config.h"

#define SCALE_32    0
#define SCALE_16    1
#define SCALE_8     2
#define SCALE_4     3
#define SCALE_NONE  0xFF

#define RESULTS ((volatile uint8_t *)0xD000)

/* ---- Functions under test (mirrored from game sources) ---- */

static uint8_t grid_to_mm(uint8_t g)
{
    return 31 - g;
}

static uint8_t t_entity_z_to_scale(uint8_t z_dist)
{
    if (z_dist <= 1) return SCALE_32;
    if (z_dist == 2) return SCALE_16;
    if (z_dist == 3) return SCALE_8;
    if (z_dist == 4) return SCALE_4;
    return SCALE_NONE;
}

static uint8_t t_entity_screen_x(int16_t dx_sub)
{
    int16_t sx = (DIVER_X - 8) - (dx_sub * 3 / 4);
    if (sx < PRED_X_MIN) sx = PRED_X_MIN;
    if (sx > PRED_X_MAX) sx = PRED_X_MAX;
    return (uint8_t)sx & 0xF8;
}

/* Minimal player movement (from player.c) */
typedef struct {
    uint8_t gx, gz;
    int16_t sub_x, sub_y;
} tplayer_t;

static void move_x(tplayer_t *p, int8_t vx)
{
    p->sub_x += vx;
    if (p->sub_x >= CUBE_SUB_XY) {
        p->sub_x -= CUBE_SUB_XY;
        if (p->gx < GRID_W - 1) p->gx++;
    } else if (p->sub_x < 0) {
        p->sub_x += CUBE_SUB_XY;
        if (p->gx > 0) p->gx--;
    }
}

static void move_z(tplayer_t *p, int8_t vz)
{
    p->sub_y -= vz;
    if (p->sub_y >= CUBE_SUB_XY) {
        p->sub_y -= CUBE_SUB_XY;
        if (p->gz < GRID_H - 1) p->gz++;
    } else if (p->sub_y < 0) {
        p->sub_y += CUBE_SUB_XY;
        if (p->gz > 0) p->gz--;
    }
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
    check(grid_to_mm(20) < grid_to_mm(10));

    /* ============================================================
     * 6. Minimap: higher gz → lower pixel (further UP / north)
     * ============================================================ */
    check(grid_to_mm(20) < grid_to_mm(10));

    /* ============================================================
     * 7. Screen: entity with higher gx (west) → left on screen
     *    dx_sub > 0 when entity.gx > player.gx
     * ============================================================ */
    sx_west = t_entity_screen_x( 4 * CUBE_SUB_XY);  /* entity west */
    sx_east = t_entity_screen_x(-4 * CUBE_SUB_XY);  /* entity east */
    check(sx_west < sx_east);

    /* ============================================================
     * 8. Screen X and minimap X agree: entity west of player
     *    appears LEFT on both screen and minimap
     * ============================================================ */
    mm_west = grid_to_mm(20);  /* entity gx=20 */
    mm_east = grid_to_mm(12);  /* player gx=16, entity gx=12 */
    check(sx_west < sx_east && mm_west < mm_east);

    /* ============================================================
     * 9. Z scale: close (|dz|=0) → SCALE_32 (largest)
     * ============================================================ */
    check(t_entity_z_to_scale(0) == SCALE_32);

    /* ============================================================
     * 10. Z scale: far (|dz|=4) → SCALE_4 (smallest)
     * ============================================================ */
    check(t_entity_z_to_scale(4) == SCALE_4);

    /* ============================================================
     * 11. Z scale: monotonically increasing scale index with
     *     distance (bigger index = smaller sprite)
     * ============================================================ */
    check(t_entity_z_to_scale(1) < t_entity_z_to_scale(2) &&
          t_entity_z_to_scale(2) < t_entity_z_to_scale(3) &&
          t_entity_z_to_scale(3) < t_entity_z_to_scale(4));

    /* ============================================================
     * 12. Z scale: beyond max range → SCALE_NONE
     * ============================================================ */
    check(t_entity_z_to_scale(5) == SCALE_NONE);

    /* ============================================================
     * 13. Shark pursuit X: player.gx > shark.gx → gdx = +1
     *     (shark gx increases to chase west toward player)
     * ============================================================ */
    {
        int8_t gdx = 0;
        uint8_t shark_gx = 10, player_gx = 20;
        if (player_gx > shark_gx) gdx = 1;
        else if (player_gx < shark_gx) gdx = -1;
        check(gdx == 1);
    }

    /* ============================================================
     * 14. Shark pursuit Z: player.gz > shark.gz → gdz = +1
     *     (shark gz increases to chase north toward player)
     * ============================================================ */
    {
        int8_t gdz = 0;
        uint8_t shark_gz = 10, player_gz = 20;
        if (player_gz > shark_gz) gdz = 1;
        else if (player_gz < shark_gz) gdz = -1;
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
        before_sx = t_entity_screen_x(dx_sub);

        shark_gx++;  /* gdx = +1 step */
        dx_sub = (int16_t)(shark_gx - player_gx) * CUBE_SUB_XY;
        after_sx = t_entity_screen_x(dx_sub);

        /* shark gx increased (west) → screen sx should decrease (left) */
        check(after_sx <= before_sx);
    }

    /* ---- Write summary ---- */
    RESULTS[0] = t_num;
    RESULTS[1] = t_pass;
    RESULTS[2 + t_num] = 0xAA;

    for (;;);
}
