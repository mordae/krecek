#pragma once
#define MAP_COLS 30
#define MAP_ROWS 30
#define ENUM_TYPES 22

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	EMPTY = 0,
	COBLE = 1,
	BUNKER,
	BRICKS,
	HEALTH_PACK, // UN1
	STONE,
	MAGMA,
	WATER,
	STONE_HOLE,
	CHEKERD,
	AMMO_BOX,	// (was UN2)
	SHOTGUN_PICKUP, // (was UN3)
	ENEMY1,		// UN 4
	MOSS_STONE,
	WOOD_WALL,
	PRISMARYN,
	CHOLOTATE,
	ENEMY2, // UN 5
	PATTERN_IN_BROWN,
	SAND_WITH_MOSS,
	IRON,
	TELEPORT = 21,
	PLAYER_SPAWN,
	DOOR = 26
} TileType;

typedef enum {
	NOTHING = 0, //should not reach
	NORTH,
	SOUTH,
	EAST,
	WEST,
	NS, // North-South orientation (vertical on map)
	EW  // East-West orientation (horizontal on map)
} Door_Side;

typedef enum {
	DOOR_CLOSED = 0,
	DOOR_OPENING,
	DOOR_OPEN,
	DOOR_CLOSING
} DoorState;

typedef struct Doors {
	Door_Side side;
	DoorState state;
	float open_amount; // 0.0 (closed) to 1.0 (fully open)
	float timer;       // For animation or auto-close delay
	bool active;       // If this tile has a door
} Doors;

typedef struct Tile {
	Doors doors;
	bool solid;
	TileType type;
} Tile;

typedef struct Level {
	Tile map[MAP_ROWS][MAP_COLS];
	const TileType (*map_id)[MAP_ROWS][MAP_COLS];
	const Door_Side (*door_map_id)[MAP_ROWS][MAP_COLS];
} Level;
