#pragma once
#include "maps.h"

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
extern const TileType maps_map3[MAP_ROWS][MAP_COLS];
extern const TileType maps_map4[MAP_ROWS][MAP_COLS];

#define FILE_NUM 4
static const TileType (*FILES[FILE_NUM])[MAP_COLS]
	__attribute__((unused)) = { maps_map1, maps_map2, maps_map3, maps_map4 };
