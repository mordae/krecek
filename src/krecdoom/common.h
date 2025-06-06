#ifndef COMMON_H
#include <stdint.h>
#define COMMON_H
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#define FOV_DEGREES 60.0f
#define FOV_RADIANS (FOV_DEGREES * (float)M_PI / 180.0f)
#define TILE_SIZE 64.0f
#define PROJECTION_PLANE_DISTANCE ((SCREEN_WIDTH / 2.0f) / tanf(FOV_RADIANS / 2.0f))
#define MOVE_SPEED 75.0f
#define ROTATE_SPEED 3.0f

#define MAP_ROWS 24
#define MAP_COLS 24

#define MINIMAP_WIDTH 32
#define MINIMAP_HEIGHT 32
#define MINIMAP_TILE_SIZE 4
#define MINIMAP_VIEW_TILES 8
#define MINIMAP_PADDING 2
#define PLAYER_DOT_SIZE 3
#define COLLISION_PENETRATION_FRACTION 0.1f

#define MENU_MAP_WIDTH 80
#define MENU_MAP_HEIGHT 80
#define MENU_MAP_PADDING 20

#define PISTOL_DAMAGE 25
#define SHOTGUN_DAMAGE 40
#define N_GUNS 3

#define PISTOL_MAX_RANGE_TILES 15.0f
#define SHOTGUN_MAX_RANGE_TILES 5.0f
#define BULLET_VISUAL_DURATION_MS 100
#define SHOTGUN_SPREAD_DEGREES 10.0f
#define SHOTGUN_PELLETS 5

static float zBuffer[SCREEN_WIDTH];

#define ENEMY_ATTACK_RANGE_TILES 8.0f
#define ENEMY_ATTACK_COOLDOWN_MS 1000
#define ENEMY_DAMAGE 10
#define ENEMY_VISION_RANGE_TILES 8.0f
#define ENEMY_MOVE_SPEED 50.0f
#define MAX_ENEMIES 2
typedef enum { ENEMY_IDLE, ENEMY_CHASING } EnemyState;

typedef struct {
	float x, y;
	int health;
	bool alive;
	uint32_t last_attack_time;
	EnemyState state;
} Enemy;
#define multiply332(x, f) \
	rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define DRAW_RED rgb_to_rgb565(183, 0, 0)
#define RED rgb_to_rgb565(194, 20, 20)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(23, 62, 224)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define BLACK rgb_to_rgb565(0, 0, 0)

#define COLOR_MINIMAP_EMPTY BLACK
#define COLOR_MINIMAP_LINE GREEN
#define COLOR_CEILING BLUE
#define COLOR_FLOOR RED
#define COLOR_WALL_DARK rgb_to_rgb565(0x69, 0x69, 0x69)
#define COLOR_WALL_BRIGHT rgb_to_rgb565(0xA9, 0xA9, 0xA9)
#define COLOR_MINIMAP_WALL WHITE
#define COLOR_MINIMAP_PLAYER YELLOW

typedef enum {
	EMPTY = 0,
	WALL = 1,
} TileType;

#endif
