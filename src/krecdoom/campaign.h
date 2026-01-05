#pragma once
#include "include_maps.h"
#include "maps.h"
#include <sdk.h>
#include "common.h"
void Map_starter(const TileType (*next_map)[MAP_ROWS][MAP_COLS]);

static inline void map_starter_caller(void)
{
	if (map.map_id == &maps_map2) {
		Map_starter(&maps_map3);
		return;
	}
	if (map.map_id == &maps_map1) {
		Map_starter(&maps_map2);
		return;
	}
	if (map.map_id == &maps_map3) {
		Map_starter(&maps_map1);
		return;
	}
}
