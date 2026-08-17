#pragma once
#include <sdk.h>
#include <tft.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(M_PI)
#define M_PI 3.14159265358979323846f
#endif

/*
 * The world is a fixed voxel volume. X and Y are horizontal, Z points up.
 * Blocks are stored as nibbles, so we get 64x64x32 voxels for 64 KiB.
 */
#define WORLD_X 64
#define WORLD_Y 64
#define WORLD_Z 32
#define SEA_LEVEL 12

#define SCREEN_W TFT_WIDTH
#define SCREEN_H TFT_HEIGHT

/* Blocks. Only 16 of them fit into a nibble. */
enum block {
	BLOCK_AIR = 0,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
	BLOCK_COBBLE,
	BLOCK_SAND,
	BLOCK_LOG,
	BLOCK_LEAVES,
	BLOCK_PLANKS,
	BLOCK_GLASS,
	BLOCK_WATER,
	BLOCK_BRICK,
	BLOCK_COAL_ORE,
	BLOCK_IRON_ORE,
	BLOCK_GLOWSTONE,
	BLOCK_BEDROCK,
	NUM_BLOCKS,
};

/* Faces we can texture separately. */
enum face {
	FACE_TOP = 0,
	FACE_SIDE,
	FACE_BOTTOM,
	NUM_FACES,
};

#define TEX_SIZE 8
#define TEX_MASK (TEX_SIZE - 1)

struct block_info {
	const char *name;
	uint8_t solid;	  /* Stops the player. */
	uint8_t opaque;	  /* Stops the rays. */
	uint8_t liquid;	  /* Can be swum through. */
	uint8_t shades;	  /* Casts a shadow, keeping the sky light out. */
	uint8_t light;	  /* Emitted light, 0 to 15. */
	uint8_t hardness; /* Mining time in 1/10 s, 0 means instant. */
};

extern const struct block_info block_info[NUM_BLOCKS];

/* Procedurally generated textures, indexed by block, face and texel. */
extern color_t block_tex[NUM_BLOCKS][NUM_FACES][TEX_SIZE * TEX_SIZE];

/* Flat top-down color of a block, used by the HUD. */
extern color_t block_avg[NUM_BLOCKS];

void blocks_init(void);

/*
 * Draw a small isometric cube of given block, top-left corner at x, y.
 * Brightness is out of 32.
 */
void blocks_draw_icon(int x, int y, int size, uint8_t block, uint8_t bright);

/*
 * World storage.
 */
extern uint8_t world_data[WORLD_X * WORLD_Y * WORLD_Z / 2];

/* Height of the first block that sees the sky, per column. */
extern uint8_t world_height[WORLD_X][WORLD_Y];

/* One above the highest block in the world. Rays above it only see sky. */
extern uint8_t world_top;

/* Positions of light emitting blocks near the player. */
#define MAX_LIGHTS 32
#define MAX_ACTIVE_LIGHTS 6

struct light {
	uint8_t x, y, z;
	uint8_t level;
};

extern struct light world_lights[MAX_LIGHTS];
extern int world_num_lights;

extern struct light active_lights[MAX_ACTIVE_LIGHTS];
extern int num_active_lights;

extern uint32_t world_seed;

/* Time of day in seconds, wrapping around DAY_LENGTH. */
#define DAY_LENGTH 480.0f
extern float world_time;

inline static bool world_inside(int x, int y, int z)
{
	return (unsigned)x < WORLD_X && (unsigned)y < WORLD_Y && (unsigned)z < WORLD_Z;
}

inline static unsigned world_index(int x, int y, int z)
{
	return ((unsigned)x * WORLD_Y + (unsigned)y) * WORLD_Z + (unsigned)z;
}

/* Read a block without bounds checking. */
inline static uint8_t world_get_fast(int x, int y, int z)
{
	unsigned i = world_index(x, y, z);
	uint8_t pair = world_data[i >> 1];
	return (i & 1) ? (pair >> 4) : (pair & 0x0f);
}

/* Read a block, treating everything outside of the world as air. */
inline static uint8_t world_get(int x, int y, int z)
{
	if (!world_inside(x, y, z))
		return BLOCK_AIR;

	return world_get_fast(x, y, z);
}

/* Write a block without updating the derived data. */
inline static void world_put_raw(int x, int y, int z, uint8_t block)
{
	unsigned i = world_index(x, y, z);
	uint8_t *pair = &world_data[i >> 1];

	if (i & 1)
		*pair = (*pair & 0x0f) | (block << 4);
	else
		*pair = (*pair & 0xf0) | block;
}

inline static bool block_is_solid(uint8_t block)
{
	return block_info[block].solid;
}

inline static bool block_is_opaque(uint8_t block)
{
	return block_info[block].opaque;
}

/* Generate a brand new world. */
void world_generate(uint32_t seed);

/* Recalculate the sky height of a single column. */
void world_update_height(int x, int y);

/* Rebuild the list of light emitting blocks. */
void world_rebuild_lights(void);

/* Pick the lights closest to given point for the renderer to use. */
void world_select_lights(float px, float py, float pz);

/* Change a block and record the change for the save file. */
void world_set(int x, int y, int z, uint8_t block);

/* Find a safe spot to drop the player at. */
void world_find_spawn(float *out_x, float *out_y, float *out_z);

/* Persistence. Returns false when there is nothing to load. */
bool world_load(void);
void world_save(void);
void world_forget_edits(void);

/*
 * The player.
 */
struct player {
	float x, y, z;	    /* Feet position. */
	float vx, vy, vz;   /* Velocity. */
	float yaw, pitch;   /* Where we look. */
	float bob;	    /* Walk cycle phase. */
	bool on_ground;	    /* Standing on something solid. */
	bool in_water;	    /* Eyes or body in a liquid. */
	bool flying;	    /* Creative flight. */
	bool head_in_water; /* Eyes below the water surface. */

	uint8_t hotbar[9];
	int slot;

	/* Block we are aiming at, if any. */
	bool has_target;
	int tx, ty, tz; /* Targeted block. */
	int px, py, pz; /* Free space in front of it. */

	float mining;	   /* Seconds spent mining the current target. */
	float mining_need; /* Seconds needed to break it. */
	float swing;	   /* Hand swing animation phase. */
};

extern struct player player;

#define PLAYER_EYE 1.62f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_RADIUS 0.3f
#define REACH 5.5f

void player_reset(void);
void player_input(float dt);
void player_update(float dt);
void player_look_dir(float *dx, float *dy, float *dz);

/* Aim a ray at the world. Returns true when it hits a block. */
bool world_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_dist,
		   int *hx, int *hy, int *hz, int *px, int *py, int *pz);

/*
 * Rendering.
 */
void render_init(void);
void render_world(void);
void render_sky_colors(color_t *zenith, color_t *horizon, float *daylight);

/* Dim a rectangle of the frame buffer. Amount is out of 32. */
void render_darken(int x0, int y0, int x1, int y1, uint8_t amount);

/* How many screen pixels one ray covers. 1 means a ray per pixel. */
extern int render_scale;

/*
 * HUD and menus.
 */
enum game_state {
	STATE_TITLE = 0,
	STATE_PLAY,
	STATE_PAUSE,
	STATE_PICKER,
};

extern enum game_state game_state;
extern bool opt_sfx;
extern bool opt_music;
extern bool opt_debug;

void hud_paint(float dt);
void hud_input(float dt);
void hud_notice_save(bool exists);
void sfx_play(const char *melody);
