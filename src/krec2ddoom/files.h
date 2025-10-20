#pragma once
#include "../krecdoom/maps.h"

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
extern const TileType maps_map3[MAP_ROWS][MAP_COLS];
extern const TileType auto_safe[MAP_ROWS][MAP_COLS];

#define FILE_NUM 4

static const TileType (*FILES[FILE_NUM])[MAP_COLS] = { auto_safe, maps_map1, maps_map2, maps_map3 };

static const char *FILES_NAME[FILE_NUM] = { "auto_safe", "maps_map1", "maps_map2", "maps_map3" };

static const char *FILES_PATH[FILE_NUM] = { "safe", "map1", "map2", "map3" };
