#pragma once

#define TILE_SIZE 16
#define MAP_ROWS 20
#define MAP_COLS 15
#define MAP_ROWS_2 10
#define MAP_COLS_2 7.5f

#define PLAYER_SPRITES 4
#define PLAYER_WIDTH 6
#define PLAYER_HEIGHT 12

#define WHITE rgb_to_rgb565(255, 255, 255)

typedef enum {
	EMPTY = 0,
	FLOOR = 1,
	FLOOR_1,
	FLOOR_2,
	FLOOR_3,
	FLOOR_4,
	LEFT_WALL,
	RIGHT_WALL,
	NORTH_WALL,
	SOUTH_WALL,
} TileType;
