#pragma once
#include "../krecdoom/maps.h"

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
extern const TileType maps_map3[MAP_ROWS][MAP_COLS];
extern const TileType maps_map4[MAP_ROWS][MAP_COLS];
extern const TileType maps_map5[MAP_ROWS][MAP_COLS];
extern const TileType maps_map6[MAP_ROWS][MAP_COLS];
extern const TileType maps_map7[MAP_ROWS][MAP_COLS];
extern const TileType auto_safe[MAP_ROWS][MAP_COLS];

extern const Door_Side maps_doors1[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors2[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors3[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors4[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors5[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors6[MAP_ROWS][MAP_COLS];
extern const Door_Side maps_doors7[MAP_ROWS][MAP_COLS];
extern const Door_Side auto_safe_doors[MAP_ROWS][MAP_COLS];

#define FILE_NUM 8

static const TileType (*FILES[FILE_NUM])[MAP_COLS] = {
	auto_safe, maps_map1, maps_map2, maps_map3, maps_map4, maps_map5, maps_map6, maps_map7
};

static const Door_Side (*DOOR_FILES[FILE_NUM])[MAP_COLS] = {
	auto_safe_doors, maps_doors1, maps_doors2, maps_doors3, maps_doors4, maps_doors5, maps_doors6, maps_doors7
};

static const char *FILES_NAME[FILE_NUM] = {
	"auto_safe", "maps_map1", "maps_map2", "maps_map3", "maps_map4", "maps_map5", "maps_map6", "maps_map7"
};

static const char *FILES_PATH[FILE_NUM] = {
	"safe", "map1", "map2", "map3", "map4", "map5", "map6", "map7"
};
