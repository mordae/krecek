#pragma once

#define TILE_SIZE 16
#define MAP_ROWS 20
#define MAP_COLS 15
#define MAP_ROWS_2 10
#define MAP_COLS_2 7.5f

#define PLAYER_SPRITES 4
#define PLAYER_DIRECTION 4
#define PLAYER_WIDTH 6
#define PLAYER_HEIGHT 12

#define AUTOSAFE_TIME 30 //seconds

#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define ORANGE 0xFD20
#define PURPLE 0x8010
#define PINK 0xFE19
#define BROWN 0xA145
#define GRAY 0x8410
#define DARK_GRAY 0x4208
#define LIGHT_GRAY 0xC618
#define DARK_BLUE 0x0010
#define DARK_GREEN 0x03E0
#define DARK_RED 0x8000
#define LIGHT_BLUE 0x051F
#define LIGHT_GREEN 0x87F0
#define LIGHT_RED 0xFC10

#define DARK_GREY DARK_GRAY
#define LIGHT_GREY LIGHT_GRAY

typedef enum {
	EMPTY = 0,
	FLOOR = 1,
	FLOOR_1,
	FLOOR_2,
	FLOOR_3,
	FLOOR_4,
	LEFT_WALL,
	RIGHT_WALL,
	NORTH_WALL,
	SOUTH_WALL,
} TileType;
