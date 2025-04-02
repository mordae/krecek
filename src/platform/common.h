#pragma once

// Screen and tile dimensions
#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 48

typedef enum {
	EMPTY = 0,
	FLOOR_MID = 1,
	FLOOR_L = 2,
	FLOOR_R,
	FLOOR_JUMP_MID,
	FLOOR_JUMP_L,
	FLOOR_JUMP_R,
	FLOOR_WIN_MID,
	FLOOR_WIN_L,
	FLOOR_WIN_R,
	SPAWNER = 10
} TileType;
