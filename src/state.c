/*
 * state.c -- State machine dispatcher + all game state implementations
 *
 * States: TITLE, INTRO (stub), GAME, SUMMARY, GAMEOVER
 * Sprites drawn directly to screen RAM (no SP1).
 */

#pragma disable_warning 110
#include <string.h>
#include <stdint.h>
#include "../config/game_config.h"
#include "../include/state.h"
#include "../include/game.h"
#include "../include/gfx.h"
#include "../include/input.h"
#include "../include/hw.h"
#include "../include/sound.h"
#include "../include/depth.h"
#include "../include/vsync.h"
#include "../include/sprites.h"
#include "../include/player.h"
#include "../include/treasure.h"
#include "../include/hud.h"
#include "../include/minimap.h"
#include "../include/predators.h"
#include "../include/sealine.h"
#include "../include/vignette.h"
#include "../include/dzx0.h"
#include "../include/goo_data.h"
#include "../include/entity_render.h"
#include "../include/boat.h"

/* --- Bubblefield public API (from src/bubblefield.c) --- */
extern void init_bubbles(void);
extern void bubbles_set_count(uint8_t count);
extern void bubbles_erase_all(void);
extern void update_and_draw_bubbles(int8_t vx, int8_t vy, int8_t vz);
extern uint8_t sf_cull_y;
extern uint8_t sf_cull_y_floor;

/* --- Border colour (shared with depth.c and sound.c) --- */
extern uint8_t border_val;

/* --- Bubble counts per depth (0-indexed by gy) --- */
static const uint8_t depth_bubbles[3] = {
    BUBBLES_DEPTH1, BUBBLES_DEPTH2, BUBBLES_DEPTH3
};

/* --- Inter-frame state for STATE_GAME --- */
static int8_t vx, vy, vz;
static uint8_t frame;
static uint8_t oxygen_drain_ctr;

/* --- Bubblefield inertia (persists across frames) --- */
static int8_t sv_x, sv_y, sv_z;
static uint8_t dt_x, dt_y, dt_z;
static uint8_t was_edge_x, was_edge_y, was_edge_z;

/* --- Draw state: computed in logic phase, consumed next draw phase --- */
static int8_t bubble_vx, bubble_vy, bubble_vz;
static uint8_t spr_attr;

/* --- Persistent state across game states --- */
static uint8_t current_level;
static uint8_t game_over_flag;   /* 0 = level complete, 1 = death, 2 = surfaced early */
static uint8_t anim_timer;
static uint8_t key_debounce;

/* --- Function-pointer types --- */
typedef game_state_t (*init_fn)(void);
typedef game_state_t (*tick_fn)(void);

/* --- Level names (1-indexed, entry 0 unused) --- */
static const char * const level_names[] = {
    "",
    "The Deep",
    "The Reef",
    "The Shipwreck",
    "The Ruins",
    "Five Fathoms Deep"
};
#define NAMED_LEVELS 5

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Returns 1 if any keyboard key is pressed */
static uint8_t any_key_pressed(void)
{
    if ((read_keys(0x00FE) & 0x1F) != 0x1F) return 1;
    if (has_kempston && (read_keys(KEMP_PORT) & 0x10)) return 1;
    return 0;
}


/* Print a uint8_t value (1-3 digits) at character position */
static void print_num(uint8_t col, uint8_t row, uint8_t val)
{
    char buf[4];
    uint8_t i = 0;

    if (val >= 100) { buf[i++] = '0' + val / 100; val %= 100; }
    if (i > 0 || val >= 10) { buf[i++] = '0' + val / 10; val %= 10; }
    buf[i++] = '0' + val;
    buf[i] = '\0';
    print_at(col, row, buf);
}

/* Clear screen and set uniform attributes + border */
static void vignette_load(void)
{
    dzx0_decompress(vignette_zx0, SCREEN);
    memset(ATTR, depth_get_attr(current_depth), ATTR_GAME_SZ);
}

static void screen_clear(uint8_t attr, uint8_t border)
{
    memset(SCREEN, 0, PIX_SIZE);
    memset(ATTR, attr, ATTR_SZ);
    border_val = border;
    depth_restore_border();
}

/* ------------------------------------------------------------------ */
/* STATE_TITLE                                                         */
/* ------------------------------------------------------------------ */
static game_state_t state_title_init(void)
{
    current_level = 1;
    level.total_collected = 0;
    game_over_flag = 0;

    screen_clear(0x07, 0);   /* white ink, black paper, black border */

    print_at(3, 3,  "They That Go Down To");
    print_at(4, 4,  "The Sea In Ships");
    print_at(8, 10, "Press any key");
    print_at(3, 20, "Copyright Actual Size 2026");

    key_debounce = 1;
    anim_timer = 0;   /* reuse as title frame counter for PRNG seed */
    return STATE_TITLE;
}

static game_state_t state_title_tick(void)
{
    vsync_wait();
    anim_timer++;     /* free-running counter — keypress timing = entropy */
    if (key_debounce) {
        if (!any_key_pressed()) key_debounce = 0;
        return STATE_TITLE;
    }
    if (any_key_pressed()) {
        treasure_seed((uint16_t)anim_timer * 251 + 0xA3B7);
        return STATE_INTRO;
    }
    return STATE_TITLE;
}

/* ------------------------------------------------------------------ */
/* STATE_INTRO — level briefing screen                                 */
/* ------------------------------------------------------------------ */
static game_state_t state_intro_init(void)
{
    const char *name;

    /* Spawn treasures and predators so counts are available */
    treasure_spawn(current_level);
    predators_spawn(current_level);

    screen_clear(0x07, 0);   /* white ink, black paper */

    /* Level name */
    if (current_level <= NAMED_LEVELS)
        name = level_names[current_level];
    else
        name = "Descent into Madness";

    print_at(2, 4, "Level ");
    print_num(8, 4, current_level);
    print_at(2, 6, name);

    /* Objectives */
    print_at(2, 10, "Relics to recover: ");
    print_num(21, 10, level.arch_count);

    print_at(2, 12, "Salvage on sonar: ");
    print_num(20, 12, level.treasure_count - level.arch_count);

    if (pred_ray_count) {
        print_at(2, 14, "Rays: ");
        print_num(8, 14, pred_ray_count);
    }
    if (pred_shark_count) {
        print_at(2, 15, "Sharks: ");
        print_num(10, 15, pred_shark_count);
    }
    if (pred_goo_count) {
        print_at(2, 16, "GOOs: ");
        print_num(8, 16, pred_goo_count);
    }

    print_at(5, 18, "Press any key to dive");

    key_debounce = 1;
    return STATE_INTRO;
}

static game_state_t state_intro_tick(void)
{
    vsync_wait();
    if (key_debounce) {
        if (!any_key_pressed()) key_debounce = 0;
        return STATE_INTRO;
    }
    if (any_key_pressed()) return STATE_LEVEL_INTRO;
    return STATE_INTRO;
}

/* ------------------------------------------------------------------ */
/* STATE_LEVEL_INTRO — boat scroll-in animation                        */
/* ------------------------------------------------------------------ */

#define LINTRO_SEA_Y        64    /* 4 char rows above centre */
#define LINTRO_TARGET_PX    104   /* 13 * 8 = screen centre for 6-byte sprite */
#define LINTRO_BOAT_WL      20
#define LINTRO_BOAT_W       6
#define LINTRO_BOAT_H       32
#define LINTRO_SCROLL_PX    2     /* pixels per frame */
#define LINTRO_BOB_FRAMES   50    /* 1 second at 50 fps */
#define LINTRO_DIVER_X      (DIVER_X >> 3)  /* col 15 */
#define LINTRO_BOAT_ATTR    0x2E  /* yellow ink, cyan paper */
#define LINTRO_BG_ATTR      0x29  /* depth 1: blue ink, cyan paper */
#define LINTRO_DIVER_ATTR   0x2E  /* matches game starting attr */
#define LINTRO_ATTR_ROW     4     /* topmost char row the boat can touch */
#define LINTRO_ATTR_H       7     /* char rows 4-10 cover full bob range */

#define LINTRO_SCROLL        0
#define LINTRO_BOB           1
#define LINTRO_DESCEND       2
#define LINTRO_HOLD          3
#define LINTRO_DESCEND_SPEED 3     /* frames per pixel of descent */
#define LINTRO_HOLD_FRAMES   25    /* 0.5 seconds at 50 fps */

/* Same sine table as sea line — ±6 px, 32 entries */
static const int8_t lintro_sine[32] = {
     0,  1,  2,  3,  4,  5,  5,  6,
     6,  6,  5,  5,  4,  3,  2,  1,
     0, -1, -2, -3, -4, -5, -5, -6,
    -6, -6, -5, -5, -4, -3, -2, -1
};

static int16_t lintro_px;
static uint8_t lintro_phase;
static uint8_t lintro_timer;
static uint8_t lintro_sea_phase;
static uint8_t lintro_drawn;
static int16_t lintro_prev_px;
static uint8_t lintro_prev_y;
static uint8_t lintro_diver_y;      /* diver descent target Y */
static uint8_t lintro_diver_drawn;  /* diver XOR currently on screen */
static uint8_t lintro_diver_draw_y; /* Y where XOR is on screen */
static uint8_t lintro_diver_frame;  /* frame index currently XOR'd */
static uint8_t lintro_saved_vsync; /* saved vsync_mode for restore */
static uint8_t lintro_sl_prev[32]; /* sea line prev Y per column */

/* Set clipped attribute rect covering the full bob range */
static void lintro_set_attr(int16_t px, uint8_t attr)
{
    int8_t col = (int8_t)(px >> 3);
    uint8_t shift = (uint8_t)(px & 7);
    uint8_t w = shift ? LINTRO_BOAT_W + 1 : LINTRO_BOAT_W;
    uint8_t start_col;

    if (col < 0) {
        int8_t skip = -col;
        if ((uint8_t)skip >= w) return;
        w -= (uint8_t)skip;
        start_col = 0;
    } else {
        start_col = (uint8_t)col;
        if (start_col >= 32) return;
        if (start_col + w > 32) w = 32 - start_col;
    }
    set_attr_rect(start_col, LINTRO_ATTR_ROW, w, LINTRO_ATTR_H, attr);
}

static uint8_t lintro_boat_y(void)
{
    uint8_t center_col = (uint8_t)(((lintro_px >> 3) + 3) & 0x1F);
    uint8_t idx = (center_col + lintro_sea_phase) & 31;
    return (uint8_t)(LINTRO_SEA_Y - LINTRO_BOAT_WL + lintro_sine[idx]);
}

/* XOR sea line: combined erase-old + draw-new, no attrs */
static void lintro_sealine_tick(void)
{
    uint8_t col, idx;
    int16_t sy;

    for (col = 0; col < 32; col++) {
        if (lintro_sl_prev[col] < 192)
            SCREEN[scr_off(col << 3, lintro_sl_prev[col])] ^= 0xFF;

        idx = (col + lintro_sea_phase) & 31;
        sy = (int16_t)LINTRO_SEA_Y + lintro_sine[idx];

        if (sy >= 0 && sy < 192) {
            SCREEN[scr_off(col << 3, (uint8_t)sy)] ^= 0xFF;
            lintro_sl_prev[col] = (uint8_t)sy;
        } else {
            lintro_sl_prev[col] = 255;
        }
    }
}

static game_state_t state_level_intro_init(void)
{
    uint8_t i;

    lintro_saved_vsync = vsync_mode;
    vsync_mode = 0;

    screen_clear(LINTRO_BG_ATTR, 1);

    lintro_px = -(int16_t)(LINTRO_BOAT_W * 8);
    lintro_phase = LINTRO_SCROLL;
    lintro_timer = 0;
    lintro_sea_phase = 0;
    lintro_drawn = 0;
    lintro_diver_drawn = 0;
    for (i = 0; i < 32; i++) lintro_sl_prev[i] = 255;

    return STATE_LEVEL_INTRO;
}

static game_state_t state_level_intro_tick(void)
{
    uint8_t boat_y;
    int16_t old_px;
    int8_t  old_col, new_col;

    vsync_wait();

    old_px = lintro_px;

    switch (lintro_phase) {
    case LINTRO_SCROLL:
        lintro_px += LINTRO_SCROLL_PX;
        if (lintro_px >= LINTRO_TARGET_PX) {
            lintro_px = LINTRO_TARGET_PX;
            lintro_phase = LINTRO_BOB;
            lintro_timer = 0;
        }
        break;

    case LINTRO_BOB:
        if (++lintro_timer >= LINTRO_BOB_FRAMES) {
            lintro_phase = LINTRO_DESCEND;
            lintro_timer = 0;
        }
        break;

    case LINTRO_DESCEND:
        if (++lintro_timer >= LINTRO_DESCEND_SPEED) {
            lintro_timer = 1;
            lintro_diver_y++;
            if (lintro_diver_y >= DIVER_Y) {
                lintro_phase = LINTRO_HOLD;
                lintro_timer = 0;
                lintro_diver_y = DIVER_Y;
            }
        }
        break;

    case LINTRO_HOLD:
        if (++lintro_timer >= LINTRO_HOLD_FRAMES) {
            vsync_mode = lintro_saved_vsync;
            return STATE_GAME;
        }
        break;
    }

    boat_y = lintro_boat_y();

    old_col = (int8_t)(old_px >> 3);
    new_col = (int8_t)(lintro_px >> 3);

    /* Delta-clear: erase exposed column on byte boundary crossing */
    if (lintro_drawn) {
        if (new_col != old_col && old_col >= 0) {
            clear_blit(old_col, lintro_prev_y, 1, LINTRO_BOAT_H);
            lintro_sl_prev[(uint8_t)old_col] = 255;
        }
        if (lintro_prev_y != boat_y) {
            uint8_t prev_w = (lintro_prev_px & 7)
                             ? LINTRO_BOAT_W + 1 : LINTRO_BOAT_W;
            int8_t prev_col = (int8_t)(lintro_prev_px >> 3);
            if (lintro_prev_y < boat_y)
                clear_blit(prev_col, lintro_prev_y,
                           prev_w, boat_y - lintro_prev_y);
            else
                clear_blit(prev_col,
                           (uint8_t)(boat_y + LINTRO_BOAT_H),
                           prev_w, lintro_prev_y - boat_y);
        }
    }

    /* Sea line: XOR erase old + draw new */
    lintro_sealine_tick();

    /* Draw boat */
    write_blit_px(lintro_px, boat_y, boat_bitmap,
                  LINTRO_BOAT_W, LINTRO_BOAT_H);

    /* Attrs: clear exposed column, set boat range */
    if (new_col != old_col && old_col >= 0)
        set_attr_rect((uint8_t)old_col, LINTRO_ATTR_ROW,
                      1, LINTRO_ATTR_H, LINTRO_BG_ATTR);
    lintro_set_attr(lintro_px, LINTRO_BOAT_ATTR);

    /* Diver: init Y on first DESCEND frame */
    if (lintro_phase == LINTRO_DESCEND && lintro_timer == 0) {
        lintro_diver_y = boat_y + LINTRO_BOAT_H;
        lintro_timer = 1;
    }

    /* XOR diver: erase + draw back-to-back to minimise flicker */
    if (lintro_phase == LINTRO_DESCEND || lintro_phase == LINTRO_HOLD) {
        uint8_t cur_frame = (lintro_sea_phase >> 4) & 1;

        xor16_x = DIVER_X;
        xor16_attr = LINTRO_DIVER_ATTR;

        if (lintro_diver_drawn &&
            (lintro_diver_draw_y != lintro_diver_y ||
             lintro_diver_frame != cur_frame)) {
            xor16_y = lintro_diver_draw_y;
            xor16_spr = sprites_get_frame(lintro_diver_frame);
            xor_sprite_16();
            lintro_diver_drawn = 0;
        }

        if (!lintro_diver_drawn) {
            xor16_y = lintro_diver_y;
            xor16_spr = sprites_get_frame(cur_frame);
            xor_sprite_16();
            set_attr_rect(LINTRO_DIVER_X, lintro_diver_y >> 3,
                          2, 3, LINTRO_DIVER_ATTR);
            lintro_diver_drawn = 1;
            lintro_diver_draw_y = lintro_diver_y;
            lintro_diver_frame = cur_frame;
        }
    }

    lintro_drawn = 1;
    lintro_prev_px = lintro_px;
    lintro_prev_y = boat_y;
    lintro_sea_phase++;

    return STATE_LEVEL_INTRO;
}

/* ------------------------------------------------------------------ */
/* STATE_GAME                                                          */
/* ------------------------------------------------------------------ */
static game_state_t state_game_init(void)
{
    vx = 0;
    vy = 0;
    vz = 0;
    sv_x = 0; sv_y = 0; sv_z = 0;
    dt_x = 0; dt_y = 0; dt_z = 0;
    was_edge_x = 0; was_edge_y = 0; was_edge_z = 0;
    bubble_vx = 0; bubble_vy = 0; bubble_vz = 0;
    frame = 0;
    oxygen_drain_ctr = OXYGEN_DRAIN_RATE;

    if (game_over_flag == 2) {
        /* Continuing after surfacing — keep position, replenish oxygen */
        player.oxygen = OXYGEN_MAX;
        player.sub_z = CUBE_SUB_Z - 1;
        player.invuln_timer = 0;
    } else {
        player_init();
    }

    bubbles_set_count(BUBBLES_DEPTH1);
    init_bubbles();

    /* Clear screen, set up depth colours, overlay vignette */
    sprites_init();
    depth_set(1);
    vignette_load();
    spr_attr = depth_get_paper() | 0x06;

    /* Reset predator and treasure draw tracking */
    predators_init();
    treasure_init_render();

    /* Initialise HUD pixel patterns */
    hud_init();

    /* Reset minimap update timer and draw initial depth bar */
    minimap_init();
    depth_indicator_init();

    /* Sea line and sea floor */
    sealine_init();
    seafloor_init();
    sf_cull_y = 0;
    sf_cull_y_floor = 0;

    return STATE_GAME;
}

static game_state_t state_game_tick(void)
{
    uint8_t k;
    uint8_t kq, ka, ko, kp, kz, kx;
    uint8_t depth_changed;
    uint8_t damage;

    /* ================================================================
     * Frame structure: draw-then-compute
     *
     * The loop is split into two phases separated by vsync_wait():
     *
     *   1. DRAW PHASE  — runs immediately after vsync, while the
     *      beam is in the top border (~14 336 T-states of safe
     *      window).  All screen writes happen here, racing ahead
     *      of the beam.  Uses bubble_vx/vy/vz, spr_attr, and
     *      player/predator/treasure positions computed by the
     *      PREVIOUS tick's logic phase.
     *
     *   2. LOGIC PHASE  — runs while the beam scans the active
     *      display.  No screen writes (except depth-change hide,
     *      which is a rare one-shot).  Reads input, moves entities,
     *      checks collisions, and writes bubble_vx/vy/vz + spr_attr
     *      for the NEXT draw phase.
     *
     * This gives one frame (20 ms) of display latency between input
     * and visible response — imperceptible to the player.
     * ================================================================ */

    /* === DRAW PHASE =============================================== */
    vsync_wait();

    predators_erase();

    {
        uint8_t trans = depth_is_transitioning();
        uint8_t at_floor = (player.gy == GRID_D - 1 && !trans);
        uint8_t at_bottom = (player.gy == GRID_D - 1);
        int16_t sly_i;
        uint16_t total_depth;

        /* Sea line: pinned at world top (gy=0, sub_z=0).
         * Not shown at the bottommost subcube. */
        total_depth = (uint16_t)player.gy * CUBE_SUB_Z + player.sub_z;
        sly_i = SEALINE_DEFAULT_Y -
            (int16_t)(total_depth * SEALINE_DEFAULT_Y / CUBE_SUB_Z);

        if (sly_i >= 0 && !trans && !at_bottom) {
            sf_cull_y = (uint8_t)(sly_i + 6);
        } else {
            sf_cull_y = 0;
        }

        /* Sea floor cull: clip bubbles below the floor line.
         * Offset by 4 so 4x4 close bubbles never overlap the floor. */
        if (at_floor) {
            uint8_t sfy = VIEW_H - 1 -
                (uint8_t)((uint16_t)player.sub_z * (VIEW_H - 1 - DIVER_Y - 16) / CUBE_SUB_Z);
            sf_cull_y_floor = (sfy > 4) ? sfy - 4 : 1;
            update_and_draw_bubbles(bubble_vx, bubble_vy, bubble_vz);
            seafloor_update(sfy);
        } else {
            sf_cull_y_floor = 0;
            update_and_draw_bubbles(bubble_vx, bubble_vy, bubble_vz);
            seafloor_erase();
        }

        if (sly_i >= 0 && !trans && !at_bottom)
            sealine_update((uint8_t)sly_i);
        else
            sealine_erase();

        /* Entity Y anchor: top of a 32px sprite at the depth's feature */
        if (player.gy == 0) {
            int16_t ey = (sly_i >= 0) ? sly_i - 32 : 20;
            if (ey < PRED_Y_MIN) ey = PRED_Y_MIN;
            if (ey > PRED_Y_MAX) ey = PRED_Y_MAX;
            env_entity_y = (uint8_t)ey;
        } else if (player.gy == 1) {
            env_entity_y = DIVER_Y - 8;
        } else {
            int16_t ey;
            if (at_floor) {
                uint8_t sfy2 = VIEW_H - 1 -
                    (uint8_t)((uint16_t)player.sub_z *
                    (VIEW_H - 1 - DIVER_Y - 16) / CUBE_SUB_Z);
                ey = (int16_t)sfy2 - 32;
            } else {
                ey = VIEW_H - 40;
            }
            if (ey < PRED_Y_MIN) ey = PRED_Y_MIN;
            if (ey > PRED_Y_MAX) ey = PRED_Y_MAX;
            env_entity_y = (uint8_t)ey;
        }
    }

    if (!depth_is_transitioning())
        predators_draw(frame);
    predators_cleanup_attrs();
    treasure_render(frame);
    sprites_player_draw((frame >> 3) & 1);
    sprites_player_set_colour(spr_attr);

    hud_draw(player.oxygen, player.health);

    minimap_draw();
    depth_indicator_draw();

    /* === LOGIC PHASE ============================================== */

    /* --- Sample keyboard + joystick in one call --- */
    k  = scan_input();
    kq = k & INPUT_FWD;
    ka = k & INPUT_BACK;
    ko = k & INPUT_LEFT;
    kp = k & INPUT_RIGHT;
    kz = k & INPUT_DESC;
    kx = k & INPUT_ASC;

    /* Horizontal (O/P) — friction model */
    if (ko && vx <  SPEED) vx++;
    else if (kp && vx > -SPEED) vx--;
    else if ((frame & 7) == 0) { if (vx > 0) vx--; else if (vx < 0) vx++; }

    /* Forward/backward (Q/A) — friction model */
    if (kq && vz > -SPEED) vz--;
    else if (ka && vz <  SPEED) vz++;
    else if ((frame & 7) == 0) { if (vz > 0) vz--; else if (vz < 0) vz++; }

    /* Vertical (Z = descend, X = ascend) — buoyancy model: diver floats up by default */
    if (kz && vy > -SPEED) vy--;
    else if (kx && vy < SPEED + 1) vy++;
    else if (vy < SPEED) vy++;

    /* --- Apply player speed divisor (fractional accumulator) --- */
    {
        int8_t raw_vx = vx, raw_vy = vy, raw_vz = vz;
#if PLAYER_SPEED_DIV > 1
        {
            static int8_t frac_x, frac_y, frac_z;

            frac_x += raw_vx;
            if (frac_x >= PLAYER_SPEED_DIV)       { vx =  1; frac_x -= PLAYER_SPEED_DIV; }
            else if (frac_x <= -PLAYER_SPEED_DIV)  { vx = -1; frac_x += PLAYER_SPEED_DIV; }
            else                                    { vx =  0; }

            frac_y += raw_vy;
            if (frac_y >= PLAYER_DEPTH_DIV) {
                vy = 1; frac_y -= PLAYER_DEPTH_DIV;
                if (frac_y >= PLAYER_DEPTH_DIV) { vy = 2; frac_y -= PLAYER_DEPTH_DIV; }
            } else if (frac_y <= -PLAYER_DEPTH_DIV) {
                vy = -1; frac_y += PLAYER_DEPTH_DIV;
                if (frac_y <= -PLAYER_DEPTH_DIV) { vy = -2; frac_y += PLAYER_DEPTH_DIV; }
            } else { vy = 0; }

            frac_z += raw_vz;
            if (frac_z >= PLAYER_SPEED_DIV)       { vz =  1; frac_z -= PLAYER_SPEED_DIV; }
            else if (frac_z <= -PLAYER_SPEED_DIV)  { vz = -1; frac_z += PLAYER_SPEED_DIV; }
            else                                    { vz =  0; }
        }
#endif

        /* --- Bubblefield inertia: snap while key held, coast when released --- */
        if (ko | kp) { sv_x = raw_vx; dt_x = BUBBLE_DRIFT_FRAMES; }
        else if (sv_x != 0 && --dt_x == 0) {
            dt_x = BUBBLE_DRIFT_FRAMES;
            if (sv_x > 0) sv_x--; else sv_x++;
        }

        /* Vertical: always tracks velocity (buoyancy keeps it moving) */
        sv_y = raw_vy;

        if (kq | ka) { sv_z = raw_vz; dt_z = BUBBLE_DRIFT_FRAMES; }
        else if (sv_z != 0 && --dt_z == 0) {
            dt_z = BUBBLE_DRIFT_FRAMES;
            if (sv_z > 0) sv_z--; else sv_z++;
        }

        bubble_vx = sv_x;
        bubble_vy = sv_y;
        bubble_vz = sv_z;
    }

    /* --- Player movement & cube traversal --- */
    depth_changed = player_update(vx, vy, vz);

    /* Position-based world edge detection (reliable every frame) */
    {
        uint8_t edge_x = (player.gx == 0 && player.sub_x == 0) ||
                         (player.gx == GRID_W - 1 && player.sub_x >= CUBE_SUB_XY - 1);
        uint8_t edge_y = (player.gy == 0 && player.sub_z == 0) ||
                         (player.gy == GRID_D - 1 && player.sub_z >= CUBE_SUB_Z - 1);
        uint8_t edge_z = (player.gz == 0 && player.sub_y == 0) ||
                         (player.gz == GRID_H - 1 && player.sub_y >= CUBE_SUB_XY - 1);

        if (edge_x && !was_edge_x) { sv_x = -sv_x; bubble_vx = sv_x; }
        else if (edge_x) { sv_x = 0; bubble_vx = 0; }
        was_edge_x = edge_x;

        if (edge_y) { sv_y = 0; bubble_vy = 0; }
        was_edge_y = edge_y;

        if (edge_z && !was_edge_z) { sv_z = -sv_z; bubble_vz = sv_z; }
        else if (edge_z) { sv_z = 0; bubble_vz = 0; }
        was_edge_z = edge_z;
    }
    if (depth_changed) {
        bubbles_set_count(depth_bubbles[player.gy]);
        predators_hide_all();
        treasure_hide_all();
    }

    /* --- Predator AI & movement --- */
    predators_update();

    /* Advance depth colour transition (no-op if not transitioning) */
    {
        uint8_t was_trans = depth_is_transitioning();
        depth_transition_tick();
        if (was_trans && !depth_is_transitioning())
            minimap_init();
    }

    /* --- Treasure proximity check --- */
    treasure_check_collection();

    /* --- Surfacing: level complete if relics done, status screen if not --- */
    if (player.gy == 0 && player.at_bound_y) {
        if (level.arch_collected >= level.arch_count) {
            game_over_flag = 0;
            return STATE_SUMMARY;
        } else {
            game_over_flag = 2;
            return STATE_SUMMARY;
        }
    }

    /* --- Predator collision check --- */
    damage = predators_check_collision();
    if (damage == 255) {
        game_over_flag = 1;
        return STATE_GOO_DEATH;
    }
    if (damage > 0 && player.invuln_timer == 0) {
        if (player.health > damage)
            player.health -= damage;
        else
            player.health = 0;
        player.invuln_timer = INVULNERABLE_FRAMES;
        sfx_damage_jingle();
    }

    /* --- Check game over conditions --- */
    if (player.health == 0 || player.oxygen == 0) {
        game_over_flag = 1;
        return STATE_GAMEOVER;
    }

    /* --- Oxygen drain --- */
    if (player.oxygen > 0) {
        if (--oxygen_drain_ctr == 0) {
            oxygen_drain_ctr = OXYGEN_DRAIN_RATE;
            player.oxygen--;
        }
    }

    /* --- Compute draw state for next frame --- */
    if (player.invuln_timer > 0 && (player.invuln_timer & 0x02))
        spr_attr = depth_get_paper() | (ATTR[0] & 0x07);
    else
        spr_attr = depth_get_paper() | 0x06;

    frame++;

    /* --- Distance-based sonar ping (closest same-depth object) --- */
    {
        uint8_t best_dist = 255, best_type = SONAR_TREASURE;
        uint8_t i, d;
        int8_t dx, dz;

        for (i = 0; i < level.treasure_count; i++) {
            if (treasures[i].collected) continue;
            if (treasures[i].gy != player.gy) continue;
            dx = (int8_t)(player.gx - treasures[i].gx);
            dz = (int8_t)(player.gz - treasures[i].gz);
            if (dx < 0) dx = -dx;
            if (dz < 0) dz = -dz;
            d = (uint8_t)dx > (uint8_t)dz ? (uint8_t)dx : (uint8_t)dz;
            if (d < best_dist) { best_dist = d; best_type = SONAR_TREASURE; }
        }

        for (i = 0; i < predator_count; i++) {
            if (!predators[i].active) continue;
            if (predators[i].type != player.gy) continue;
            dx = (int8_t)(player.gx - predators[i].gx);
            dz = (int8_t)(player.gz - predators[i].gz);
            if (dx < 0) dx = -dx;
            if (dz < 0) dz = -dz;
            d = (uint8_t)dx > (uint8_t)dz ? (uint8_t)dx : (uint8_t)dz;
            if (d < best_dist) {
                best_dist = d;
                best_type = (predators[i].type == PRED_GOO) ?
                    SONAR_GOO : SONAR_PREDATOR;
            }
        }

        sonar_update(best_dist, best_type);
    }
    beep_tick();

    return STATE_GAME;
}

/* ------------------------------------------------------------------ */
/* STATE_GAMEOVER — colour-cycle death animation                       */
/* ------------------------------------------------------------------ */

/* 8-step colour cycle: bright white → yellow → cyan → green →
 * magenta → red → blue → black.  ~6 frames per step = 48 frames. */
static const uint8_t death_attrs[8] = {
    0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x00
};
static const uint8_t death_borders[8] = {
    7, 6, 5, 4, 3, 2, 1, 0
};
#define DEATH_STEPS        8
#define DEATH_FRAMES_PER   6
#define DEATH_TOTAL_FRAMES (DEATH_STEPS * DEATH_FRAMES_PER)

static game_state_t state_gameover_init(void)
{
    anim_timer = 0;
    return STATE_GAMEOVER;
}

static game_state_t state_gameover_tick(void)
{
    uint8_t step;

    vsync_wait();

    step = anim_timer / DEATH_FRAMES_PER;
    if (step >= DEATH_STEPS) step = DEATH_STEPS - 1;

    memset(ATTR, death_attrs[step], ATTR_SZ);
    border_val = death_borders[step];
    depth_restore_border();

    /* Continuous high tone during death */
    sfx_play_tone(10);

    anim_timer++;
    if (anim_timer >= DEATH_TOTAL_FRAMES)
        return STATE_SUMMARY;

    return STATE_GAMEOVER;
}

/* ------------------------------------------------------------------ */
/* STATE_SUMMARY — level results screen                                */
/* ------------------------------------------------------------------ */
static game_state_t state_summary_init(void)
{
    const char *name;

    screen_clear(0x07, 0);   /* white ink, black paper */

    if (game_over_flag == 1) {
        print_at(11, 3, "Game Over");
    } else if (game_over_flag == 2) {
        print_at(8, 3, "Surfaced Early");
    } else {
        print_at(9, 3, "Level Complete!");
    }

    /* Level name */
    if (current_level <= NAMED_LEVELS)
        name = level_names[current_level];
    else
        name = "Descent into Madness";

    print_at(2, 6, "Level ");
    print_num(8, 6, current_level);
    print_at(2, 7, name);

    /* Treasure stats */
    print_at(2, 10, "Relics recovered: ");
    print_num(20, 10, level.arch_collected);
    print_at(22, 10, "/");
    print_num(23, 10, level.arch_count);

    print_at(2, 12, "Salvage collected: ");
    print_num(21, 12, level.collected_count - level.arch_collected);
    print_at(23, 12, "/");
    print_num(24, 12, level.treasure_count - level.arch_count);

    print_at(2, 14, "Total this dive: ");
    print_num(19, 14, level.collected_count);

    if (game_over_flag == 1) {
        print_at(6, 18, "Press any key");
    } else if (game_over_flag == 2) {
        print_at(3, 18, "Press any key to");
        print_at(7, 19, "dive again");
    } else {
        print_at(4, 18, "Press any key for");
        print_at(8, 19, "next level");
    }

    key_debounce = 1;
    return STATE_SUMMARY;
}

static game_state_t state_summary_tick(void)
{
    vsync_wait();
    if (key_debounce) {
        if (!any_key_pressed()) key_debounce = 0;
        return STATE_SUMMARY;
    }
    if (any_key_pressed()) {
        if (game_over_flag == 1)
            return STATE_TITLE;
        if (game_over_flag == 2)
            return STATE_GAME;
        current_level++;
        return STATE_INTRO;
    }
    return STATE_SUMMARY;
}

/* ------------------------------------------------------------------ */
/* STATE_GOO_DEATH — anglerfish reveal + swallow animation             */
/* ------------------------------------------------------------------ */

#define SCRATCH_BUF ((uint8_t *)0x6000)
#define GOO_DRAW_COL    GOO_CROP_COL + 1
#define GOO_DRAW_ROW    GOO_CROP_ROW - 8
#define GOO_DRAW_END_Y  GOO_CROP_ROW - 8 + GOO_CROP_H

static const uint8_t * const goo_frames[4] = {
    goo_frame1, goo_frame2, goo_frame3, goo_frame4
};
static const uint8_t goo_hold[4] = {
    80, 70, 90, 80
};

static uint8_t goo_step;
static uint8_t goo_timer;

#define BIT_REV_TABLE  ((uint8_t *)0x6900)
#define BIT_REV_PAGE   0x69

static void init_bit_rev(void)
{
    uint16_t i;
    for (i = 0; i < 256; i++) {
        uint8_t b = (uint8_t)i;
        b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
        b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
        b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
        BIT_REV_TABLE[i] = b;
    }
}

static void write_crop_to_screen(void) __naked
{
    __asm

    push    bc
    push    de
    push    ix

    ld      ix, #0x6000             ; SCRATCH_BUF
    ld      c, #GOO_DRAW_ROW        ; y = starting row

_wr_row:
    ;; ---- compute screen address for y=C, column 0 ----
    ld      a, c
    ld      e, a                    ; save y in E
    and     #0x07
    or      #0x40                   ; screen base high nibble
    ld      h, a
    ld      a, e
    and     #0xC0
    rrca
    rrca
    rrca
    or      h
    ld      h, a                    ; H = 010 TT SSS
    ld      a, e
    and     #0x38
    rlca
    rlca
    ld      l, a                    ; L = RRR 00000 (col 0)

    ;; ---- Pass 1: write source to left screen ----
    ld      a, l
    or      #GOO_DRAW_COL           ; add left column offset
    ld      l, a

    push    ix                      ; save source ptr for pass 2

    ld      b, #GOO_CROP_W
_wr_left:
    ld      a, 0(ix)
    inc     ix
    xor     (hl)
    ld      (hl), a
    inc     hl
    djnz    _wr_left

    ;; ---- Pass 2 setup ----
    ld      d, h                    ; D = row high byte
    ld      a, l
    add     a, #(GOO_CROP_W - 1)
    ld      e, a                    ; DE = right screen end

    ld      h, #BIT_REV_PAGE        ; H = table page (0x69)

    pop     ix                      ; reload source start

    ;; ---- Pass 2: bit-reverse + write to right screen ----
    ld      b, #GOO_CROP_W
_wr_right:
    ld      a, 0(ix)
    inc     ix
    ld      l, a                    ; table index
    ld      a, (hl)                 ; bit-reversed byte
    ex      de, hl                  ; HL=screen, DE=table
    xor     (hl)
    ld      (hl), a
    dec     hl
    ex      de, hl                  ; DE=screen-1, HL=table
    djnz    _wr_right

    ;; ---- next row ----
    inc     c
    ld      a, c
    cp      #GOO_DRAW_END_Y
    jp      nz, _wr_row

    pop     ix
    pop     de
    pop     bc
    ret

    __endasm;
}

static game_state_t state_goo_death_init(void)
{
    goo_step = 0;
    goo_timer = 0;

    init_bit_rev();
    dzx0_decompress(goo_frames[0], SCRATCH_BUF);
    write_crop_to_screen();
    sprites_player_draw(0);
    sprites_player_set_colour(depth_get_paper() | 0x06);

    sfx_play_note(30, 255);

    return STATE_GOO_DEATH;
}

static game_state_t state_goo_death_tick(void)
{
    vsync_wait();

    if (++goo_timer >= goo_hold[goo_step]) {
        goo_step++;
        if (goo_step >= 4)
            return STATE_GAMEOVER;

        goo_timer = 0;

        write_crop_to_screen();
        dzx0_decompress(goo_frames[goo_step], SCRATCH_BUF);
        write_crop_to_screen();
        sprites_player_draw(0);
        sprites_player_set_colour(depth_get_paper() | 0x06);

        if (goo_step < 2)
            sfx_play_note(20, (uint8_t)(255 - goo_step * 60));
        else
            sfx_play_tone(40);
    }

    return STATE_GOO_DEATH;
}

/* ------------------------------------------------------------------ */
/* Dispatch tables                                                     */
/* ------------------------------------------------------------------ */
static const init_fn inits[STATE_COUNT] = {
    state_title_init,         /* STATE_TITLE       */
    state_intro_init,         /* STATE_INTRO       */
    state_level_intro_init,   /* STATE_LEVEL_INTRO */
    state_game_init,          /* STATE_GAME        */
    state_summary_init,       /* STATE_SUMMARY     */
    state_gameover_init,      /* STATE_GAMEOVER    */
    state_goo_death_init      /* STATE_GOO_DEATH   */
};

static const tick_fn ticks[STATE_COUNT] = {
    state_title_tick,         /* STATE_TITLE       */
    state_intro_tick,         /* STATE_INTRO       */
    state_level_intro_tick,   /* STATE_LEVEL_INTRO */
    state_game_tick,          /* STATE_GAME        */
    state_summary_tick,       /* STATE_SUMMARY     */
    state_gameover_tick,      /* STATE_GAMEOVER    */
    state_goo_death_tick      /* STATE_GOO_DEATH   */
};

/* ------------------------------------------------------------------ */
/* State machine runner                                                */
/* ------------------------------------------------------------------ */
void state_run(void)
{
    game_state_t cur = STATE_TITLE;
    game_state_t next;

    next = inits[cur]();

    while (1) {
        if (next != cur) {
            cur = next;
            next = inits[cur]();
        }
        next = ticks[cur]();
    }
}
