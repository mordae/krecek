#pragma once
#define MAP_COLS 30
#define MAP_ROWS 30

#define ENUM_TYPES 22

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
  AMMO_BOX,       // (was UN2)
  SHOTGUN_PICKUP, // (was UN3)
  ENEMY1,         // UN 4
  MOSS_STONE,
  WOOD_WALL,
  PRISMARYN,
  CHOLOTATE,
  ENEMY2, // UN 5
  PATTERN_IN_BROWN,
  SAND_WITH_MOSS,
  IRON,
  TELEPORT = 21,
  PLAYER_SPAWN
} TileType;
