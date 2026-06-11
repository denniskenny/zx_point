/*
 * state.c -- State machine dispatcher + all game state implementations
 *
 * States: TITLE, INTRO (stub), GAME, SUMMARY, GAMEOVER
 * Sprites drawn directly to screen RAM (no SP1).
 */

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

/* --- Bubblefield public API (from src/bubblefield.c) --- */
extern void init_bubbles(void);
extern void bubbles_set_count(uint8_t count);
extern void bubbles_erase_all(void);
extern void update_and_draw_bubbles(int8_t vx, int8_t vy, int8_t vz);

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
static uint8_t game_over_flag;   /* 1 = death, 0 = level complete */
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
    /* Port 0x00FE with all address lines low selects all half-rows */
    return (read_keys(0x00FE) & 0x1F) != 0x1F;
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
    print_at(2, 10, "Treasures: ");
    print_num(13, 10, level.treasure_count);

    print_at(2, 12, "Relics to recover: ");
    print_num(21, 12, level.arch_count);

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
    if (any_key_pressed()) return STATE_GAME;
    return STATE_INTRO;
}

/* ------------------------------------------------------------------ */
/* STATE_GAME                                                          */
/* ------------------------------------------------------------------ */
static game_state_t state_game_init(void)
{
    vx = 0;
    vy = 0;
    vz = -2;   /* gentle forward drift */
    sv_x = 0; sv_y = 0; sv_z = 0;
    dt_x = 0; dt_y = 0; dt_z = 0;
    was_edge_x = 1; was_edge_y = 1; was_edge_z = 0;
    bubble_vx = 0; bubble_vy = 0; bubble_vz = 0;
    frame = 0;
    oxygen_drain_ctr = OXYGEN_DRAIN_RATE;

    /* Initialise player at grid centre, full health/oxygen */
    player_init();

    bubbles_set_count(BUBBLES_DEPTH1);
    init_bubbles();

    /* Clear screen and set up depth colours */
    sprites_init();
    depth_set(1);
    spr_attr = depth_get_paper() | 0x06;

    /* Reset predator and treasure draw tracking */
    predators_init();
    treasure_init_render();

    /* Initialise HUD pixel patterns */
    hud_init();

    /* Reset minimap update timer */
    minimap_init();

    return STATE_GAME;
}

static game_state_t state_game_tick(void)
{
    uint8_t k, joy;
    uint8_t kq, kw, ka, ks, ko, kp;
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
    update_and_draw_bubbles(bubble_vx, bubble_vy, bubble_vz);
    if (!depth_is_transitioning())
        predators_draw(frame);
    treasure_render(frame);
    sprites_player_draw((frame >> 3) & 1);
    sprites_player_set_colour(spr_attr);

    hud_draw(player.oxygen, player.health);

    minimap_draw();
    depth_indicator_draw();

    /* === LOGIC PHASE ============================================== */

    /* --- Sample keyboard and adjust velocity --- */
    k  = read_keys(KEY_QWERT);
    kq = !(k & 0x01);
    kw = !(k & 0x02);

    k  = read_keys(KEY_ASDFG);
    ka = !(k & 0x01);
    ks = !(k & 0x02);

    k  = read_keys(KEY_POIUY);
    kp = !(k & 0x01);
    ko = !(k & 0x02);

    if (has_kempston) {
        joy = read_keys(KEMP_PORT);
        if (joy & 0x08) kq = 1;
        if (joy & 0x04) ka = 1;
        if (joy & 0x02) ko = 1;
        if (joy & 0x01) kp = 1;
        if (joy & 0x10) kw = 1;
        if (joy & 0x20) ks = 1;
    }

    if (kq && vy <  SPEED) vy++;
    else if (ka && vy > -SPEED) vy--;
    else if ((frame & 7) == 0) { if (vy > 0) vy--; else if (vy < 0) vy++; }

    if (ko && vx <  SPEED) vx++;
    else if (kp && vx > -SPEED) vx--;
    else if ((frame & 7) == 0) { if (vx > 0) vx--; else if (vx < 0) vx++; }

    if (kw && vz > -SPEED) vz--;
    else if (ks && vz <  SPEED) vz++;
    else if ((frame & 7) == 0) { if (vz > 0) vz--; else if (vz < 0) vz++; }

    /* --- Apply player speed divisor (fractional accumulator) --- */
    {
        int8_t raw_vx = vx, raw_vy = vy, raw_vz = vz;
#if PLAYER_SPEED_DIV > 1
        {
            static int8_t frac_x, frac_y, frac_z;
            int8_t eff;

            frac_x += raw_vx;
            eff = frac_x / PLAYER_SPEED_DIV;
            frac_x -= eff * PLAYER_SPEED_DIV;
            vx = eff;

            frac_y += raw_vy;
            eff = frac_y / PLAYER_DEPTH_DIV;
            frac_y -= eff * PLAYER_DEPTH_DIV;
            vy = eff;

            frac_z += raw_vz;
            eff = frac_z / PLAYER_SPEED_DIV;
            frac_z -= eff * PLAYER_SPEED_DIV;
            vz = eff;
        }
#endif

        /* --- Bubblefield inertia: snap while key held, coast when released --- */
        if (ko | kp) { sv_x = raw_vx; dt_x = BUBBLE_DRIFT_FRAMES; }
        else if (sv_x != 0 && --dt_x == 0) {
            dt_x = BUBBLE_DRIFT_FRAMES;
            if (sv_x > 0) sv_x--; else sv_x++;
        }

        if (kq | ka) { sv_y = raw_vy; dt_y = BUBBLE_DRIFT_FRAMES; }
        else if (sv_y != 0 && --dt_y == 0) {
            dt_y = BUBBLE_DRIFT_FRAMES;
            if (sv_y > 0) sv_y--; else sv_y++;
        }

        if (kw | ks) { sv_z = raw_vz; dt_z = BUBBLE_DRIFT_FRAMES; }
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

        if (edge_y && !was_edge_y) { sv_y = -sv_y; bubble_vy = sv_y; }
        else if (edge_y) { sv_y = 0; bubble_vy = 0; }
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

    /* --- Surfacing: level complete if relics done, game over if not --- */
    if (player.gy == 0 && player.at_bound_y) {
        if (level.arch_collected >= level.arch_count) {
            game_over_flag = 0;
            return STATE_SUMMARY;
        } else {
            game_over_flag = 1;
            return STATE_GAMEOVER;
        }
    }

    /* --- Predator collision check --- */
    damage = predators_check_collision();
    if (damage == 255) {
        game_over_flag = 1;
        return STATE_GAMEOVER;
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

    /* --- Distance-based sonar ping --- */
    sonar_update(treasure_nearest_distance());
    beep_tick();

    return STATE_GAME;
}

/* ------------------------------------------------------------------ */
/* STATE_GAMEOVER — colour-cycle death animation                       */
/* ------------------------------------------------------------------ */

/* 8-step colour cycle: bright white → yellow → cyan → green →
 * magenta → red → blue → black.  ~6 frames per step = 48 frames. */
static const uint8_t death_attrs[8] = {
    0x7F, 0x76, 0x6D, 0x64, 0x5B, 0x52, 0x49, 0x00
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

    if (game_over_flag) {
        print_at(11, 3, "Game Over");
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
    print_at(2, 10, "Treasures found: ");
    print_num(19, 10, level.collected_count);
    print_at(21, 10, "/");
    print_num(22, 10, level.treasure_count);

    print_at(2, 12, "Relics recovered: ");
    print_num(20, 12, level.arch_collected);
    print_at(22, 12, "/");
    print_num(23, 12, level.arch_count);

    print_at(2, 14, "Total treasures: ");
    print_num(19, 14, level.total_collected);

    if (game_over_flag) {
        print_at(6, 18, "Press any key");
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
        if (game_over_flag)
            return STATE_TITLE;
        /* Advance to next level */
        current_level++;
        return STATE_INTRO;
    }
    return STATE_SUMMARY;
}

/* ------------------------------------------------------------------ */
/* Dispatch tables                                                     */
/* ------------------------------------------------------------------ */
static const init_fn inits[STATE_COUNT] = {
    state_title_init,       /* STATE_TITLE    */
    state_intro_init,       /* STATE_INTRO    */
    state_game_init,        /* STATE_GAME     */
    state_summary_init,     /* STATE_SUMMARY  */
    state_gameover_init     /* STATE_GAMEOVER */
};

static const tick_fn ticks[STATE_COUNT] = {
    state_title_tick,       /* STATE_TITLE    */
    state_intro_tick,       /* STATE_INTRO    */
    state_game_tick,        /* STATE_GAME     */
    state_summary_tick,     /* STATE_SUMMARY  */
    state_gameover_tick     /* STATE_GAMEOVER */
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
