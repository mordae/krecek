#pragma once
#include <stdint.h>

#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

typedef enum TileEffect {
	TILE_EFFECT_NONE = 0,
	TILE_EFFECT_TELEPORT = 1,
	TILE_EFFECT_DAMAGE = 2,
	NUM_TILE_EFFECTS,
} TileEffect;

typedef union Tile {
	uint32_t u32;

	struct {
		uint32_t tile_id : 8;

		uint32_t : 8;
		uint32_t : 8;

		uint32_t effect : 4;
		uint32_t collides_right : 1;
		uint32_t collides_up : 1;
		uint32_t collides_down : 1;
		uint32_t collides_left : 1;
	};

	struct {
		uint32_t : 8;

		uint32_t map : 7;
		uint32_t px : 5;
		uint32_t py : 4;

		uint32_t : 8;
	};

	struct {
		uint32_t : 8;

		uint32_t : 8;
		uint32_t damage : 8;

		uint32_t : 8;
	};
} __attribute__((__packed__)) Tile;

static_assert(sizeof(Tile) == 4);
