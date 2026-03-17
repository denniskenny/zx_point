/*
 * state.c -- State machine dispatcher + STATE_GAME implementation
 *
 * The game init/tick wraps the original starfield.c main() loop body.
 * Stub states redirect to STATE_GAME for Phase 1.
 *
 * Sprites drawn directly to screen RAM (no SP1).
 */

#include <string.h>
#include <stdint.h>
#include "../config/game_config.h"
#include "../include/state.h"
#include "../include/game.h"
#include "../include/gfx.h"
#include "../include/input.h"
#include "../include/sound.h"
#include "../include/depth.h"
#include "../include/vsync.h"
#include "../include/sprites.h"
#include "../include/player.h"
#include "../include/treasure.h"
#include "../include/hud.h"
#include "../include/minimap.h"
#include "../include/predators.h"

/* --- Starfield public API (from src/starfield.c) --- */
extern void init_stars(void);
extern void stars_set_count(uint8_t count);
extern void stars_erase_all(void);
extern void update_and_draw_stars(int8_t vx, int8_t vy, int8_t vz);

/* --- Star counts per depth (0-indexed by gy) --- */
static const uint8_t depth_stars[3] = {
    STARS_DEPTH1, STARS_DEPTH2, STARS_DEPTH3
};

/* --- Inter-frame state for STATE_GAME --- */
static int8_t vx, vy, vz;
static uint8_t frame;
static uint8_t ping_ctr;
static uint8_t oxygen_drain_ctr;

/* --- Function-pointer types --- */
typedef game_state_t (*init_fn)(void);
typedef game_state_t (*tick_fn)(void);

/* ------------------------------------------------------------------ */
/* STATE_GAME                                                          */
/* ------------------------------------------------------------------ */
static game_state_t state_game_init(void)
{
    vx = 0;
    vy = 0;
    vz = -2;   /* gentle forward drift */
    frame = 0;
    ping_ctr = PING_INTERVAL;
    oxygen_drain_ctr = OXYGEN_DRAIN_RATE;

    /* Initialise player at grid centre, full health/oxygen */
    player_init();

    /* Spawn treasures and predators for level 1 */
    treasure_spawn(1);
    predators_spawn(1);

    stars_set_count(STARS_DEPTH1);
    init_stars();

    /* Clear screen and set up depth colours */
    sprites_init();
    depth_set(1);

    /* Reset predator draw tracking */
    predators_init();

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
    uint8_t spr_attr;

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

    /* Kempston joystick */
    joy = read_keys(KEMP_PORT);
    if (joy & 0x08) kq = 1;
    if (joy & 0x04) ka = 1;
    if (joy & 0x02) ko = 1;
    if (joy & 0x01) kp = 1;
    if (joy & 0x10) kw = 1;
    if (joy & 0x20) ks = 1;

    if (kq && vy <  SPEED) vy++;
    if (ka && vy > -SPEED) vy--;
    if (ko && vx <  SPEED) vx++;
    if (kp && vx > -SPEED) vx--;
    if (kw && vz > -SPEED) vz--;
    if (ks && vz <  SPEED) vz++;

    /* --- Player movement & cube traversal --- */
    depth_changed = player_update(vx, vy, vz);
    if (depth_changed) {
        stars_set_count(depth_stars[player.gy]);
        predators_hide_all();
    }

    /* --- Predator AI & movement --- */
    predators_update();

    /* Advance depth colour transition (no-op if not transitioning) */
    depth_transition_tick();

    /* --- Treasure proximity check --- */
    treasure_check_collection();

    /* --- Predator collision check --- */
    damage = predators_check_collision();
    if (damage == 255) {
        /* GOO instant death */
        return STATE_GAMEOVER;
    }
    if (damage > 0 && player.invuln_timer == 0) {
        if (player.health > damage)
            player.health -= damage;
        else
            player.health = 0;
        player.invuln_timer = INVULNERABLE_FRAMES;
        beep_ping();   /* placeholder damage sound */
    }

    /* --- Check game over conditions --- */
    if (player.health == 0 || player.oxygen == 0) {
        return STATE_GAMEOVER;
    }

    /* --- Oxygen drain --- */
    if (player.oxygen > 0) {
        if (--oxygen_drain_ctr == 0) {
            oxygen_drain_ctr = OXYGEN_DRAIN_RATE;
            player.oxygen--;
        }
    }

    /* --- Compute player attribute --- */
    if (player.invuln_timer > 0 && (player.invuln_timer & 0x02)) {
        /* Flash: use depth paper colour only (sprite invisible) */
        spr_attr = depth_get_attr(player.gy + 1);
    } else {
        /* Normal: yellow ink (6) on depth paper */
        spr_attr = (depth_get_attr(player.gy + 1) & 0xF8) | 0x06;
    }

    /* --- Vsync -- floating bus with HALT fallback --- */
    vsync_wait();

    /* --- Draw order: erase+draw predators, stars, player on top --- */
    predators_render();
    update_and_draw_stars(vx, vy, vz);
    sprites_player_draw((frame >> 3) & 1);
    sprites_player_set_colour(spr_attr);

    /* --- HUD (oxygen + health bars) --- */
    hud_draw(player.oxygen, player.health);

    /* --- Minimap (drawn last to overlay play area) --- */
    minimap_draw();

    frame++;

    /* --- Ping beeper --- */
    if (--ping_ctr == 0) {
        ping_ctr = PING_INTERVAL;
        beep_ping();
    }

    return STATE_GAME;
}

/* ------------------------------------------------------------------ */
/* Stub states -- redirect to STATE_GAME for Phase 1                   */
/* ------------------------------------------------------------------ */
static game_state_t state_stub_init(void) { return STATE_GAME; }
static game_state_t state_stub_tick(void) { return STATE_GAME; }

/* ------------------------------------------------------------------ */
/* Dispatch tables                                                     */
/* ------------------------------------------------------------------ */
static const init_fn inits[STATE_COUNT] = {
    state_stub_init,    /* STATE_TITLE    */
    state_stub_init,    /* STATE_INTRO    */
    state_game_init,    /* STATE_GAME     */
    state_stub_init,    /* STATE_SUMMARY  */
    state_stub_init     /* STATE_GAMEOVER */
};

static const tick_fn ticks[STATE_COUNT] = {
    state_stub_tick,    /* STATE_TITLE    */
    state_stub_tick,    /* STATE_INTRO    */
    state_game_tick,    /* STATE_GAME     */
    state_stub_tick,    /* STATE_SUMMARY  */
    state_stub_tick     /* STATE_GAMEOVER */
};

/* ------------------------------------------------------------------ */
/* State machine runner                                                */
/* ------------------------------------------------------------------ */
void state_run(void)
{
    game_state_t cur = STATE_GAME;   /* Phase 1: boot straight to game */
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
