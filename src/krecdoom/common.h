#pragma once

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

typedef enum {
	EMPTY = 0,
	WALL = 1,
} TileType;

typedef struct {
	float x, y;
	float angle;
	float dx, dy;
	float nx, ny;
	float fov;
	float ray_angle;
	float ray_x, ray_y;
	float distance;
} Player;

static Player player;

extern TileType maps_map1[MAP_HEIGHT][MAP_WIDTH];
extern TileType maps_map2[MAP_HEIGHT][MAP_WIDTH];
#ifndef COMMON_H
#define COMMON_H

extern TileType (*map)[MAP_WIDTH];
#endif
