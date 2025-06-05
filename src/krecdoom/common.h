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
#define MINIMAP_WIDTH 32
#define MINIMAP_HEIGHT 32
#define MINIMAP_TILE_SIZE (MINIMAP_WIDTH / MAP_COLS)
#define MINIMAP_PADDING 2

#define multiply332 \
	(x, f) rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define RED rgb_to_rgb565(194, 20, 20)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(23, 62, 224)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define COLOR_MINIMAP_EMPTY rgb_to_rgb565(0, 0, 0)

#define COLOR_MINIMAP_LINE GREEN
#define COLOR_CEILING BLUE
#define COLOR_FLOOR RED
#define COLOR_WALL_DARK rgb_to_rgb565(0x69, 0x69, 0x69);
#define COLOR_WALL_BRIGHT rgb_to_rgb565(0xA9, 0xA9, 0xA9);
#define COLOR_MINIMAP_WALL WHITE
#define COLOR_MINIMAP_PLAYER YELLOW

typedef enum {
	EMPTY = 0,
	WALL = 1,
} TileType;

#endif
