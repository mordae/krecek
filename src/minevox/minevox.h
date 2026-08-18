#pragma once
#include <sdk.h>
#include <tft.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mem.h"

/*
 * ===========================================================================
 * Minevox
 * ===========================================================================
 *
 * A small voxel survival game for the Krecek handheld: a 160x120 screen,
 * 264 KiB of RAM, one analog stick, six buttons and no floating point unit.
 *
 * This header is the vocabulary of the whole game. Every other file
 * includes it, and each subsystem lives in its own .c file:
 *
 *   mem.c     our own allocators, so we never call malloc()
 *   blocks.c  what a block is, and the textures we generate for it
 *   world.c   the voxel volume, terrain generation and ray casting
 *   save.c    worlds on disk, several of them
 *   items.c   the inventory, dropped items and crafting
 *   mobs.c    creatures, their behaviour and their sprites
 *   nether.c  the nether reactor and the event it triggers
 *   player.c  physics, survival stats and what the buttons do
 *   render.c  the per pixel ray caster that draws the world
 *   hud.c     hearts, hotbar and crosshair
 *   menu.c    every full screen page, driven by buttons or by touch
 *   main.c    the handful of callbacks the SDK expects
 *
 * A COORDINATE SYSTEM, ONCE, SO IT IS NEVER IN DOUBT
 *
 *   x and y run horizontally, z points at the sky. A block at integer
 *   coordinates (x, y, z) fills the cube from (x, y, z) to (x+1, y+1, z+1),
 *   so the block you stand on is the one at floor(position) - 1 in z.
 *   Angles: yaw 0 looks along +x, and yaw increases counter clockwise.
 *   Pitch is 0 at the horizon and positive looking up.
 */

#if !defined(M_PI)
#define M_PI 3.14159265358979323846f
#endif

/*
 * ---------------------------------------------------------------------------
 * The world
 * ---------------------------------------------------------------------------
 *
 * WHY THESE NUMBERS?
 *
 * We store one byte per voxel. That is the single most important decision
 * in the whole program, so it is worth spelling out.
 *
 * An earlier version packed two blocks per byte to save space, which
 * limited us to 16 kinds of block. Survival needs more than that: ores,
 * ingot blocks, nether blocks, a crafting table. One byte gives us 256
 * kinds and removes the shift-and-mask from the ray caster's innermost
 * loop, which is the hottest code in the game.
 *
 * The price is memory: 48 * 48 * 40 = 76800 bytes, 75 KiB of our 264 KiB.
 * That is why the map is 48 blocks across rather than 64. It is still
 * wider than you can see, because fog swallows everything past 38 blocks.
 * The extra height instead went upwards, to 40, because the nether spire
 * needs somewhere to grow.
 */
#define WORLD_X 48
#define WORLD_Y 48
#define WORLD_Z 40
#define SEA_LEVEL 12

#define SCREEN_W TFT_WIDTH
#define SCREEN_H TFT_HEIGHT

/*
 * Blocks.
 *
 * The numbers matter: they are written into save files, so inserting a new
 * block in the middle would change what every existing world contains. Add
 * new ones at the end.
 */
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
	BLOCK_GOLD_ORE,
	BLOCK_GLOWSTONE,
	BLOCK_BEDROCK,
	BLOCK_GOLD_BLOCK,
	BLOCK_IRON_BLOCK,
	BLOCK_CRAFTING,
	BLOCK_OBSIDIAN,
	BLOCK_NETHERRACK,
	BLOCK_GLOWING_OBSIDIAN,
	BLOCK_REACTOR,
	BLOCK_LAVA,
	NUM_BLOCKS,
};

/* Faces we texture separately. */
enum face {
	FACE_TOP = 0,
	FACE_SIDE,
	FACE_BOTTOM,
	NUM_FACES,
};

#define TEX_SIZE 8
#define TEX_MASK (TEX_SIZE - 1)

/*
 * What kind of tool a block wants before it gives up its drop.
 *
 * Mining without the right tool still breaks the block, it just leaves
 * nothing behind, exactly like the real game.
 */
enum tool_kind {
	TOOL_NONE = 0, /* Hands are fine. */
	TOOL_PICK,     /* Needs a pickaxe. */
	TOOL_AXE,      /* Faster with an axe, but hands work. */
};

struct block_info {
	const char *name;
	uint8_t solid;	  /* Stops the player walking through. */
	uint8_t opaque;	  /* Stops rays, so you cannot see past it. */
	uint8_t liquid;	  /* Can be swum through. */
	uint8_t shades;	  /* Casts a shadow, keeping daylight out. */
	uint8_t light;	  /* Light this block gives off, 0 to 15. */
	uint8_t hardness; /* Mining time in tenths of a second. */
	uint8_t tool;	  /* One of enum tool_kind. */
	uint16_t drop;	  /* Item id it leaves behind, 0 for nothing. */
};

extern const struct block_info block_info[NUM_BLOCKS];

/* Procedurally generated textures, indexed by block, face and texel. */
extern color_t block_tex[NUM_BLOCKS][NUM_FACES][TEX_SIZE * TEX_SIZE];

void blocks_init(void);

/*
 * Draw a small isometric cube of a block, top left corner at x, y.
 * Brightness is out of 32, so 32 is the texture's own colour.
 */
void blocks_draw_icon(int x, int y, int size, uint8_t block, uint8_t bright);

inline static bool block_is_solid(uint8_t block)
{
	return block_info[block].solid;
}

inline static bool block_is_opaque(uint8_t block)
{
	return block_info[block].opaque;
}

/*
 * ---------------------------------------------------------------------------
 * Items
 * ---------------------------------------------------------------------------
 *
 * An item id is either a block id, in which case the item can be placed in
 * the world, or one of the ids from ITEM_BASE upwards, which only exist in
 * the inventory. Keeping them in one numbering means the inventory, the
 * hotbar and the crafting table all speak a single language.
 */
#define ITEM_BASE 128

enum item {
	ITEM_STICK = ITEM_BASE,
	ITEM_COAL,
	ITEM_IRON_INGOT,
	ITEM_GOLD_INGOT,
	ITEM_PORKCHOP,
	ITEM_WOOD_PICK,
	ITEM_STONE_PICK,
	ITEM_IRON_PICK,
	ITEM_SWORD,
	NUM_ITEM_IDS,
};

#define ITEM_COUNT (NUM_ITEM_IDS - ITEM_BASE)

struct item_info {
	const char *name;
	uint8_t stack;	    /* Largest number that fits in one slot. */
	uint8_t tool;	    /* enum tool_kind this item counts as. */
	uint8_t tool_speed; /* Mining speed multiplier, in sixteenths. */
	uint8_t damage;	    /* Extra damage when swung at a mob. */
	uint8_t heal;	    /* Health restored when eaten, 0 if inedible. */
	color_t color;	    /* Colour used to draw it when it is not a block. */
};

extern const struct item_info item_info[ITEM_COUNT];

/* True when this id can be placed in the world. */
inline static bool item_is_block(uint16_t item)
{
	return item > 0 && item < NUM_BLOCKS;
}

const char *item_name(uint16_t item);
uint8_t item_stack_size(uint16_t item);
uint8_t item_tool_kind(uint16_t item);

/*
 * The inventory: nine hotbar slots followed by twenty seven storage slots,
 * laid out exactly like the real game so the muscle memory transfers.
 */
#define HOTBAR_SLOTS 9
#define STORAGE_SLOTS 27
#define INVENTORY_SLOTS (HOTBAR_SLOTS + STORAGE_SLOTS)

struct inv_slot {
	uint16_t item;
	uint16_t count;
};

extern struct inv_slot inventory[INVENTORY_SLOTS];

void inventory_clear(void);
bool inventory_add(uint16_t item, int count);
bool inventory_take(uint16_t item, int count);
int inventory_count(uint16_t item);
void inventory_drop_all(float x, float y, float z);

/*
 * Dropped items lying in the world, waiting to be picked up. These are
 * allocated from a pool in mem.c, which is the main reason that file
 * exists.
 */
struct drop {
	struct drop *next; /* Singly linked list of live drops. */
	float x, y, z;
	float vz;
	uint16_t item;
	uint16_t count;
	float age;   /* Seconds since it appeared, used for the bobbing. */
	float delay; /* Cannot be picked up until this reaches zero. */
};

extern struct drop *drops;

void items_init(void);
void items_reset(void);
void items_spawn(float x, float y, float z, uint16_t item, int count);
void items_update(float dt);
int items_live_count(void);

/* Crafting, including the recipes that stand in for smelting. */
#define CRAFT_INPUTS 2

struct recipe {
	uint16_t in[CRAFT_INPUTS];
	uint8_t in_count[CRAFT_INPUTS];
	uint16_t out;
	uint8_t out_count;
	bool needs_table; /* Some recipes want a crafting table nearby. */
};

extern const struct recipe recipes[];
extern const int num_recipes;

bool recipe_can_make(int index);
bool recipe_make(int index);

/*
 * ---------------------------------------------------------------------------
 * Mobs
 * ---------------------------------------------------------------------------
 *
 * Creatures are drawn as flat sprites that always face the camera, the
 * same trick Doom used. On a 160x120 screen a sprite is indistinguishable
 * from a box of voxels, and it costs a tiny fraction of the work.
 */
enum mob_type {
	MOB_PIG = 0,
	MOB_SHEEP,
	MOB_COW,
	MOB_ZOMBIE,
	MOB_SKELETON,
	MOB_PIGMAN,
	NUM_MOB_TYPES,
};

#define MOB_SPRITE 16

struct mob_info {
	const char *name;
	uint8_t hostile; /* Chases and hits the player. */
	uint8_t max_health;
	uint8_t damage; /* Health taken per hit. */
	uint8_t nether; /* Only spawns during a nether event. */
	float speed;
	float height;
	uint16_t drop; /* Item left behind when killed. */
	uint8_t drop_count;
};

extern const struct mob_info mob_info[NUM_MOB_TYPES];

struct mob {
	struct mob *next;
	uint8_t type;
	uint8_t health;
	float x, y, z;
	float vx, vy, vz;
	float yaw;
	float think;  /* Seconds until it picks a new direction. */
	float hurt;   /* Flashes red while above zero. */
	float attack; /* Cooldown between its hits. */
	float bob;    /* Walk animation phase. */
	bool on_ground;
};

extern struct mob *mobs;

void mobs_init(void);
void mobs_reset(void);
struct mob *mobs_spawn(int type, float x, float y, float z);
void mobs_update(float dt);
void mobs_populate(void);
int mobs_live_count(void);

/* Damage a mob, returning true when the blow kills it. */
bool mobs_hurt(struct mob *mob, int amount, float push_x, float push_y);

/* Unlink a dead mob from the live list and give its slot back. */
void mobs_kill(struct mob *mob);

/* The mob the player is looking at, or NULL. */
struct mob *mobs_pick(float ox, float oy, float oz, float dx, float dy, float dz, float reach,
		      float *out_dist);

/* Sprite pixels for a mob type, MOB_SPRITE squared, keyed on transparency. */
extern color_t mob_sprite[NUM_MOB_TYPES][MOB_SPRITE * MOB_SPRITE];

/*
 * ---------------------------------------------------------------------------
 * The nether reactor
 * ---------------------------------------------------------------------------
 *
 * A nod to the old pocket and bedrock versions, which had no nether
 * dimension. Instead you built a small structure of gold and cobblestone
 * around a reactor core, tapped it, and the world itself turned hostile
 * for a while: a spire of netherrack grew around you, the sky went red and
 * the place filled with pigmen and loot.
 *
 * That fits a handheld far better than a second world would, because it
 * needs no extra voxel storage at all.
 */
enum nether_state {
	NETHER_IDLE = 0,
	NETHER_ACTIVE,
	NETHER_FADING,
};

extern enum nether_state nether_state;
extern float nether_timer;

void nether_init(void);
void nether_reset(void);

/* True when the blocks around this core match the pattern. */
bool nether_pattern_ok(int x, int y, int z);

/* Try to light the core the player is looking at. Returns true on success. */
bool nether_activate(int x, int y, int z);

void nether_update(float dt);

/* How red the world looks right now, 0 to 1. */
float nether_intensity(void);

/*
 * ---------------------------------------------------------------------------
 * World storage
 * ---------------------------------------------------------------------------
 */
extern uint8_t world_data[WORLD_X * WORLD_Y * WORLD_Z];

/* Height of the lowest block in each column that still sees the sky. */
extern uint8_t world_height[WORLD_X][WORLD_Y];

/* One above the highest block anywhere, so rays can give up early. */
extern uint8_t world_top;

#define MAX_LIGHTS 48
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

/* Read a block. The caller promises the coordinates are inside the world. */
inline static uint8_t world_get_fast(int x, int y, int z)
{
	return world_data[world_index(x, y, z)];
}

/* Read a block, treating everything outside the world as air. */
inline static uint8_t world_get(int x, int y, int z)
{
	if (!world_inside(x, y, z))
		return BLOCK_AIR;

	return world_get_fast(x, y, z);
}

/* Write a block without touching the height map or the light list. */
inline static void world_put_raw(int x, int y, int z, uint8_t block)
{
	world_data[world_index(x, y, z)] = block;
}

void world_generate(uint32_t seed);
void world_update_height(int x, int y);
void world_rebuild_lights(void);
void world_select_lights(float px, float py, float pz);

/* Change a block, keeping the height map, lights and save file in step. */
void world_set(int x, int y, int z, uint8_t block);

/* Same, but does not record the change for the save file. */
void world_set_quiet(int x, int y, int z, uint8_t block);

void world_find_spawn(float *out_x, float *out_y, float *out_z);

/* Walk a ray through the voxels. True when it hits a block. */
bool world_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_dist,
		   int *hx, int *hy, int *hz, int *px, int *py, int *pz);

/*
 * ---------------------------------------------------------------------------
 * Saved worlds
 * ---------------------------------------------------------------------------
 *
 * The console gives every game four save slots of 4 KiB each. Storing a
 * whole voxel world would need 75 KiB, so instead we save the seed and the
 * list of blocks the player changed. Regenerating from the seed and
 * replaying the changes rebuilds the world exactly, and a slot holds well
 * over a thousand edits.
 */
#define WORLD_SLOTS 4
#define WORLD_NAME_LEN 12

struct world_summary {
	bool used;
	char name[WORLD_NAME_LEN + 1];
	uint32_t seed;
	uint16_t edits;
	uint16_t minutes; /* Time played, rounded down. */
	uint8_t health;
	bool survival;
};

extern int current_world;
extern bool survival_mode;
extern float world_play_time;

void save_scan(void);
const struct world_summary *save_summary(int slot);
bool save_load(int slot);
void save_store(int slot);
void save_erase(int slot);
void save_new(int slot, const char *name, uint32_t seed, bool survival);
void save_forget_edits(void);

/* Record a change for the save file. Called by world_set. */
void save_note_edit(int x, int y, int z, uint8_t block);
int save_edit_count(void);

/*
 * ---------------------------------------------------------------------------
 * The player
 * ---------------------------------------------------------------------------
 */
struct player {
	float x, y, z; /* Position of the feet. */
	float vx, vy, vz;
	float yaw, pitch;
	float bob; /* Vertical head bob, already in blocks. */
	bool on_ground;
	bool in_water;
	bool head_in_water;
	bool flying;

	/* Survival stats. Creative mode leaves them alone. */
	int health;
	int max_health;
	float regen;	  /* Counts up to the next point of healing. */
	float air;	  /* Seconds of breath left under water. */
	float hurt_flash; /* Screen flashes red while above zero. */
	float fall_from;  /* Height where the current fall started. */

	float respawn_x, respawn_y, respawn_z;

	int slot; /* Selected hotbar slot. */

	bool has_target;
	int tx, ty, tz; /* Block being aimed at. */
	int px, py, pz; /* Empty space in front of it. */

	float mining;
	float mining_need;
	float swing;
};

extern struct player player;

#define PLAYER_EYE 1.62f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_RADIUS 0.3f
#define REACH 5.5f
#define PLAYER_MAX_HEALTH 20

void player_reset(bool keep_position);
void player_input(float dt);
void player_update(float dt);
void player_look_dir(float *dx, float *dy, float *dz);
void player_hurt(int amount);
void player_heal(int amount);
uint16_t player_held_item(void);

/*
 * ---------------------------------------------------------------------------
 * Rendering
 * ---------------------------------------------------------------------------
 */
void render_init(void);
void render_world(void);
void render_sky_colors(color_t *zenith, color_t *horizon, float *daylight);
void render_darken(int x0, int y0, int x1, int y1, uint8_t amount);

/* Screen pixels per ray. One means a ray for every single pixel. */
extern int render_scale;

/*
 * How far away the ray caster found something, so that sprites can be
 * hidden behind hills. This is a Z-BUFFER, the same idea a graphics card
 * uses, with two economies forced on us by having 264 KiB of RAM:
 *
 *   - one byte per entry instead of four, scaled so 255 is the far plane;
 *   - one entry per two by two block of pixels rather than per pixel.
 *
 * Full resolution would cost 19200 bytes and we simply do not have them.
 * Quartering it costs a pixel of accuracy at the edge of an occluder,
 * which on a screen this size nobody has ever noticed.
 */
#define DEPTH_W (SCREEN_W / 2)
#define DEPTH_H (SCREEN_H / 2)

extern uint8_t depth_buffer[DEPTH_W * DEPTH_H];

/* Nearest thing drawn in the cell covering this pixel. */
inline static uint8_t depth_at(int px, int py)
{
	return depth_buffer[(py >> 1) * DEPTH_W + (px >> 1)];
}

/* Remember a depth, keeping whichever is nearest. */
inline static void depth_store(int px, int py, uint8_t d)
{
	uint8_t *slot = &depth_buffer[(py >> 1) * DEPTH_W + (px >> 1)];

	if (d < *slot)
		*slot = d;
}

/*
 * ---------------------------------------------------------------------------
 * Screens, HUD and input
 * ---------------------------------------------------------------------------
 */
enum game_state {
	STATE_TITLE = 0,
	STATE_WORLDS, /* Pick, create or delete a world. */
	STATE_NEW,    /* Name and settings for a new world. */
	STATE_PLAY,
	STATE_PAUSE,
	STATE_INVENTORY,
	STATE_CRAFT,
	STATE_PICKER, /* Creative block picker. */
	STATE_DEAD,
	STATE_HELP,
};

extern enum game_state game_state;
extern bool opt_sfx;
extern bool opt_music;
extern bool opt_debug;

void hud_paint(float dt);

/* Small drawing helpers the menus share with the HUD. */
void hud_shadow_text(int x, int y, color_t color, const char *text);
void hud_shadow_center(int x, int y, color_t color, const char *text);
void hud_frame(int x0, int y0, int x1, int y1, color_t color);
void hud_draw_item(int x, int y, int size, uint16_t item, uint8_t bright);
uint8_t hud_icon_bright(void);

void menu_paint(float dt);
void menu_input(float dt);
void menu_open(enum game_state state);
void menu_toast(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void sfx_play(const char *melody);

/*
 * A pointer, whether it is the touch screen or the mouse on the host.
 *
 * Menus are much nicer to use with a finger than with a stick, and the
 * console has a resistive touch panel, so every page that can be tapped
 * is also reachable with the buttons.
 */
struct pointer {
	int x, y;      /* Screen pixels. */
	bool down;     /* Currently pressed. */
	bool pressed;  /* Went down this frame. */
	bool released; /* Came up this frame. */
	int press_x, press_y;
};

extern struct pointer pointer;

void pointer_update(void);

/* True when the pointer was released inside this rectangle. */
bool ui_button(int x, int y, int w, int h, const char *label, bool selected);

/* True when the pointer is inside this rectangle right now. */
bool ui_hover(int x, int y, int w, int h);
