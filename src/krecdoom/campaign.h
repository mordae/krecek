#pragma once
#include "include_maps.h"
#include "maps.h"
#include <sdk.h>
#include "common.h"
extern void Map_starter(const TileType map[MAP_ROWS][MAP_COLS]);

static inline void map_starter_caller(void)
{
	if (Maps == maps_map2) {
		Map_starter(maps_map3);
		return;
	}

	if (Maps == maps_map1) {
		Map_starter(maps_map2);
		return;
	}

	if (Maps == maps_map3) {
		Map_starter(maps_map1);
		return;
	}
}
