#pragma once
#include "include_maps.h"
#include "maps.h"
#include <sdk.h>
#include "common.h"
void Map_starter(const TileType (*next_map)[MAP_ROWS][MAP_COLS], const Door_Side (*door_map)[MAP_ROWS][MAP_COLS]);

static inline void map_starter_caller(void)
{
	if (level.map_id == &maps_map2) {
		Map_starter(&maps_map3, &maps_doors3);
		return;
	}
	if (level.map_id == &maps_map1) {
		Map_starter(&maps_map2, &maps_doors2);
		return;
	}
	if (level.map_id == &maps_map3) {
		Map_starter(&maps_map1, &maps_doors1);
		return;
	}
}
