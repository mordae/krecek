#pragma once
#include <stdint.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

#define TILE_SIZE 16
#define MAP_SIZE_BITS 5
#define MAP_SIZE (1 << MAP_SIZE_BITS)
#define MAP_MAX (MAP_SIZE - 1)

#define ROOM_BITS 3
#define ROOM_SIZE (1 << ROOM_BITS)

typedef enum TileType {
	TILE_NONE = 0,
	TILE_FLOOR_UP,
	TILE_FLOOR_DOWN,
	TILE_HEAL,
	TILE_RECHARGE,
	TILE_CHEST,
} TileType;

typedef struct __packed Tile {
	uint8_t tile_id;
	uint8_t _unused1;
	uint8_t _unused2;
	bool solid : 1;
	TileType type : 7;
} Tile;

static_assert(sizeof(Tile) == 4);

typedef struct Level {
	uint32_t seed;
	int floor;
	Tile map[MAP_SIZE][MAP_SIZE];
	uint8_t sx, sy, ex, ey;
} Level;

void level_generate(Level *level);
