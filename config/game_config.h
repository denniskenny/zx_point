#ifndef _GAME_CONFIG_H_
#define _GAME_CONFIG_H_

/* ================================================================== */
/* game_config.h — All configurable constants for ZX Point            */
/* ================================================================== */

#include <stdint.h>

/* --- Screen layout --- */
#define SCREEN   ((uint8_t *)0x4000)
#define ATTR     ((uint8_t *)0x5800)
#define PIX_SIZE 6144
#define ATTR_SZ  768
#define ATTR_GAME_SZ (20 * 32)  /* top 20 char rows — excludes status area */

/* --- Keyboard half-row port addresses --- */
/* Q=Forward(into screen) */
#define KEY_QWERT  0xFBFE   /* Q=bit0 */
/* A=Backward(out of screen) */
#define KEY_ASDFG  0xFDFE   /* A=bit0 */
/* P=Right  O=Left */
#define KEY_POIUY  0xDFFE   /* P=bit0  O=bit1 */
/* Z=Descend */
#define KEY_SHZXCV 0xFEFE   /* CapsShift=bit0  Z=bit1 */

/* --- Kempston joystick --- */
/* R=Right  L=Left  U=Forward  D=Backward  Fire=Descend */
#define KEMP_PORT  0x001F   /* active-high: R=0 L=1 D=2 U=3 Fire=4 */

/* --- Bubblefield parameters --- */
#define XY_HALF    128      /* x,y range: -128 .. +127  */
#define XY_SPAN    256      /* XY_HALF * 2              */
#define Z_MIN      1
#define Z_MAX      255
#define FOCAL      128      /* perspective focal length  */
#define SPEED      3        /* pixels per frame per key  */

/* --- Diver sprite position --- */
#define DIVER_X  120   /* (256 - 16) / 2, byte-aligned */
#define DIVER_Y  88    /* (192 - 16) / 2               */

/* --- World grid --- */
#define GRID_W  32
#define GRID_D  3
#define GRID_H  32

/* --- Depth-level bubble counts --- */
#define BUBBLES_DEPTH1  45
#define BUBBLES_DEPTH2  35
#define BUBBLES_DEPTH3  29
/* Bubble array size = largest depth count (derived, don't edit) */
#define BUBBLES_MAX2(a, b)  ((a) > (b) ? (a) : (b))
#define NUM_BUBBLES  BUBBLES_MAX2(BUBBLES_DEPTH1, BUBBLES_MAX2(BUBBLES_DEPTH2, BUBBLES_DEPTH3))

/* --- Distances (in cube units) --- */
#define CONTACT_DISTANCE  2    /* X/Y proximity for treasure pickup */
#define TREASURE_VISIBLE_RANGE  4  /* grid cells: sprite appears when within range */

/* --- Player constants --- */
#define INVULNERABLE_FRAMES  60
#define TIME_LIMIT_FRAMES    (3 * 60 * 50)  /* 3 minutes at 50 fps */
#define HEALTH_MAX  5
#define OXYGEN_MAX  100

/* --- Sub-cube traversal --- */
/* Sub-steps per grid cell (do NOT change to tune speed).           */
#define CUBE_SUB_XY     16
#define CUBE_SUB_Z      300

/* --- Speed divisors --- */
/* Effective speed = SPEED / divisor.                               */
/* Higher values = slower.  1 = full speed, 4 = quarter speed.     */
#define PLAYER_SPEED_DIV    6
#define PLAYER_DEPTH_DIV    3   /* depth ascend/descend (~30% faster than XY) */
#define BUBBLE_DRIFT_FRAMES 64  /* frames per velocity step when coasting to stop */

/* --- Player start position (centre of grid) --- */
#define START_GX  16
#define START_GZ  16

/* --- Treasure types --- */
#define TREASURE_STATUE     0
#define TREASURE_TABLET     1
#define TREASURE_ALTAR      2
#define TREASURE_FIRSTAID   3
#define TREASURE_OXYGEN     4
#define TREASURE_MAP        5
#define TREASURE_LOG        6
#define TREASURE_TYPE_COUNT 7
#define TREASURE_ARCH_FIRST 0
#define TREASURE_ARCH_LAST  2
#define TREASURE_FLOT_FIRST 3
#define TREASURE_FLOT_LAST  6
#define MAX_TREASURES       16

/* --- Oxygen drain --- */
#define OXYGEN_DRAIN_RATE  (TIME_LIMIT_FRAMES / OXYGEN_MAX)  /* 90 frames */

/* --- HUD layout (character coordinates) --- */
#define HUD_ROW  21

/* --- Minimap layout (character coordinates) --- */
#define MINIMAP_ROW  20
#define MINIMAP_COL  28
#define MINIMAP_SIZE 32   /* pixels (4x4 chars) */

/* --- Bubblefield viewport (pixels) --- */
/* The play area is the top 160 rows (rows 0-19 in char coords).        */
/* The bottom 32 rows (char rows 20-23) are reserved for the minimap    */
/* and HUD — the bubblefield engine never renders below VIEW_H.         */
#define VIEW_W       256
#define VIEW_H       (MINIMAP_ROW * 8)   /* 160 */

/* Horizontal bubble clipping — ~20% inset on each side */
#define BF_X_MIN     50
#define BF_X_MAX     206   /* exclusive: sx < 206 to pass */

/* Depth LOD cull — bubbles at z >= this skip projection entirely */
#define BF_Z_CULL    160

/* --- Predator types --- */
#define PRED_RAY    0
#define PRED_SHARK  1
#define PRED_GOO    2

/* --- Predator limits --- */
#define MAX_PREDATORS       64
/* Max sprites drawn at once per entity type (predators, treasures).  */
/* Each slot costs draw time; 8 x 32x32 XOR sprites is ~1/3 of a frame. */
#define MAX_VISIBLE_ENTITIES  8

/* --- Predator grid movement (frames between cube changes) --- */
#define PRED_RAY_GRID_INTERVAL    250   /* 5s  */
#define PRED_SHARK_GRID_INTERVAL  300   /* 6s  */
#define PRED_GOO_GRID_INTERVAL    750   /* 15s */

/* --- Predator screen-space bounds (32x32 sprite with margin) --- */
#define PRED_X_MIN  16
#define PRED_X_MAX  207
#define PRED_Y_MIN  16
#define PRED_Y_MAX  143

/* --- Predator facing --- */
/* Source art faces right. A predator right of screen centre is drawn  */
/* mirrored so it always faces the diver. It only flips once its centre */
/* is more than this many pixels past the centre, so it doesn't flicker */
/* while hovering on the line.                                          */
#define PRED_FACE_HYSTERESIS  4

/* --- Predator visibility range (grid cubes, same as treasure) --- */
#define PRED_VISIBLE_RANGE  4

/* --- Predator proximity damage (frames in contact before 1 HP lost) --- */
#define PRED_PROXIMITY_FRAMES  50   /* 1 second at 50 fps */

/* --- Shark pursuit range (grid cubes) --- */
#define PRED_SHARK_PURSUE_RANGE  2

/* --- Sonar ranges / intervals (in cube units & frames) --- */
#define SONAR_RANGE_FAR    10   /* must match sonar_intervals[] length in sound.c */

/* ================================================================== */
/* Level contents                                                      */
/* ================================================================== */
/* Entry N is level N+1. Levels beyond LEVEL_TABLE_LEN take the last   */
/* entry plus LEVEL_EXTRA_* for each further level. Counts are capped  */
/* at MAX_TREASURES / MAX_PREDATORS.                                   */
/* Rays live at depth 1, sharks at depth 2, GOOs at depth 3.          */
#define LEVEL_TABLE_LEN    5
/*                         L1  L2  L3  L4  L5 */
#define LEVEL_TREASURES  {  1,  2,  3,  4,  5 }
#define LEVEL_RAYS       {  1,  2,  3,  4,  5 }
#define LEVEL_SHARKS     {  3,  0,  1,  2,  3 }
#define LEVEL_GOOS       {  1,  1,  1,  1,  1 }  /* L1-4 = 1 for GOO testing; design says 0 */

#define LEVEL_EXTRA_TREASURES  1
#define LEVEL_EXTRA_RAYS       1
#define LEVEL_EXTRA_SHARKS     1
#define LEVEL_EXTRA_GOOS       1

/* --- Predator stress-test build (make test-preds) --- */
/* -DTEST_MANY_PREDS replaces the ray/shark counts on every level.    */
#ifndef TEST_RAYS
#define TEST_RAYS    40
#endif
#ifndef TEST_SHARKS
#define TEST_SHARKS  23
#endif

#endif /* _GAME_CONFIG_H_ */
