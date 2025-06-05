#ifndef COMMON_H
#define COMMON_H
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#define FOV_DEGREES 60.0f
#define FOV_RADIANS (FOV_DEGREES * (float)M_PI / 180.0f)
#define TILE_SIZE 64.0f
#define PROJECTION_PLANE_DISTANCE ((SCREEN_WIDTH / 2.0f) / tanf(FOV_RADIANS / 2.0f))
#define MOVE_SPEED 50.0f
#define ROTATE_SPEED 3.0f
#define MAP_ROWS 8
#define MAP_COLS 8

#define multiply332 \
	(x, f) rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define RED rgb_to_rgb565(194, 20, 20)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(23, 62, 224)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

typedef enum {
	EMPTY = 0,
	Wall = 1,
} TileType;

#endif
