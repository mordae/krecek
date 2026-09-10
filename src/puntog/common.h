#pragma once
#include <sdk.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define TILE_SIZE 16
#define DUNGEON_COLS 30
#define DUNGEON_ROWS 24
#define LOBBY_COLS 14
#define LOBBY_ROWS 10

#define PLAYER_SPRITES 4
#define PLAYER_DIRECTION 4
#define PLAYER_WIDTH 6
#define PLAYER_HEIGHT 12

#define MAX_ACTIVE_ZOMBIES 14
#define POOL_SIZE_ZOMBIES 24
#define MAX_ACTIVE_PROJECTILES 18
#define POOL_SIZE_PROJECTILES 24
#define MAX_ACTIVE_PARTICLES 48
#define POOL_SIZE_PARTICLES 64
#define MAX_ACTIVE_PICKUPS 16
#define MAX_ACTIVE_TRAPS 6
#define MAX_ROOMS 6

#define AUTOSAVE_TIME 15.0f

// Color constants (RGB565)
#define WHITE       0xFFFF
#define BLACK       0x0000
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define ORANGE      0xFD20
#define PURPLE      0x8010
#define PINK        0xFE19
#define BROWN       0xA145
#define GRAY        0x8410
#define DARK_GRAY   0x2104
#define LIGHT_GRAY  0xC618
#define DARK_BLUE   0x0010
#define DARK_GREEN  0x03E0
#define DARK_RED    0x8000
#define LIGHT_BLUE  0x051F
#define LIGHT_GREEN 0x87F0
#define GOLD        0xFEA0

// 216-color RGB spectrum (6x6x6 color cube: r, g, b in 0..5)
static inline color_t rgb216(int r, int g, int b)
{
	static const uint8_t lev[6] = { 0, 51, 102, 153, 204, 255 };
	return rgb_to_rgb565(lev[r % 6], lev[g % 6], lev[b % 6]);
}

typedef struct {
	color_t c[4]; // 0=transparent, 1=outline, 2=base, 3=highlight
} Palette4;

typedef enum {
	EMPTY = 0,
	TILE_FLOOR = 1,
	TILE_FLOOR_1,
	TILE_FLOOR_2,
	TILE_FLOOR_3,
	TILE_FLOOR_4,
	TILE_LEFT_WALL,
	TILE_RIGHT_WALL,
	TILE_NORTH_WALL,
	TILE_SOUTH_WALL,
	TILE_CORNER_NW,
	TILE_CORNER_NE,
	TILE_CORNER_SW,
	TILE_CORNER_SE,
	TILE_CORRIDOR,
	TILE_TORCH_0,
	TILE_TORCH_1,
	TILE_LAVA_0,
	TILE_LAVA_1,
	TILE_BANNER,
	TILE_SPAWNER,
	TILE_LADDER,
	TILE_PORTAL,
	TILE_CHEST_CLOSED,
	TILE_CHEST_OPEN,
	TILE_WARDROBE,
	TILE_SHOP_ALTAR,
} TileType;

typedef enum {
	SPELL_FIREBALL = 0,
	SPELL_ICE_DAGGER,
	SPELL_MAGIC_MISSILE,
	SPELL_ARCANE_SHIELD,
	SPELL_RUNE_TRAP,
	NUM_SPELLS,
} SpellType;

typedef struct {
	const char *name;
	const char *aspect;
	float heat_cost;
	int damage;
	float cooldown;
	const char *desc;
} SpellDef;

typedef enum {
	STATE_TITLE = 0,
	STATE_NAME_MENU,
	STATE_WARDROBE,
	STATE_LOBBY,
	STATE_DUNGEON,
	STATE_SPELL_MENU,
	STATE_SHOP,
	STATE_GAMEOVER,
	STATE_FLOOR_CLEAR,
	STATE_PAUSE,
	STATE_TUTORIAL,
	STATE_INFO,
} GameState;

#define SAVE_MAGIC 0x504E5447 // 'PNTG'

typedef struct Save {
	uint32_t magic;
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
	bool set;
	float timespend;
	int coins;
	int max_hp;
	int cooling_level;
	int spell_power;
	int potions;
	int highest_floor;
	int music_enabled;
	int music_volume;
} Save;

typedef struct {
	float x, y;
	float vx, vy;
	float speed;
	int hp, max_hp;
	int direction; // 0=Down, 1=Left, 2=Right, 3=Up
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
	bool nameset;

	// Inverted Heat System
	float heat; // 0.0 to 100.0
	bool overheated;
	float cooling_rate; // heat units cooled per second (base 26.0)

	// Spells & Combat
	SpellType selected_spell;
	float spell_cooldown;
	int spell_power; // bonus damage

	// Dodge roll
	float dodge_timer; // active roll duration
	float dodge_cd;    // cooldown
	float dodge_vx, dodge_vy;

	// Defensive shield
	float shield_timer; // duration remaining

	// Status & Invulnerability
	float invuln_timer;
	int coins;
	int potions; // consumable emergency coolers/heals

	// Animation
	float walk_anim;
	int walk_frame;
} Player;
