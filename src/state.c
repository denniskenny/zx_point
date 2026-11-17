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
#include "../include/music.h"
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
#include "../include/title_logo.h"
#include "../include/dzx0.h"
#include "../include/goo_data.h"
#include "../include/entity_render.h"
#include "../include/sprites_packed.h"
#include "../include/bubblefield.h"
#include "../include/grid.h"

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
static uint8_t dt_x, dt_z;
static uint8_t was_edge_x, was_edge_z;

/* --- Draw state: computed in logic phase, consumed next draw phase --- */
static int8_t bubble_vx, bubble_vy, bubble_vz;

/* --- Persistent state across game states --- */
static uint8_t current_level;
/* How the last dive ended (drives the summary / game-over screens) */
#define OUTCOME_COMPLETE  0   /* surfaced with all relics */
#define OUTCOME_DIED      1   /* killed or out of oxygen/health */
#define OUTCOME_SURFACED  2   /* surfaced early: relics still missing */
static uint8_t game_over_flag;
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
    game_over_flag = OUTCOME_COMPLETE;

    screen_clear(0x07, 0);   /* white ink, black paper, black border */

    /* Title logo banner: ZX0-compressed, screen-layout pixels decompress
     * straight to the top 112 rows; its attribute cells go to char rows 0-13. */
    dzx0_decompress(title_logo_zx0, SCREEN);
    dzx0_decompress(title_logo_attr_zx0, ATTR);

    print_at(6, 17, "Press any key");
    print_at(3, 21, "Copyright Actual Size 2026");

    key_debounce = 1;
    return STATE_TITLE;
}

static game_state_t state_title_tick(void)
{
    /* Wait for any key held on entry (e.g. from the summary screen) to
     * release, so the 3-voice shanty isn't skipped the instant it starts. */
    if (key_debounce) {
        vsync_wait();
        if (!any_key_pressed()) key_debounce = 0;
        return STATE_TITLE;
    }

    /* Blocking 3-channel Tritone shanty; returns on any key/joystick.
     * tritone_ticks (rows played) is our keypress-timing entropy source. */
    oro_play();
    treasure_seed((uint16_t)tritone_ticks * 251 + 0xA3B7);
    return STATE_INTRO;
}

/* ------------------------------------------------------------------ */
/* STATE_INTRO — level briefing screen                                 */
/* ------------------------------------------------------------------ */
/* Per-level contents from config/game_config.h (see LEVEL_* tables) */
static const uint8_t lvl_treasures[] = LEVEL_TREASURES;
static const uint8_t lvl_rays[]      = LEVEL_RAYS;
static const uint8_t lvl_sharks[]    = LEVEL_SHARKS;
static const uint8_t lvl_goos[]      = LEVEL_GOOS;
typedef char lvl_table_len_check[
    (sizeof(lvl_treasures) == LEVEL_TABLE_LEN && sizeof(lvl_rays) == LEVEL_TABLE_LEN &&
     sizeof(lvl_sharks) == LEVEL_TABLE_LEN && sizeof(lvl_goos) == LEVEL_TABLE_LEN) ? 1 : -1];

/* Count for `level` (1-based): table entry, then +extra per level beyond */
static uint8_t level_count(const uint8_t *table, uint8_t extra, uint8_t lvl)
{
    uint16_t n;
    if (lvl <= LEVEL_TABLE_LEN) return table[lvl - 1];
    n = table[LEVEL_TABLE_LEN - 1] + (uint16_t)extra * (lvl - LEVEL_TABLE_LEN);
    return n > 255 ? 255 : (uint8_t)n;
}

static game_state_t state_intro_init(void)
{
    const char *name;
    uint8_t n_rays   = level_count(lvl_rays,   LEVEL_EXTRA_RAYS,   current_level);
    uint8_t n_sharks = level_count(lvl_sharks, LEVEL_EXTRA_SHARKS, current_level);
#ifdef TEST_MANY_PREDS
    n_rays   = TEST_RAYS;
    n_sharks = TEST_SHARKS;
#endif

    /* Spawn treasures and predators so counts are available */
    treasure_spawn(current_level,
                   level_count(lvl_treasures, LEVEL_EXTRA_TREASURES, current_level));
    predators_spawn(n_rays, n_sharks,
                    level_count(lvl_goos, LEVEL_EXTRA_GOOS, current_level));

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
    vsync_wait();    /* briefing is silent; the shanty played on the title */
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
#define LINTRO_BOAT_WL      28
#define LINTRO_BOAT_W       (BOAT_WIDTH / 8)   /* bytes, from boat.zxp */
#define LINTRO_BOAT_H       BOAT_HEIGHT
#define LINTRO_BOB_FRAMES   50    /* 1 second at 50 fps */
#define LINTRO_DIVER_X      (DIVER_X >> 3)  /* col 15 */
#define LINTRO_BOAT_ATTR    0x2A  /* red ink, cyan paper */
#define LINTRO_BG_ATTR      0x29  /* depth 1: blue ink, cyan paper */
#define LINTRO_ATTR_ROW     3     /* topmost char row the boat can touch */
#define LINTRO_ATTR_H       7     /* char rows 4-10 cover full bob range */

#define LINTRO_SCROLL        0
#define LINTRO_BOB           1
#define LINTRO_DESCEND       2
#define LINTRO_HOLD          3
#define LINTRO_DESCEND_SPEED 3     /* frames per pixel of descent */
#define LINTRO_HOLD_FRAMES   25    /* 0.5 seconds at 50 fps */


static int16_t lintro_px;
static uint8_t lintro_phase;
static uint8_t lintro_timer;
static uint8_t lintro_sea_phase;   /* sine wave phase (boat bob + sea line) */
static uint8_t lintro_wave_ctr;    /* ticks since the last wave step */
static uint8_t lintro_ticks;       /* free-running tick count (diver animation) */
static uint8_t lintro_drawn;
static int16_t lintro_prev_px;
static uint8_t lintro_prev_y;
static uint8_t lintro_diver_y;      /* diver descent target Y */
static uint8_t lintro_diver_drawn;  /* diver XOR currently on screen */
static uint8_t lintro_diver_draw_y; /* Y where XOR is on screen */
static uint8_t lintro_diver_frame;  /* frame index currently XOR'd */
static uint8_t lintro_saved_vsync; /* saved vsync_mode for restore */
static uint8_t lintro_sl_prev[32]; /* sea line prev Y per column */

/* The intro sea line only ever spans LINTRO_SEA_Y +-6 (13 pixel rows), so
 * cache those rows' screen addresses once instead of calling scr_off()
 * twice per column per frame. */
#define LINTRO_SL_TOP   (LINTRO_SEA_Y - 6)
#define LINTRO_SL_ROWS  13
static uint8_t *lintro_sl_row[LINTRO_SL_ROWS];

/* Precached shifted copies of the boat: (w+1) bytes/row, carry column at
 * right, one per sub-byte shift 1..7 (shift 0 uses boat_bitmap directly).
 * All seven are built when the intro starts, so any LINTRO_SCROLL_PX works. */
#define LINTRO_BOAT_SW  (LINTRO_BOAT_W + 1)
/* Lives in the GOO-death scratch buffer (0x6000, transient): the boat
 * intro and GOO death never overlap, and BSS has no room left under
 * $FF58. Rebuilt every intro since GOO death overwrites it. */
#define LINTRO_BOAT_SX_SIZE  (LINTRO_BOAT_SW * LINTRO_BOAT_H)
#define LINTRO_BOAT_SX(si)  ((uint8_t *)0x6000 + (uint16_t)(si) * LINTRO_BOAT_SX_SIZE)
typedef char lintro_boat_sx_fits[(7u * LINTRO_BOAT_SX_SIZE <= 0x0900u) ? 1 : -1];
static uint8_t lintro_boat_have = 0;   /* bitmask of built variants */

static const uint8_t *lintro_boat_data(uint8_t shift)
{
    uint8_t si, row, c;
    uint8_t *dst;

    if (!shift) return boat_bitmap;

    si = shift - 1;                    /* shifts 1..7 -> slots 0..6 */
    if (lintro_boat_have & (1 << si)) return LINTRO_BOAT_SX(si);

    dst = LINTRO_BOAT_SX(si);
    for (row = 0; row < LINTRO_BOAT_H; row++) {
        const uint8_t *src = boat_bitmap + (uint16_t)row * LINTRO_BOAT_W;
        uint8_t carry = 0;
        for (c = 0; c < LINTRO_BOAT_SW; c++) {
            uint8_t b = (c < LINTRO_BOAT_W) ? src[c] : 0;
            dst[c] = (b >> shift) | carry;
            carry = (uint8_t)(b << (8 - shift));
        }
        dst += LINTRO_BOAT_SW;
    }
    lintro_boat_have |= (uint8_t)(1 << si);
    return LINTRO_BOAT_SX(si);
}

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
    return (uint8_t)(LINTRO_SEA_Y - LINTRO_BOAT_WL + sine_tab[idx]);
}

/* XOR sea line: combined erase-old + draw-new, no attrs */
static void lintro_sealine_tick(void)
{
    uint8_t col, sy, prev, lo, hi;
    int16_t bcol = lintro_px >> 3;
    int16_t bcol_end = bcol + ((lintro_px & 7) ? LINTRO_BOAT_W + 1 : LINTRO_BOAT_W);

    /* Columns [lo, hi) are under the boat: no sea line drawn there */
    lo = (bcol < 0) ? 0 : (bcol > 32) ? 32 : (uint8_t)bcol;
    hi = (bcol_end < 0) ? 0 : (bcol_end > 32) ? 32 : (uint8_t)bcol_end;

    for (col = 0; col < 32; col++) {
        prev = lintro_sl_prev[col];
        if (col >= lo && col < hi) {
            lintro_sl_prev[col] = 255;
            continue;
        }

        /* row index 0..12 (LINTRO_SEA_Y + sine_tab, always on screen) */
        sy = (uint8_t)(6 + sine_tab[(col + lintro_sea_phase) & 31]);
        if (prev == sy) continue;                       /* unchanged */

        if (prev != 255) lintro_sl_row[prev][col] ^= 0xFF;
        lintro_sl_row[sy][col] ^= 0xFF;
        lintro_sl_prev[col] = sy;
    }
}

static game_state_t state_level_intro_init(void)
{
    uint8_t i;


    lintro_saved_vsync = vsync_mode;
    vsync_mode = 0;

    screen_clear(LINTRO_BG_ATTR, 1);

    lintro_px = -(int16_t)(LINTRO_BOAT_W * 8);
    lintro_boat_have = 0;               /* scratch may hold GOO frames */
    for (i = 1; i < 8; i++)             /* prebuild all shifted variants */
        lintro_boat_data(i);
    lintro_phase = LINTRO_SCROLL;
    lintro_timer = 0;
    lintro_sea_phase = 0;
    lintro_wave_ctr = 0;
    lintro_ticks = 0;
    lintro_drawn = 0;
    lintro_diver_drawn = 0;
    for (i = 0; i < 32; i++) lintro_sl_prev[i] = 255;
    for (i = 0; i < LINTRO_SL_ROWS; i++)
        lintro_sl_row[i] = SCREEN + scr_off(0, LINTRO_SL_TOP + i);
    key_debounce = 1;

    return STATE_LEVEL_INTRO;
}

/* Advance the intro's phase state machine. Returns 1 when the intro is
 * over (finished, or skipped with a key) and the game should start. */
static uint8_t lintro_advance(void)
{
    if (key_debounce) {
        if (!any_key_pressed()) key_debounce = 0;
    } else if (any_key_pressed()) {
        return 1;
    }

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
        if (++lintro_timer >= LINTRO_HOLD_FRAMES) return 1;
        break;
    }
    return 0;
}

/* Clear only the strips the boat has just uncovered: the char column it
 * left on the left, and the rows above/below after a bob. */
static void lintro_erase_delta(int8_t old_col, int8_t new_col, uint8_t boat_y)
{
    if (new_col != old_col && old_col >= 0) {
        clear_blit(old_col, lintro_prev_y, 1, LINTRO_BOAT_H);
        lintro_sl_prev[(uint8_t)old_col] = 255;
    }
    if (lintro_prev_y != boat_y) {
        uint8_t prev_w = (lintro_prev_px & 7) ? LINTRO_BOAT_W + 1 : LINTRO_BOAT_W;
        int8_t prev_col = (int8_t)(lintro_prev_px >> 3);
        if (lintro_prev_y < boat_y)
            clear_blit(prev_col, lintro_prev_y, prev_w, boat_y - lintro_prev_y);
        else
            clear_blit(prev_col, (uint8_t)(boat_y + LINTRO_BOAT_H),
                       prev_w, lintro_prev_y - boat_y);
    }
}

/* Blit the boat and keep its colour block in step with its position */
static void lintro_draw_boat(int8_t old_col, int8_t new_col, uint8_t boat_y)
{
    uint8_t shift = (uint8_t)(lintro_px & 7);

    write_blit(new_col, boat_y, lintro_boat_data(shift),
               shift ? LINTRO_BOAT_SW : LINTRO_BOAT_W, LINTRO_BOAT_H);

    /* Column the boat left: back to background colour */
    if (new_col != old_col && old_col >= 0)
        set_attr_rect((uint8_t)old_col, LINTRO_ATTR_ROW,
                      1, LINTRO_ATTR_H, LINTRO_BG_ATTR);

    /* The boat moves right by at most one char column per 4 ticks, so after
     * the first full paint only the column entering on the right needs the
     * boat colour. Bobbing doesn't change the (full bob range) attr rect. */
    if (!lintro_drawn) {
        lintro_set_attr(lintro_px, LINTRO_BOAT_ATTR);
    } else if (lintro_px != lintro_prev_px) {
        int16_t r_new = (lintro_px + LINTRO_BOAT_W * 8 - 1) >> 3;
        int16_t r_old = (lintro_prev_px + LINTRO_BOAT_W * 8 - 1) >> 3;
        if (r_new != r_old && r_new >= 0 && r_new < 32)
            set_attr_rect((uint8_t)r_new, LINTRO_ATTR_ROW, 1, LINTRO_ATTR_H,
                          LINTRO_BOAT_ATTR);
    }
}

/* Diver dropping from the boat (DESCEND/HOLD phases). XOR erase + draw
 * back-to-back, and only when its Y or animation frame changes. */
static void lintro_draw_diver(uint8_t boat_y)
{
    uint8_t cur_frame;

    if (lintro_phase == LINTRO_DESCEND && lintro_timer == 0) {
        lintro_diver_y = boat_y + LINTRO_BOAT_H;   /* first DESCEND frame */
        lintro_timer = 1;
    }
    if (lintro_phase != LINTRO_DESCEND && lintro_phase != LINTRO_HOLD)
        return;

    cur_frame = (lintro_ticks >> 4) % diver_frame_count;
    xor16_x = DIVER_X;
    xor16_attr = LINTRO_BG_ATTR;

    if (lintro_diver_drawn &&
        (lintro_diver_draw_y != lintro_diver_y || lintro_diver_frame != cur_frame)) {
        xor16_y = lintro_diver_draw_y;
        xor16_spr = sprites_get_frame(lintro_diver_frame);
        xor_sprite_16();
        lintro_diver_drawn = 0;
    }

    if (!lintro_diver_drawn) {
        xor16_y = lintro_diver_y;
        xor16_spr = sprites_get_frame(cur_frame);
        xor_sprite_16();
        lintro_diver_drawn = 1;
        lintro_diver_draw_y = lintro_diver_y;
        lintro_diver_frame = cur_frame;
    }

    sprites_diver_set_colour_at(LINTRO_DIVER_X, lintro_diver_draw_y >> 3, 3, 0x28);
}

static game_state_t state_level_intro_tick(void)
{
    uint8_t boat_y;
    int8_t old_col, new_col;

    vsync_wait();

    old_col = (int8_t)(lintro_px >> 3);
    if (lintro_advance()) {
        vsync_mode = lintro_saved_vsync;
        return STATE_GAME;
    }
    new_col = (int8_t)(lintro_px >> 3);
    boat_y = lintro_boat_y();

    if (lintro_drawn) lintro_erase_delta(old_col, new_col, boat_y);
    lintro_sealine_tick();
    lintro_draw_boat(old_col, new_col, boat_y);
    lintro_draw_diver(boat_y);

    lintro_drawn = 1;
    lintro_prev_px = lintro_px;
    lintro_prev_y = boat_y;
    lintro_ticks++;
    if (++lintro_wave_ctr >= LINTRO_WAVE_FRAMES) {
        lintro_wave_ctr = 0;
        lintro_sea_phase++;
    }

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
    dt_x = 0; dt_z = 0;
    was_edge_x = 0; was_edge_z = 0;
    bubble_vx = 0; bubble_vy = 0; bubble_vz = 0;
    frame = 0;
    oxygen_drain_ctr = OXYGEN_DRAIN_RATE;

    if (game_over_flag == OUTCOME_SURFACED) {
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

/* ------------------------------------------------------------------ */
/* STATE_GAME helpers                                                  */
/* ------------------------------------------------------------------ */

/* Accelerate one horizontal axis: key k1 pushes in direction d1 (+1/-1),
 * k2 in the opposite direction, both capped at +-SPEED. With neither
 * pushing, friction steps the speed toward 0 every 8 frames. */
static int8_t axis_accel(int8_t v, uint8_t k1, int8_t d1, uint8_t k2)
{
    if (k1 && (d1 > 0 ? v < SPEED : v > -SPEED)) return v + d1;
    if (k2 && (d1 > 0 ? v > -SPEED : v < SPEED)) return v - d1;
    if ((frame & 7) == 0) {
        if (v > 0) return v - 1;
        if (v < 0) return v + 1;
    }
    return v;
}

#if PLAYER_SPEED_DIV > 1
/* Fractional speed divider: accumulate raw speed and emit up to
 * max_steps whole steps of +-1 each time the accumulator passes div. */
static int8_t frac_x, frac_y, frac_z;

static int8_t frac_step(int8_t *frac, int8_t raw, int8_t div, uint8_t max_steps)
{
    int8_t v = 0;
    *frac += raw;
    while (max_steps--) {
        if (*frac >= div)       { v++; *frac -= div; }
        else if (*frac <= -div) { v--; *frac += div; }
        else break;
    }
    return v;
}
#endif

/* Bubble-field inertia: follow the key-driven speed while held, then
 * coast down by 1 every BUBBLE_DRIFT_FRAMES once released. */
static int8_t bubble_coast(int8_t sv, uint8_t held, int8_t raw, uint8_t *dt)
{
    if (held) { *dt = BUBBLE_DRIFT_FRAMES; return raw; }
    if (sv != 0 && --*dt == 0) {
        *dt = BUBBLE_DRIFT_FRAMES;
        return (sv > 0) ? sv - 1 : sv + 1;
    }
    return sv;
}

/* World edge: reverse the bubble drift on the first frame at the edge,
 * then hold it at 0 while the player stays there. */
static int8_t edge_bounce(int8_t sv, uint8_t edge, uint8_t *was)
{
    if (edge) sv = *was ? 0 : -sv;
    *was = edge;
    return sv;
}

static uint8_t clamp_pred_y(int16_t ey)
{
    if (ey < PRED_Y_MIN) ey = PRED_Y_MIN;
    if (ey > PRED_Y_MAX) ey = PRED_Y_MAX;
    return (uint8_t)ey;
}

/* Draw phase, background: bubbles, sea floor, sea line, and the Y the
 * entities sit at for the current depth. */
static void game_draw_environment(void)
{
    uint8_t trans = depth_is_transitioning();
    uint8_t at_bottom = (player.gy == GRID_D - 1);
    uint8_t at_floor = at_bottom && !trans;
    uint8_t show_sealine;
    uint8_t sfy = 0;
    int16_t sly_i;
    uint16_t total_depth;

    /* Sea line: pinned at world top (gy=0, sub_z=0).
     * Not shown at the bottommost subcube. */
    total_depth = (uint16_t)player.gy * CUBE_SUB_Z + player.sub_z;
    sly_i = SEALINE_DEFAULT_Y -
        (int16_t)(total_depth * SEALINE_DEFAULT_Y / CUBE_SUB_Z);
    show_sealine = (sly_i >= 0 && !trans && !at_bottom);
    sf_cull_y = show_sealine ? (uint8_t)(sly_i + 6) : 0;

    /* Sea floor rises from the bottom of the view as the player sinks
     * through the deepest cube. Bubbles are culled 4 px above it so 4x4
     * close bubbles never overlap the floor. */
    if (at_floor) {
        sfy = VIEW_H - 1 -
            (uint8_t)((uint16_t)player.sub_z * (VIEW_H - 1 - DIVER_Y - 16) / CUBE_SUB_Z);
        sf_cull_y_floor = (sfy > 4) ? sfy - 4 : 1;
    } else {
        sf_cull_y_floor = 0;
    }

    update_and_draw_bubbles(bubble_vx, bubble_vy, bubble_vz);

    if (at_floor) seafloor_update(sfy);
    else          seafloor_erase();

    if (show_sealine) sealine_update((uint8_t)sly_i);
    else              sealine_erase();

    /* Entity Y anchor: top of a 32px sprite at the depth's feature */
    if (player.gy == 0)
        env_entity_y = clamp_pred_y((sly_i >= 0) ? (int16_t)sealine_get_clamp_y() : 20);
    else if (player.gy == 1)
        env_entity_y = DIVER_Y - 8;
    else
        env_entity_y = clamp_pred_y(at_floor ? (int16_t)sfy - 32 : VIEW_H - 40);
}

/* Logic phase, input: player velocity (vx/vy/vz) and the bubble drift
 * used by the NEXT draw phase. */
static void game_update_velocity(void)
{
    uint8_t k = scan_input();
    uint8_t kq = k & INPUT_FWD;
    uint8_t ka = k & INPUT_BACK;
    uint8_t ko = k & INPUT_LEFT;
    uint8_t kp = k & INPUT_RIGHT;
    uint8_t kz = k & INPUT_DESC;
    uint8_t kx = k & INPUT_ASC;
    int8_t raw_vx, raw_vy, raw_vz;

    vx = axis_accel(vx, ko, 1, kp);      /* O = +x, P = -x */
    vz = axis_accel(vz, kq, -1, ka);     /* Q = -z, A = +z */

    /* Vertical (Z = descend, X = ascend) — buoyancy model: diver floats up by default */
    if (kz && vy > -SPEED) vy--;
    else if (kx && vy < SPEED + 1) vy++;
    else if (vy < SPEED) vy++;

    raw_vx = vx; raw_vy = vy; raw_vz = vz;
#if PLAYER_SPEED_DIV > 1
    vx = frac_step(&frac_x, raw_vx, PLAYER_SPEED_DIV, 1);
    vy = frac_step(&frac_y, raw_vy, PLAYER_DEPTH_DIV, 2);
    vz = frac_step(&frac_z, raw_vz, PLAYER_SPEED_DIV, 1);
#endif

    /* Vertical bubbles always track velocity (buoyancy keeps them moving) */
    sv_x = bubble_coast(sv_x, ko | kp, raw_vx, &dt_x);
    sv_y = raw_vy;
    sv_z = bubble_coast(sv_z, kq | ka, raw_vz, &dt_z);
}

/* Logic phase: stop / bounce the bubble drift at the world edges */
static void game_world_edges(void)
{
    uint8_t edge_x = (player.gx == 0 && player.sub_x == 0) ||
                     (player.gx == GRID_W - 1 && player.sub_x >= CUBE_SUB_XY - 1);
    uint8_t edge_y = (player.gy == 0 && player.sub_z == 0) ||
                     (player.gy == GRID_D - 1 && player.sub_z >= CUBE_SUB_Z - 1);
    uint8_t edge_z = (player.gz == 0 && player.sub_y == 0) ||
                     (player.gz == GRID_H - 1 && player.sub_y >= CUBE_SUB_XY - 1);

    sv_x = edge_bounce(sv_x, edge_x, &was_edge_x);
    if (edge_y) sv_y = 0;
    sv_z = edge_bounce(sv_z, edge_z, &was_edge_z);
}

/* Logic phase: sonar pings for the closest same-depth object */
static void game_sonar(void)
{
    uint8_t best_dist = 255, best_type = SONAR_TREASURE;
    uint8_t i, d;

    for (i = 0; i < level.treasure_count; i++) {
        if (treasures[i].collected) continue;
        if (treasures[i].gy != player.gy) continue;
        d = grid_dist(player.gx, player.gz, treasures[i].gx, treasures[i].gz);
        if (d < best_dist) { best_dist = d; best_type = SONAR_TREASURE; }
    }

    for (i = 0; i < predator_count; i++) {
        if (!predators[i].active) continue;
        if (predators[i].type != player.gy) continue;   /* type == depth */
        d = grid_dist(player.gx, player.gz, predators[i].gx, predators[i].gz);
        if (d < best_dist) {
            best_dist = d;
            best_type = (predators[i].type == PRED_GOO) ? SONAR_GOO : SONAR_PREDATOR;
        }
    }

    sonar_update(best_dist, best_type);
    beep_tick();
}

static game_state_t state_game_tick(void)
{
    uint8_t damage;

    /* ================================================================
     * Frame structure: draw-then-compute
     *
     * The loop is split into two phases separated by vsync_wait():
     *
     *   1. DRAW PHASE  — runs immediately after vsync, while the
     *      beam is in the top border (~14 336 T-states of safe
     *      window).  All screen writes happen here, racing ahead
     *      of the beam.  Uses bubble_vx/vy/vz and
     *      player/predator/treasure positions computed by the
     *      PREVIOUS tick's logic phase.
     *
     *   2. LOGIC PHASE  — runs while the beam scans the active
     *      display.  No screen writes (except depth-change hide,
     *      which is a rare one-shot).  Reads input, moves entities,
     *      checks collisions, and writes bubble_vx/vy/vz
     *      for the NEXT draw phase.
     *
     * This gives one frame (20 ms) of display latency between input
     * and visible response — imperceptible to the player.
     * ================================================================ */

    /* === DRAW PHASE =============================================== */
    vsync_wait();

    treasure_erase();
    game_draw_environment();

    predators_render(frame, !depth_is_transitioning());
    treasure_draw(frame);
    sprites_player_draw((frame >> 3) % diver_frame_count);
    if (player.invuln_timer > 0 && (player.invuln_timer & 0x02))
        set_attr_rect(DIVER_X >> 3, DIVER_Y >> 3, 2, 2, depth_get_attr(current_depth));
    else
        sprites_player_set_colour(depth_get_paper());

    hud_draw(player.oxygen, player.health);

    minimap_draw();
    depth_indicator_draw();

    /* === LOGIC PHASE ============================================== */
    game_update_velocity();

    /* --- Player movement & cube traversal --- */
    if (player_update(vx, vy, vz)) {
        bubbles_set_count(depth_bubbles[player.gy]);
        predators_hide_all();
        treasure_hide_all();
    }
    game_world_edges();
    bubble_vx = sv_x;
    bubble_vy = sv_y;
    bubble_vz = sv_z;

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
        game_over_flag = (level.arch_collected >= level.arch_count)
                         ? OUTCOME_COMPLETE : OUTCOME_SURFACED;
        return STATE_SUMMARY;
    }

    /* --- Predator collision check --- */
    damage = predators_check_collision();
    if (damage == 255) {
        game_over_flag = OUTCOME_DIED;
        return STATE_GOO_DEATH;
    }
    if (damage > 0 && player.invuln_timer == 0) {
        player.health = (player.health > damage) ? player.health - damage : 0;
        player.invuln_timer = INVULNERABLE_FRAMES;
        sfx_damage_jingle();
    }

    /* --- Check game over conditions --- */
    if (player.health == 0 || player.oxygen == 0) {
        game_over_flag = OUTCOME_DIED;
        return STATE_GAMEOVER;
    }

    /* --- Oxygen drain --- */
    if (player.oxygen > 0 && --oxygen_drain_ctr == 0) {
        oxygen_drain_ctr = OXYGEN_DRAIN_RATE;
        player.oxygen--;
    }

    frame++;

    game_sonar();

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

    if (game_over_flag == OUTCOME_DIED) {
        print_at(11, 3, "Game Over");
    } else if (game_over_flag == OUTCOME_SURFACED) {
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

    if (game_over_flag == OUTCOME_DIED) {
        print_at(6, 18, "Press any key");
    } else if (game_over_flag == OUTCOME_SURFACED) {
        print_at(3, 18, "Press any key to");
        print_at(7, 19, "dive again");
    } else {
        print_at(4, 18, "Press any key for");
        print_at(8, 19, "next level");
    }

    key_debounce = 1;

    if (game_over_flag == OUTCOME_COMPLETE)
        music_play_blocking(music_fanfare, music_fanfare_len);
    /* The 3-voice Tritone tune (Spanish dirge for game over, Lowlands shanty
     * for level-complete) is played blocking in the tick; nothing to start. */
    return STATE_SUMMARY;
}

static game_state_t state_summary_tick(void)
{
    /* Wait for any key held on entry to release before the blocking player. */
    if (key_debounce) {
        vsync_wait();
        if (!any_key_pressed()) key_debounce = 0;
        return STATE_SUMMARY;
    }

    /* Game over: blocking 3-channel Tritone "Spanish Ladies" dirge. */
    if (game_over_flag == OUTCOME_DIED) {
        spanish_play();                 /* loops the dirge until any key */
        return STATE_TITLE;
    }

    /* Level complete: blocking 3-channel Tritone "Lowlands Away" shanty. */
    lowlands_play();                    /* loops the shanty until any key */
    if (game_over_flag == OUTCOME_SURFACED)
        return STATE_LEVEL_INTRO;
    current_level++;
    return STATE_INTRO;
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
    sprites_player_set_colour(depth_get_paper());

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
        sprites_player_set_colour(depth_get_paper());

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
