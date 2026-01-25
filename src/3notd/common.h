#pragma once
#include <stdint.h>
#include <tft.h>

#define LOOKING_POWER 80.f
#define WALKING_POWER 140.f
#define FOV 200

#define BLUE rgb_to_rgb565(0, 0, 255)
#define YELLOW rgb_to_rgb565(160, 160, 0)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define MAX_NUMBER_SECTORS 128
#define MAX_WALLS_PER_SECTOR 8

#define TFT_WIDTH2 TFT_WIDTH / 2
#define TFT_HEIGHT2 TFT_HEIGHT / 2

// Max textures are 64
typedef struct {
	int w, h;		   // texture width/height
	const unsigned char *name; // pointer to pixel data
} TextureMaps;

// Range: -32,767 to +32,767
typedef int16_t fixed_t;

struct Texture {
	uint8_t texture_index;
	uint8_t shade;
};

struct WallProperties {
	uint8_t active : 1; // used ?
	uint8_t solid : 1;  // 0 = portal (walkable), 1 = solid wall
	uint8_t texture_scale;

	struct Texture texture;

	// These bits define the wall's vertical size
	uint32_t bottom_height : 12;
	uint32_t top_height : 12;

	// Range: -32,767 to +32,767
	fixed_t p1_x;
	fixed_t p1_y;
	fixed_t p2_x;
	fixed_t p2_y;
};

struct FloorProperties {
	uint8_t walkable;
	uint32_t texture_tile_x : 12;
	uint32_t texture_tile_y : 12;
	struct Texture texture;
};

struct Sector {
	struct WallProperties walls[MAX_WALLS_PER_SECTOR];
	struct FloorProperties floor;

	fixed_t distance; // Runtime Painter's Algorithm
	uint8_t surface;  // Runtime flag for rendering
};

struct Map {
	char name[32];	   // Map Name
	char filename[32]; // File Name
	uint16_t num_sectors;
	struct Sector sectors[MAX_NUMBER_SECTORS];
};

typedef struct {
	float cos[360];
	float sin[360];
} math;

typedef struct {
	int x, y, z;
	int a; // Angle
	int l; // Look
} player;

extern player P;
extern math M;
extern struct Map current_map;
extern TextureMaps Textures[64];
