#include "level.h"
#include <tileset.png.h>

static uint32_t xorshift_seed;

static uint32_t xorshift(void)
{
	uint32_t x = xorshift_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshift_seed = x;
	return x;
}

static uint32_t chances[TS_TILESET_PNG_COUNT] = {
	UINT32_MAX * 1.0,
	UINT32_MAX * 1.0,
	UINT32_MAX * 0.1,
	UINT32_MAX * 0.1,
};

static uint32_t random_tile_id(void)
{
	for (int i = 0; i < 8; i++) {
		uint32_t roll = xorshift() * 0x9e3779b9u;
		uint32_t tile_id = roll % TS_TILESET_PNG_COUNT;

		if (chances[tile_id] >= roll)
			return tile_id;
	}

	uint32_t roll = xorshift() * 0x9e3779b9u;
	return roll % TS_TILESET_PNG_COUNT;
}

void level_generate(Level *level)
{
	xorshift_seed = level->seed;

	for (int y = 0; y < MAP_SIZE; y++) {
		for (int x = 0; x < MAP_SIZE; x++) {
			level->map[y][x].tile_id = random_tile_id();
		}
	}
}
