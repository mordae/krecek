#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#define FOV_DEGREES 60.0f
#define FOV_RADIANS (FOV_DEGREES * (float)M_PI / 180.0f)
#define TILE_SIZE 64.0f
#define PROJECTION_PLANE_DISTANCE ((SCREEN_WIDTH / 2.0f) / tanf(FOV_RADIANS / 2.0f))
#define MOVE_SPEED 225.0f
#define ROTATE_SPEED 3.0f

#define MAP_ROWS 24
#define MAP_COLS 24

#define PISTOL_DAMAGE 25
#define SHOTGUN_DAMAGE 40
#define N_GUNS 3

#define PISTOL_MAX_RANGE_TILES 15.0f
#define SHOTGUN_MAX_RANGE_TILES 5.0f
#define BULLET_VISUAL_DURATION_MS 100
#define SHOTGUN_SPREAD_DEGREES 10.0f
#define SHOTGUN_PELLETS 5

#define DRAW_RED rgb_to_rgb565(183, 0, 0)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define YELLOW rgb_to_rgb565(255, 255, 0)

typedef enum {
	EMPTY = 0,
	COBLE = 1,
	BUNKER,
	TELEPORT,
} TileType;

#endif
