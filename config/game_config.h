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

/* --- Keyboard half-row port addresses --- */
#define KEY_QWERT  0xFBFE   /* Q=bit0  W=bit1 */
#define KEY_ASDFG  0xFDFE   /* A=bit0  S=bit1 */
#define KEY_POIUY  0xDFFE   /* P=bit0  O=bit1 */

/* --- Kempston joystick --- */
#define KEMP_PORT  0x001F   /* active-high: R=0 L=1 D=2 U=3 F1=4 F2=5 */

/* --- Starfield parameters --- */
#define NUM_STARS  100
#define XY_HALF    128      /* x,y range: -128 .. +127  */
#define XY_SPAN    256      /* XY_HALF * 2              */
#define Z_MIN      1
#define Z_MAX      255
#define FOCAL      128      /* perspective focal length  */
#define SPEED      3        /* pixels per frame per key  */

/* --- Diver sprite position --- */
#define DIVER_X  120   /* (256 - 16) / 2, byte-aligned */
#define DIVER_Y  88    /* (192 - 16) / 2               */

/* --- Sonar ping --- */
#define PING_INTERVAL  12   /* frames between pings */

/* --- World grid --- */
#define GRID_W  64
#define GRID_D  3
#define GRID_H  64

/* --- Depth-level star counts --- */
#define STARS_DEPTH1  100
#define STARS_DEPTH2  50
#define STARS_DEPTH3  15

/* --- Distances (in cube units) --- */
#define CUBE_DISTANCE     1    /* placeholder — calibrate to 10s traverse */
#define CONTACT_DISTANCE  2    /* X/Y proximity for treasure pickup */

/* --- Player constants --- */
#define INVULNERABLE_FRAMES  60
#define TIME_LIMIT_FRAMES    (3 * 60 * 50)  /* 3 minutes at 50 fps */
#define HEALTH_MAX  5
#define OXYGEN_MAX  100

/* --- Sub-cube traversal --- */
/* At SPEED=3, 50fps: 10s horizontal = 1500, 20s vertical = 3000 */
#define CUBE_SUB_XY     1500
#define CUBE_SUB_Z      3000

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
#define HUD_ROW  23

/* --- Minimap layout (character coordinates) --- */
#define MINIMAP_ROW  20
#define MINIMAP_COL  28
#define MINIMAP_SIZE 32   /* pixels (4x4 chars) */

/* --- Predator types --- */
#define PRED_RAY    0
#define PRED_SHARK  1
#define PRED_GOO    2

/* --- Predator limits --- */
#define MAX_PREDATORS       64
#define MAX_VISIBLE_PREDS   2

/* --- Predator grid movement (frames between cube changes) --- */
#define PRED_RAY_GRID_INTERVAL    250   /* 5s  */
#define PRED_SHARK_GRID_INTERVAL  300   /* 6s  */
#define PRED_GOO_GRID_INTERVAL    750   /* 15s */

/* --- Predator screen-space bounds (32x32 sprite with margin) --- */
#define PRED_X_MIN  16
#define PRED_X_MAX  207
#define PRED_Y_MIN  16
#define PRED_Y_MAX  143

/* --- Shark pursuit range (grid cubes) --- */
#define PRED_SHARK_PURSUE_RANGE  2

/* --- Sonar ranges / intervals (in cube units & frames) --- */
#define SONAR_RANGE_FAR    10
#define SONAR_RANGE_NEAR   1
#define SONAR_INTERVAL_FAR   100  /* frames (~2s at 50fps) */
#define SONAR_INTERVAL_NEAR  12   /* frames (~0.25s)       */

/* --- Ray constant --- */
#define RAY_CONSTANT  10

#endif /* _GAME_CONFIG_H_ */
