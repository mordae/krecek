#pragma once
#include <stdint.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

typedef enum TileEffect {
	TILE_EFFECT_NONE = 0,
	TILE_EFFECT_TELEPORT = 1,
	TILE_EFFECT_DAMAGE = 2,
	NUM_TILE_EFFECTS,
} TileEffect;

typedef struct __packed Tile {
	uint8_t tile_id;

	union __packed {
		struct __packed {
			uint16_t map : 7;
			uint16_t px : 5;
			uint16_t py : 4;
		};
		struct __packed {
			uint8_t _unused : 8;
			uint8_t damage : 8;
		};
	};

	TileEffect effect : 4;
	uint8_t collides_right : 1;
	uint8_t collides_up : 1;
	uint8_t collides_down : 1;
	uint8_t collides_left : 1;
} Tile;

static_assert(sizeof(Tile) == 4);
