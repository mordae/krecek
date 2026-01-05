#include "common.h"
#include "extras/inludes.h"
#include "include_maps.h"
#include "maps.h"
#include "sdk/input.h"
#include "sdk/util.h"
#include "campaign.h"

#include <limits.h>

//*
//*   TODO: RANDOM GENERATE
//*

typedef int32_t fixed_t;
static fixed_t fixed_tile_size;
static fixed_t fixed_max_dist;
static fixed_t fixed_min_shade;
static fixed_t fixed_side_shade;
static fixed_t fixed_proj_plane_dist;
static fixed_t fixed_plane_scale;

#define FIXED_ONE ((fixed_t)FIXED_SCALE)

static inline __attribute__((always_inline)) fixed_t fixed_abs(fixed_t v)
{
	return v < 0 ? -v : v;
}

// Trig lookup table (Q16.16) to avoid per-ray sinf/cosf on RP2040.
// Size 1024 -> 8 KiB for sin+cos.
#define TRIG_LUT_BITS 10
#define TRIG_LUT_SIZE (1u << TRIG_LUT_BITS)
#define TRIG_LUT_MASK (TRIG_LUT_SIZE - 1u)

static fixed_t sin_lut[TRIG_LUT_SIZE];
static fixed_t cos_lut[TRIG_LUT_SIZE];
static bool trig_lut_ready = false;

static void trig_init_once(void)
{
	if (trig_lut_ready)
		return;

	for (unsigned i = 0; i < TRIG_LUT_SIZE; i++) {
		float a = (2.0f * (float)M_PI) * ((float)i / (float)TRIG_LUT_SIZE);
		sin_lut[i] = float_to_fixed(sinf(a));
		cos_lut[i] = float_to_fixed(cosf(a));
	}

	fixed_tile_size = float_to_fixed(TILE_SIZE);
	fixed_max_dist = float_to_fixed(15.0f * TILE_SIZE);
	fixed_min_shade = float_to_fixed(0.3f);
	fixed_side_shade = float_to_fixed(0.8f);
	fixed_proj_plane_dist = float_to_fixed(PROJECTION_PLANE_DISTANCE);
	fixed_plane_scale = float_to_fixed(tanf(FOV_RADIANS / 2.0f));

	trig_lut_ready = true;
}

static inline __attribute__((always_inline)) unsigned angle_to_lut(float radians)
{
	float t = radians * ((float)TRIG_LUT_SIZE / (2.0f * (float)M_PI));
	int idx = (int)t;
	idx &= (int)TRIG_LUT_MASK;
	return (unsigned)idx;
}

sdk_game_info("krecdoom", &image_cover_png);

typedef enum {
	TEX_HEALTH_PICKUP = 22, // TTO_00
	TEX_AMMO_PICKUP = 23,	// TTO_01
	TEX_SHOTGUN_PICKUP = 24 // TTO_02
} PickupTexture;

typedef struct {
	float x, y;
	int type; // HEALTH_PACK, AMMO_BOX, or SHOTGUN_PICKUP
	bool collected;
	bool active;
} Pickup;

typedef enum { GAME_MENU, GAME_PLAYING, GAME_DEAD } GameState;

typedef enum {
	BROKE = 0,
	PISTOL = 1,
	SHOTGUN,
} Gun;

typedef enum {
	TEX_ENEMY1 = 20, // TTE_00
	TEX_ENEMY2 = 21	 // TTE_01
} EnemyTexture;

// Enemy states
typedef enum {
	ENEMY_IDLE = 0,
	ENEMY_PATROL,
	ENEMY_CHASE,
	ENEMY_ATTACK,
	ENEMY_HURT,
	ENEMY_DEAD
} EnemyState;
typedef enum { CAMPAIGN, RANDOM } MapState;

typedef struct {
	float x, y;
	float angle;
	float relative_x, relative_y;
	int score;
	int health;
	int ammo;
	int gun;
	bool alive;
	bool strafing;
	bool shotgun_unlocked;
	bool has_moved;
	char *name;
} Player;

typedef struct {
	float x, y;
	int health;
	int max_health;
	bool alive;
	bool active;
	float attack_timer;
	uint32_t last_attack_time;
	uint32_t last_hit_time;
	int type; // 1 for Solder, 2 for
	EnemyState state;
	float state_timer;
	float patrol_angle;
	float patrol_timer;
	float chase_timer;
	float hurt_timer;
} Enemy;

typedef struct {
	bool debug;
	bool map;
	bool low;
	uint16_t tilecolor;
	float is_seen;
	MapState map_type;
} Mode;

typedef struct {
	bool hit_visible;
	uint32_t hit_timer_start;
	int hit_screen_x, hit_screen_y;
	int hit_size;
	uint16_t hit_color;
} Bullet;

typedef struct {
	bool visible;
	uint32_t timer_start;
} MuzzleFlash;

typedef struct {
	bool visible;
	uint32_t timer_start;
	char message[32];
} PickupMessage;

typedef struct {
	int tile_x, tile_y;
	int start_x, start_y;
	int window_start_x, window_start_y;
	int step_x, step_y;
	int hit, side;
	int p_x, p_y;
	float p_center_x, p_center_y;
} MMap;

typedef struct {
	int w, h;
	const unsigned char *name;
} TexureMaps;
typedef struct {
	int pointer;
} Menu;

Gun gun_select[N_GUNS] = { 0, 1, 2 };
static Player player;
static Menu menu;
static Enemy enemies[MAX_ENEMIES];
static int active_enemy_count = 0;
static Mode mode;
static Bullet bullet;
static MuzzleFlash muzzle_flash;
static PickupMessage pickup_msg;
static TexureMaps Textures[64];
static GameState game_state = GAME_MENU;
static bool collected_pickups[MAP_ROWS][MAP_COLS];
static float zBuffer[SCREEN_WIDTH]; // For depth testing
static Pickup pickups[MAX_PICKUPS];
static int active_pickup_count = 0;
static bool isWall(int Tile_x, int Tile_y);
static bool isPickup(int Tile_x, int Tile_y);
static void shootBullet(float start_angle, float max_range_tiles, int visual_size,
			uint16_t visual_color);
void Map_starter(const TileType (*next_map)[MAP_ROWS][MAP_COLS]);
static void handleShooting();
extern void map_starter_caller();
static void textures_load();
static void handlePickup(int tile_x, int tile_y);
extern void game_handle_audio(float dt, float volume);
TileType currentMap[MAP_ROWS][MAP_COLS];
static void renderPickups();
static void give_pickup(TileType tile);

static void initEnemies();
static void spawnEnemy(float x, float y, int health);
static void updateEnemies(float dt);
static void renderEnemies();
extern void draw_wall_column_asm(int x, int y_start, int y_end, uint32_t tex_pos, uint32_t step,
				 const uint8_t *tex_data, int tex_width, int tex_height, int tex_x,
				 int shade_int, int x2);

extern void draw_sprite_column_asm(int x, int y_start, int y_end, uint32_t tex_pos, uint32_t step,
				   const uint8_t *tex_data, int tex_width, int tex_height,
				   int tex_x, int shade_int, int is_hurt);
int randomnumb(int maxnumber)
{
	int number;

	number = seed * 214013 + 2531011;

	seed += 1;
	return abs((number >> 16) % maxnumber);
}

static void renderGame()
{
	trig_init_once();

	// Clear depth buffer
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		zBuffer[x] = 1e30f;
	}

	// Player direction + camera plane from angle (fixed point)
	unsigned aidx = angle_to_lut(player.angle);
	fixed_t dirX = cos_lut[aidx];
	fixed_t dirY = sin_lut[aidx];
	fixed_t planeX = -fixed_mul(dirY, fixed_plane_scale);
	fixed_t planeY = fixed_mul(dirX, fixed_plane_scale);

	// Player position in tile units (Q16.16)
	fixed_t posX = (fixed_t)(player.x * (float)FIXED_SCALE / TILE_SIZE);
	fixed_t posY = (fixed_t)(player.y * (float)FIXED_SCALE / TILE_SIZE);

	// Raycast every 2 columns and duplicate to the next column.
	for (int x = 0; x < SCREEN_WIDTH; x += 2) {
		int x2 = x + 1;

		// cameraX in [-1, +1]
		fixed_t cameraX = ((2 * x) << FIXED_SHIFT) / (SCREEN_WIDTH - 1) - FIXED_ONE;

		fixed_t rayDirX = dirX + fixed_mul(planeX, cameraX);
		fixed_t rayDirY = dirY + fixed_mul(planeY, cameraX);

		int mapX = posX >> FIXED_SHIFT;
		int mapY = posY >> FIXED_SHIFT;

		fixed_t deltaDistX = (rayDirX == 0) ? INT32_MAX / 2 :
						      fixed_abs(fixed_div(FIXED_ONE, rayDirX));
		fixed_t deltaDistY = (rayDirY == 0) ? INT32_MAX / 2 :
						      fixed_abs(fixed_div(FIXED_ONE, rayDirY));

		fixed_t sideDistX;
		fixed_t sideDistY;

		int stepX;
		int stepY;
		int side;

		if (rayDirX < 0) {
			stepX = -1;
			sideDistX = fixed_mul(posX - (mapX << FIXED_SHIFT), deltaDistX);
		} else {
			stepX = 1;
			sideDistX = fixed_mul(((mapX + 1) << FIXED_SHIFT) - posX, deltaDistX);
		}

		if (rayDirY < 0) {
			stepY = -1;
			sideDistY = fixed_mul(posY - (mapY << FIXED_SHIFT), deltaDistY);
		} else {
			stepY = 1;
			sideDistY = fixed_mul(((mapY + 1) << FIXED_SHIFT) - posY, deltaDistY);
		}

		// DDA
		for (;;) {
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
			} else {
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			}

			if (isWall(mapX, mapY))
				break;
		}

		fixed_t perpDist = (side == 0) ? (sideDistX - deltaDistX) :
						 (sideDistY - deltaDistY);
		if (perpDist < 1)
			perpDist = 1;

		// Convert to world units for z-buffer / shading
		fixed_t perpDistWorld = fixed_mul(perpDist, fixed_tile_size);
		if (perpDistWorld < 1)
			perpDistWorld = 1;

		float z = fixed_to_float(perpDistWorld);
		zBuffer[x] = z;
		if (x2 < SCREEN_WIDTH)
			zBuffer[x2] = z;

		// Project wall height (fixed -> int pixels)
		fixed_t wallHeight_fixed =
			fixed_div(fixed_mul(fixed_tile_size, fixed_proj_plane_dist), perpDistWorld);
		int wallHeight = wallHeight_fixed >> FIXED_SHIFT;
		if (wallHeight < 1)
			wallHeight = 1;

		int drawStart = (SCREEN_HEIGHT / 2) - (wallHeight / 2);
		int drawEnd = (SCREEN_HEIGHT / 2) + (wallHeight / 2);

		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		int wallType = map.type[mapY][mapX] - 1;
		if (wallType <= 0)
			wallType = 0;

		int texWidth = Textures[wallType].w;
		int texHeight = Textures[wallType].h;

		// wallX is the exact point of impact in tile units
		fixed_t wallX = (side == 0) ? (posY + fixed_mul(perpDist, rayDirY)) :
					      (posX + fixed_mul(perpDist, rayDirX));
		fixed_t wallXFrac = wallX & (FIXED_ONE - 1);

		int texX = (int)(((int64_t)wallXFrac * texWidth) >> FIXED_SHIFT);
		if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0)) {
			texX = texWidth - texX - 1;
		}

		// Texture stepping in fixed
		fixed_t step = (fixed_t)(((int64_t)texHeight << FIXED_SHIFT) / wallHeight);
		fixed_t texPos = (fixed_t)((
			(int64_t)(drawStart - SCREEN_HEIGHT / 2 + wallHeight / 2) * step));

		// Distance-based shade in fixed (world units)
		fixed_t shade = FIXED_SCALE - fixed_div(perpDistWorld, fixed_max_dist);
		if (shade < fixed_min_shade)
			shade = fixed_min_shade;
		if (side == 1)
			shade = fixed_mul(shade, fixed_side_shade);
		int shade_int = (shade >> 8) & 0xFFFF;

		draw_wall_column_asm(x, drawStart, drawEnd, texPos, step, Textures[wallType].name,
				     texWidth, texHeight, texX, shade_int, x2);
	}

	// Draw enemies / pickups using existing code (zBuffer remains in world units)
	renderEnemies();
	renderPickups();

	// Draw crosshair
	int cx = SCREEN_WIDTH / 2;
	int cy = SCREEN_HEIGHT / 2;
	tft_draw_pixel(cx, cy, WHITE);
	tft_draw_pixel(cx - 1, cy, WHITE);
	tft_draw_pixel(cx + 1, cy, WHITE);
	tft_draw_pixel(cx, cy - 1, WHITE);
	tft_draw_pixel(cx, cy + 1, WHITE);
}

static bool isWall(int Tile_x, int Tile_y)
{
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return true;
	}
	TileType tile = map.type[Tile_y][Tile_x];
	switch (tile) {
	case COBLE:
	case BRICKS:
	case STONE:
	case MAGMA:
	case WATER:
	case STONE_HOLE:
	case CHEKERD:
	case MOSS_STONE:
	case WOOD_WALL:
	case PRISMARYN:
	case CHOLOTATE:
	case PATTERN_IN_BROWN:
	case SAND_WITH_MOSS:
	case IRON:
	case BUNKER:
		return true;
	case PLAYER_SPAWN:
	case HEALTH_PACK:
	case AMMO_BOX:
	case SHOTGUN_PICKUP:
	case ENEMY1:
	case ENEMY2:
	case TELEPORT:
	case EMPTY:
		return false;
	default:
		return false;
		break;
	}
	return false;
}

static bool isPickup(int Tile_x, int Tile_y)
{
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return false;
	}
	// Check if already collected
	if (collected_pickups[Tile_y][Tile_x]) {
		return false;
	}
	TileType tile = map.type[Tile_y][Tile_x];

	// Enemy tiles are not pickups
	return (tile == HEALTH_PACK || tile == AMMO_BOX || tile == SHOTGUN_PICKUP);
}
static void give_pickup(TileType tile)
{
	if (tile == HEALTH_PACK) {
		if (player.health < MAX_HEALTH) {
			player.health += HEALTH_PACK_HEAL;
			if (player.health > MAX_HEALTH) {
				player.health = MAX_HEALTH;
			}
			sdk_melody_play("/i:sine e g");
			// Show pickup message
			pickup_msg.visible = true;
			pickup_msg.timer_start = time_us_64();
			strcpy(pickup_msg.message, "HEALTH +25");
			if (mode.debug) {
				printf("Picked up health pack! Health: %d\n", player.health);
			}
		}
	} else if (tile == AMMO_BOX) {
		if (player.ammo < MAX_AMMO) {
			player.ammo += AMMO_BOX_AMOUNT;
			if (player.ammo > MAX_AMMO) {
				player.ammo = MAX_AMMO;
				pickup_msg.visible = true;
				pickup_msg.timer_start = time_us_64();
				strcpy(pickup_msg.message, "MAX AMMO");
			}
			sdk_melody_play("/i:square d");
			// Show pickup message
			pickup_msg.visible = true;
			pickup_msg.timer_start = time_us_64();
			strcpy(pickup_msg.message, "AMMO +15");
			if (mode.debug) {
				printf("Picked up ammo! Ammo: %d\n", player.ammo);
			}
		}
	} else if (tile == SHOTGUN_PICKUP) {
		if (!player.shotgun_unlocked) {
			player.shotgun_unlocked = true;
			sdk_melody_play("/i:square c e g C");
			// Show pickup message
			pickup_msg.visible = true;
			pickup_msg.timer_start = time_us_64();
			strcpy(pickup_msg.message, "SHOTGUN UNLOCKED!");
			if (mode.debug) {
				printf("Shotgun unlocked!\n");
			}
		} else if (player.ammo < MAX_AMMO) {
			player.ammo += AMMO_SHOTBOX;
			if (player.ammo > MAX_AMMO) {
				pickup_msg.visible = true;
				pickup_msg.timer_start = time_us_64();
				strcpy(pickup_msg.message, "MAX AMMO");
				player.ammo = MAX_AMMO;
			}
			sdk_melody_play("/i:square d");
			pickup_msg.visible = true;
			pickup_msg.timer_start = time_us_64();
			strcpy(pickup_msg.message, "AMMO +5");
			if (mode.debug) {
				printf("Picked up ammo! Ammo: %d\n", player.ammo);
			}
		}
	}
}
static void handlePickup(int tile_x, int tile_y)
{
	TileType tile = map.type[tile_y][tile_x];
	collected_pickups[tile_y][tile_x] = true;

	// Deactivate the pickup in the array
	for (int i = 0; i < active_pickup_count; i++) {
		int pickup_tile_x = (int)(pickups[i].x / TILE_SIZE);
		int pickup_tile_y = (int)(pickups[i].y / TILE_SIZE);
		if (pickup_tile_x == tile_x && pickup_tile_y == tile_y) {
			pickups[i].collected = true;
			pickups[i].active = false;
			break;
		}
	}
	give_pickup(tile);
}
static void load_map(const TileType (*load_map)[MAP_ROWS][MAP_COLS])
{
	for (int r = 0; r < MAP_ROWS; r++) {
		for (int co = 0; co < MAP_COLS; co++) {
			map.type[r][co] = (*load_map)[r][co];
		}
	}
	map.map_id = load_map;
}
static void handlePlayerMovement(float dt)
{
	float player_dx = 0;
	float player_dy = 0;
	player.has_moved = false;

	if (sdk_inputs.joy_y > 500 || sdk_inputs.joy_y < -500) {
		player_dx = cosf(player.angle) * MOVE_SPEED * dt *
			    (-1 * (float)sdk_inputs.joy_y / 2048);
		player_dy = sinf(player.angle) * MOVE_SPEED * dt *
			    (-1 * (float)sdk_inputs.joy_y / 2048);
		player.has_moved = true;
	}

	if (sdk_inputs.joy_x > 500 || sdk_inputs.joy_x < -500) {
		if (player.strafing) {
			player_dx += cosf(player.angle + M_PI / 2) * MOVE_SPEED * dt *
				     ((float)sdk_inputs.joy_x / 2048);
			player_dy += sinf(player.angle + M_PI / 2) * MOVE_SPEED * dt *
				     ((float)sdk_inputs.joy_x / 2048);
			player.has_moved = true;
		}
	}

	// Footstep sounds
	if (player.has_moved) {
		footstep_timer += dt;
		if (footstep_timer > 0.4f) {
			sdk_melody_play("/i:noise (p) c");
			footstep_timer = 0;
		}
	}

	player.strafing = sdk_inputs.x ? true : false;

	if (sdk_inputs_delta.a == 1 && sdk_inputs.start) {
		if (mode.debug) {
			mode.debug = false;
		} else
			mode.debug = true;
	}
	if (sdk_inputs_delta.y == 1) {
		player.gun += 1;
		if (player.gun >= N_GUNS) {
			player.gun = 0;
		}
		if (player.gun == SHOTGUN && !player.shotgun_unlocked) {
			player.gun = BROKE;
		}
	}
	if (sdk_inputs_delta.select == 1) {
		game_state = GAME_MENU;
	}

	if (sdk_inputs.joy_x > 500 || sdk_inputs.joy_x < -500) {
		if (!player.strafing) {
			player.angle += ROTATE_SPEED * dt * ((float)sdk_inputs.joy_x / 2048);
		}
	}

	handleShooting();

	float player_fx = player.x + player_dx;
	float player_fy = player.y + player_dy;

	int player_tile_fx = player_fx / TILE_SIZE;
	int player_tile_fy = player_fy / TILE_SIZE;

	// Collision with walls
	if (isWall(player_tile_fx, player_tile_fy)) {
		return;
	}

	// Check for pickups
	if (isPickup(player_tile_fx, player_tile_fy)) {
		handlePickup(player_tile_fx, player_tile_fy);
	}

	player.x = player_fx;
	player.y = player_fy;

	int player_tile_x = player.x / TILE_SIZE;
	int player_tile_y = player.y / TILE_SIZE;

	// Teleport
	if (map.type[player_tile_y][player_tile_x] == TELEPORT) {
		map_starter_caller();
	}

	// Update enemies
	updateEnemies(dt);

	// Low ammo warning
	if (player.ammo <= 3 && player.ammo > 0) {
		static float warning_timer = 0;
		warning_timer += dt;
		if (warning_timer > 2.0f) {
			tft_draw_string(30, 20, RED, "Low Ammo");
			warning_timer = 0;
		}
	}
}

static void player_view_draw()
{
	if (mode.debug) {
		return;
	}

	sdk_draw_tile(0, 0, &ts_overline_png, 1);
	tft_draw_string(22, 105, DRAW_RED, "%-i", player.score);
	tft_draw_string(70, 105, DRAW_RED, "%-i", player.health);
	tft_draw_string(105, 105, DRAW_RED, "%-i", player.ammo);

	switch (player.gun) {
	case PISTOL:
		sdk_draw_tile(75, 65, &ts_pistol_png, 1);
		break;
	case SHOTGUN:
		if (player.shotgun_unlocked) {
			sdk_draw_tile(55, 49, &ts_shotgun_png, 1);
		}
		break;
	case BROKE:
		sdk_draw_tile(55, 49, &ts_chainsaw_png, 1);
		break;
	default:
		break;
	}

	if (muzzle_flash.visible) {
		uint32_t current_time = time_us_64();
		if (current_time - muzzle_flash.timer_start < MUZZLE_FLASH_DURATION_MS * 1000) {
			for (int i = 0; i < 10; i++) {
				int fx = 80 + (rand() % 20) - 10;
				int fy = 50 + (rand() % 20) - 10;
				tft_draw_pixel(fx, fy, YELLOW);
			}
		} else {
			muzzle_flash.visible = false;
		}
	}

	// Bullet hit marker - show different colors for enemy vs wall hits
	if (bullet.hit_visible) {
		uint32_t current_time = time_us_64();
		if (current_time - bullet.hit_timer_start < BULLET_VISUAL_DURATION_MS * 1000) {
			// Draw hit marker at crosshair for enemy hits
			if (bullet.hit_screen_x == SCREEN_WIDTH / 2 &&
			    bullet.hit_screen_y == SCREEN_HEIGHT / 2) {
				// Enemy hit - draw red cross
				for (int i = -2; i <= 2; i++) {
					tft_draw_pixel(SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT / 2,
						       RED);
					tft_draw_pixel(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + i,
						       RED);
				}
			} else {
				// Wall hit - draw at hit location
				for (int dy = -bullet.hit_size / 2; dy <= bullet.hit_size / 2;
				     dy++) {
					for (int dx = -bullet.hit_size / 2;
					     dx <= bullet.hit_size / 2; dx++) {
						int draw_x = bullet.hit_screen_x + dx;
						int draw_y = bullet.hit_screen_y + dy;
						if (draw_x >= 0 && draw_x < SCREEN_WIDTH &&
						    draw_y >= 0 && draw_y < SCREEN_HEIGHT) {
							tft_draw_pixel(draw_x, draw_y,
								       bullet.hit_color);
						}
					}
				}
			}
		} else {
			bullet.hit_visible = false;
		}
	}

	// Pickup message display
	if (pickup_msg.visible) {
		uint32_t current_time = time_us_64();
		if (current_time - pickup_msg.timer_start < 2000000) { // 2 seconds
			tft_draw_string(SCREEN_WIDTH / 2 - 40, 20, GREEN, pickup_msg.message);
		} else {
			pickup_msg.visible = false;
		}
	}

	// Low ammo warning
	if (player.ammo <= 3 && player.ammo > 0) {
		tft_draw_string(50, 20, RED, "Low Ammo");
	}
}

static void handleShooting()
{
	if (sdk_inputs_delta.a == 1) {
		if (player.gun == SHOTGUN && !player.shotgun_unlocked) {
			sdk_melody_play("/i:noise (p) c");
			return;
		}
		if (player.gun == BROKE) {
			float shoot_angle = player.angle;
			uint16_t visual_color = RED;
			float current_gun_range = 0.0f;
			int visual_size = 0;
			current_gun_range = CHAINSAW_MAX_RANGE_TILES;
			visual_size = 3;
			muzzle_flash.visible = true;
			muzzle_flash.timer_start = time_us_64();

			shootBullet(shoot_angle, current_gun_range, visual_size, visual_color);
			return;
		}
		if (player.ammo > 0) {
			uint16_t visual_color = YELLOW;
			float current_gun_range = 0.0f;
			int visual_size = 8;

			if (player.gun == PISTOL) {
				current_gun_range = PISTOL_MAX_RANGE_TILES;
				sdk_melody_play("/i:square (ff) e_");
				visual_size = 6;
				// Single bullet for pistol
				shootBullet(player.angle, current_gun_range, visual_size,
					    visual_color);
			} else if (player.gun == SHOTGUN) {
				current_gun_range = SHOTGUN_MAX_RANGE_TILES;
				sdk_melody_play("/i:noise (fff) < c_");
				visual_size = 8;

				// Shotgun spread - multiple pellets with wider spread
				int pellet_count = 8; // Increased from 5 to 8
				float spread_angle = SHOTGUN_SPREAD_DEGREES * M_PI / 180.0f;

				for (int i = 0; i < pellet_count; i++) {
					// Wider spread for shotgun
					float spread = ((float)rand() / RAND_MAX - 0.5f) *
						       spread_angle * 2.0f;
					float pellet_angle = player.angle + spread;
					shootBullet(pellet_angle, current_gun_range,
						    visual_size / 2, visual_color);
				}
			}

			muzzle_flash.visible = true;
			muzzle_flash.timer_start = time_us_64();
			player.ammo--;
		} else {
			sdk_melody_play("/i:noise (p) c");
			if (mode.debug) {
				printf("Out of ammo!\n");
			}
		}
	}
}

static void shootBullet(float current_shoot_angle, float max_range_tiles, int visual_size,
			uint16_t visual_color)
{
	float rayDirX = cosf(current_shoot_angle);
	float rayDirY = sinf(current_shoot_angle);

	float max_dist = max_range_tiles * TILE_SIZE;

	int damage_to_deal = 0;
	if (player.gun == PISTOL) {
		damage_to_deal = PISTOL_DAMAGE;
	} else if (player.gun == SHOTGUN) {
		damage_to_deal = SHOTGUN_DAMAGE;
	} else if (player.gun == BROKE) {
		damage_to_deal = CHAINSAW_DAMAGE;
	}

	// Track closest hit enemy by INDEX, not pointer
	int hit_enemy_index = -1;
	float closest_enemy_dist = max_dist + 1.0f;

	// Check for enemy hits first (before walls)
	for (int i = 0; i < MAX_ENEMIES; i++) {
		// Skip dead or inactive enemies
		if (!enemies[i].alive || !enemies[i].active)
			continue;

		// Calculate vector from player to enemy
		float dx = enemies[i].x - player.x;
		float dy = enemies[i].y - player.y;

		// Calculate distance to enemy
		float dist_to_enemy = sqrtf(dx * dx + dy * dy);

		// Skip if enemy is too far
		if (dist_to_enemy > max_dist)
			continue;

		// Calculate angle to enemy
		float angle_to_enemy = atan2f(dy, dx);
		float angle_diff = fabsf(angle_to_enemy - current_shoot_angle);

		// Normalize angle difference to be between -PI and PI
		if (angle_diff > M_PI) {
			angle_diff = 2 * M_PI - angle_diff;
		}

		// Calculate the maximum angle tolerance based on distance
		// Closer enemies have larger tolerance (easier to hit)
		float angle_tolerance = atanf(TILE_SIZE * 0.3f / dist_to_enemy);

		// Check if enemy is within the bullet's cone
		if (fabsf(angle_diff) < angle_tolerance) {
			if (dist_to_enemy < closest_enemy_dist) {
				hit_enemy_index = i;
				closest_enemy_dist = dist_to_enemy;
			}
		}
	}

	// Now check for walls along the bullet path
	float rayX = player.x;
	float rayY = player.y;
	float wall_hit_dist = max_dist;

	for (float dist = 0; dist < max_dist; dist += TILE_SIZE / 8.0f) {
		rayX = player.x + rayDirX * dist;
		rayY = player.y + rayDirY * dist;

		int current_tile_x = (int)(rayX / TILE_SIZE);
		int current_tile_y = (int)(rayY / TILE_SIZE);

		if (isWall(current_tile_x, current_tile_y)) {
			wall_hit_dist = dist;

			// Show wall hit effect
			float perpWallDist = dist;
			if (perpWallDist == 0.0f)
				perpWallDist = 0.001f;

			float relativeAngle = current_shoot_angle - player.angle;
			if (relativeAngle > M_PI)
				relativeAngle -= 2 * M_PI;
			if (relativeAngle < -M_PI)
				relativeAngle += 2 * M_PI;

			int screen_x = (int)(SCREEN_WIDTH / 2.0f +
					     PROJECTION_PLANE_DISTANCE * tanf(relativeAngle));
			float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);
			int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
			int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

			bullet.hit_visible = true;
			bullet.hit_timer_start = time_us_64();
			bullet.hit_screen_x = screen_x;
			bullet.hit_screen_y = (drawStart + drawEnd) / 2;
			bullet.hit_size = visual_size;
			bullet.hit_color = visual_color;

			if (mode.debug)
				printf("Bullet hit wall (%d, %d), dist %.2f\n", current_tile_x,
				       current_tile_y, dist / TILE_SIZE);

			break;
		}
	}

	// If we hit an enemy and no wall was in the way (or enemy is closer than wall)
	if (hit_enemy_index != -1 && closest_enemy_dist <= wall_hit_dist) {
		// Double-check that enemy is still alive (in case of multiple pellets hitting same enemy)
		if (!enemies[hit_enemy_index].alive || !enemies[hit_enemy_index].active) {
			if (mode.debug) {
				printf("Enemy %d was already dead, ignoring hit\n",
				       hit_enemy_index);
			}
			return;
		}

		int old_health = enemies[hit_enemy_index].health;
		enemies[hit_enemy_index].health -= damage_to_deal;
		enemies[hit_enemy_index].last_hit_time = time_us_64();

		// Show hit marker at center (enemy hit)
		bullet.hit_visible = true;
		bullet.hit_timer_start = time_us_64();
		bullet.hit_screen_x = SCREEN_WIDTH / 2;
		bullet.hit_screen_y = SCREEN_HEIGHT / 2;
		bullet.hit_size = visual_size;
		bullet.hit_color = RED; // Red hit marker for enemy hit

		if (mode.debug) {
			printf("Enemy %d hit! Enemy health: %d -> %d (Damage: %d)\n",
			       hit_enemy_index, old_health, enemies[hit_enemy_index].health,
			       damage_to_deal);
		}

		// Set hurt state
		enemies[hit_enemy_index].state = ENEMY_HURT;
		enemies[hit_enemy_index].hurt_timer = 0.3f; // Hurt state duration

		// Check if enemy should die
		if (enemies[hit_enemy_index].health <= 0) {
			enemies[hit_enemy_index].state = ENEMY_DEAD;
			enemies[hit_enemy_index].alive = false;
			enemies[hit_enemy_index].active = false;
			player.score += (enemies[hit_enemy_index].type == 1) ? 100 : 200;
			active_enemy_count--;

			int random = randomnumb(10);
			;
			if (mode.debug)
				printf("dead random number %i\n", random);
			switch (random) {
			case 3:
				give_pickup(HEALTH_PACK);
				break;
			case 2:
				give_pickup(AMMO_BOX);
				break;
			case 1:
				give_pickup(SHOTGUN_PICKUP);
				break;
			default:
				break;
			}
			if (mode.debug) {
				printf("Enemy %d killed immediately! Score: %d, Active enemies: %d\n",
				       hit_enemy_index, player.score, active_enemy_count);
			}
		}
	}
}

static void initEnemies()
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		enemies[i].alive = false;
		enemies[i].active = false;
		enemies[i].health = 0;
		enemies[i].attack_timer = 0;
		enemies[i].state = ENEMY_IDLE;
		enemies[i].state_timer = 0;
		enemies[i].patrol_angle = 0;
		enemies[i].patrol_timer = 0;
		enemies[i].chase_timer = 0;
		enemies[i].hurt_timer = 0;
	}
	active_enemy_count = 0;
}

static void spawnEnemy(float x, float y, int health)
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (!enemies[i].alive && !enemies[i].active) {
			enemies[i].x = x;
			enemies[i].y = y;
			enemies[i].health = health;
			enemies[i].max_health = health;
			enemies[i].alive = true;
			enemies[i].active = true;
			enemies[i].attack_timer = 0;
			enemies[i].last_attack_time = 0;
			enemies[i].last_hit_time = 0;
			enemies[i].state = ENEMY_PATROL;
			enemies[i].state_timer = 0;
			enemies[i].patrol_angle = (float)(rand() % 360) * M_PI / 180.0f;
			enemies[i].patrol_timer = 0;
			enemies[i].chase_timer = 0;
			enemies[i].hurt_timer = 0;

			// Determine enemy type based on health
			if (health > ENEMY_HEALTH) {
				enemies[i].type = 2; // ENEMY2
			} else {
				enemies[i].type = 1; // ENEMY1
			}

			active_enemy_count++;
			break;
		}
	}
}

static bool canSeePlayer(Enemy *enemy)
{
	// Simple line-of-sight check
	float dx = player.x - enemy->x;
	float dy = player.y - enemy->y;
	float distance = sqrtf(dx * dx + dy * dy);

	if (distance > ENEMY_DETECTION_RANGE) {
		return false;
	}

	// Normalize direction
	if (distance > 0.1f) {
		dx /= distance;
		dy /= distance;
	}

	// Check for walls along the line
	float checkX = enemy->x;
	float checkY = enemy->y;
	float step = TILE_SIZE / 4.0f;

	for (float dist = 0; dist < distance; dist += step) {
		checkX = enemy->x + dx * dist;
		checkY = enemy->y + dy * dist;

		int tile_x = (int)(checkX / TILE_SIZE);
		int tile_y = (int)(checkY / TILE_SIZE);

		if (isWall(tile_x, tile_y)) {
			return false;
		}
	}

	return true;
}

static void updateEnemies(float dt)
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		// Skip enemies that are already dead and inactive
		if (!enemies[i].alive || !enemies[i].active)
			continue;

		// Check if enemy should die from damage (do this FIRST)
		if (enemies[i].health <= 0 && enemies[i].state != ENEMY_DEAD) {
			// Mark enemy as dead
			enemies[i].state = ENEMY_DEAD;
			enemies[i].alive = false;
			enemies[i].active = false;

			// Add score only once when enemy dies
			player.score += (enemies[i].type == 1) ? 100 : 200;
			active_enemy_count--;

			if (mode.debug) {
				printf("Enemy %d died! Score: %d, Active enemies: %d\n", i,
				       player.score, active_enemy_count);
			}
			continue; // Skip further processing for dead enemies
		}

		// Calculate distance to player
		float dx = player.x - enemies[i].x;
		float dy = player.y - enemies[i].y;
		float distance = sqrtf(dx * dx + dy * dy);

		// Update state timers
		enemies[i].state_timer += dt;
		enemies[i].attack_timer -= dt;
		enemies[i].hurt_timer -= dt;

		// State machine for living enemies
		switch (enemies[i].state) {
		case ENEMY_IDLE:
			// Wait for a bit then go to patrol
			if (enemies[i].state_timer > 2.0f) {
				enemies[i].state = ENEMY_PATROL;
				enemies[i].state_timer = 0;
				enemies[i].patrol_angle = (float)(rand() % 360) * M_PI / 180.0f;
			}
			// Check if player is visible
			if (canSeePlayer(&enemies[i])) {
				enemies[i].state = ENEMY_CHASE;
				enemies[i].state_timer = 0;
			}
			break;

		case ENEMY_PATROL:
			enemies[i].patrol_timer += dt;

			// Change direction occasionally
			if (enemies[i].patrol_timer > 3.0f) {
				enemies[i].patrol_angle +=
					((float)(rand() % 100) / 100.0f - 0.5f) * M_PI;
				enemies[i].patrol_timer = 0;
			}

			// Move in patrol direction
			float patrol_dx = cosf(enemies[i].patrol_angle) * ENEMY_SPEED * 0.3f * dt;
			float patrol_dy = sinf(enemies[i].patrol_angle) * ENEMY_SPEED * 0.3f * dt;

			float new_x = enemies[i].x + patrol_dx;
			float new_y = enemies[i].y + patrol_dy;

			// Check collision with walls
			int new_tile_x = (int)(new_x / TILE_SIZE);
			int new_tile_y = (int)(new_y / TILE_SIZE);

			if (!isWall(new_tile_x, new_tile_y)) {
				enemies[i].x = new_x;
				enemies[i].y = new_y;
			} else {
				// Hit a wall, change direction
				enemies[i].patrol_angle +=
					M_PI * 0.5f +
					((float)(rand() % 100) / 100.0f - 0.5f) * M_PI;
			}

			// Check if player is visible
			if (canSeePlayer(&enemies[i])) {
				enemies[i].state = ENEMY_CHASE;
				enemies[i].state_timer = 0;
			}
			break;

		case ENEMY_CHASE:
			enemies[i].chase_timer += dt;

			// Give up chasing after a while if can't see player
			if (enemies[i].chase_timer > 5.0f && !canSeePlayer(&enemies[i])) {
				enemies[i].state = ENEMY_PATROL;
				enemies[i].state_timer = 0;
				enemies[i].chase_timer = 0;
				break;
			}

			// Move towards player
			if (distance > 0.1f) {
				dx /= distance;
				dy /= distance;
			}

			float chase_speed = (enemies[i].type == 2) ? ENEMY_SPEED * 1.2f :
								     ENEMY_SPEED;
			float chase_x = enemies[i].x + dx * chase_speed * dt;
			float chase_y = enemies[i].y + dy * chase_speed * dt;

			// Check collision with walls
			int chase_tile_x = (int)(chase_x / TILE_SIZE);
			int chase_tile_y = (int)(chase_y / TILE_SIZE);

			if (!isWall(chase_tile_x, chase_tile_y)) {
				enemies[i].x = chase_x;
				enemies[i].y = chase_y;
			}

			// Attack if close enough
			if (distance <= ENEMY_ATTACK_RANGE) {
				enemies[i].state = ENEMY_ATTACK;
				enemies[i].state_timer = 0;
			}
			break;

		case ENEMY_ATTACK:
			// Attack the player
			if (enemies[i].attack_timer <= 0) {
				player.health -= (enemies[i].type == 2) ? ENEMY_DAMAGE * 2 :
									  ENEMY_DAMAGE;
				enemies[i].attack_timer = ENEMY_ATTACK_COOLDOWN;
				enemies[i].last_attack_time = time_us_64();

				// Play attack sound
				sdk_melody_play("/i:noise (ff) c");

				if (mode.debug) {
					printf("Enemy %d attacked! Player health: %d\n", i,
					       player.health);
				}
			}

			// If player moves away, chase again
			if (distance > ENEMY_ATTACK_RANGE * 1.5f) {
				enemies[i].state = ENEMY_CHASE;
				enemies[i].state_timer = 0;
			}
			break;

		case ENEMY_HURT:
			// Flash and maybe retreat
			if (enemies[i].hurt_timer <= 0) {
				if (distance < ENEMY_ATTACK_RANGE * 0.7f) {
					// Move away from player
					float retreat_x =
						enemies[i].x - dx * ENEMY_SPEED * 0.5f * dt;
					float retreat_y =
						enemies[i].y - dy * ENEMY_SPEED * 0.5f * dt;

					int retreat_tile_x = (int)(retreat_x / TILE_SIZE);
					int retreat_tile_y = (int)(retreat_y / TILE_SIZE);

					if (!isWall(retreat_tile_x, retreat_tile_y)) {
						enemies[i].x = retreat_x;
						enemies[i].y = retreat_y;
					}
				}

				// Go back to chasing
				enemies[i].state = ENEMY_CHASE;
				enemies[i].state_timer = 0;
			}
			break;

		case ENEMY_DEAD:
			// This should never be reached due to the continue above
			break;
		default:
			break;
		}
	}
}

static void renderEnemies()
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		// Skip dead or inactive enemies completely
		if (!enemies[i].alive || !enemies[i].active)
			continue;

		// Calculate relative position of enemy to player
		float relX = enemies[i].x - player.x;
		float relY = enemies[i].y - player.y;

		float distToEnemy = sqrtf(relX * relX + relY * relY);

		// Skip if behind player or too close
		if (relX * cosf(player.angle) + relY * sinf(player.angle) < 0.1f ||
		    distToEnemy < 0.1f) {
			continue;
		}

		// Calculate angle from player view to enemy
		float angleToEnemy = atan2f(relY, relX);
		float angleDiff = angleToEnemy - player.angle;

		// Normalize angleDiff to be between -PI and PI
		if (angleDiff > M_PI) {
			angleDiff -= 2 * M_PI;
		}
		if (angleDiff < -M_PI) {
			angleDiff += 2 * M_PI;
		}

		// Check if enemy is within FOV
		if (fabsf(angleDiff) < FOV_RADIANS / 2.0f) {
			// screen X position for the center of the enemy sprite
			int screen_x = (int)(SCREEN_WIDTH / 2.0f +
					     PROJECTION_PLANE_DISTANCE * tanf(angleDiff));

			float projectedSize = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / distToEnemy);
			int enemy_draw_size = (int)projectedSize;

			// Clamp min and max
			if (enemy_draw_size < 4)
				enemy_draw_size = 4;
			if (enemy_draw_size > SCREEN_HEIGHT * 2)
				enemy_draw_size = SCREEN_HEIGHT * 2;

			// top-left corner
			int draw_x_start = screen_x - enemy_draw_size / 2;
			int draw_y_start = (int)((SCREEN_HEIGHT / 2.0f) - (enemy_draw_size / 2.0f));

			// Clamp bounds to screen
			int x_loop_start = (draw_x_start < 0) ? 0 : draw_x_start;
			int x_loop_end = (draw_x_start + enemy_draw_size > SCREEN_WIDTH) ?
						 SCREEN_WIDTH :
						 draw_x_start + enemy_draw_size;
			int y_loop_start = (draw_y_start < 0) ? 0 : draw_y_start;
			int y_loop_end = (draw_y_start + enemy_draw_size > SCREEN_HEIGHT) ?
						 SCREEN_HEIGHT :
						 draw_y_start + enemy_draw_size;

			// Choose texture based on enemy type
			int texIndex;
			if (enemies[i].type == 1) {
				texIndex = TEX_ENEMY1; // TTE_00 for ENEMY1
			} else {
				texIndex = TEX_ENEMY2; // TTE_01 for ENEMY2
			}

			int texWidth = Textures[texIndex].w;
			int texHeight = Textures[texIndex].h;

			// Calculate shading based on distance
			fixed_t perpEnemyDist_fixed = float_to_fixed(distToEnemy);
			fixed_t shade =
				FIXED_SCALE - fixed_div(perpEnemyDist_fixed, fixed_max_dist);
			if (shade < fixed_min_shade) {
				shade = fixed_min_shade;
			}
			int shade_int = (shade >> 8) & 0xFFFF;

			// Apply red tint if enemy is hurt
			bool is_hurt = enemies[i].state == ENEMY_HURT;

			// Draw the enemy texture with depth testing and transparency
			fixed_t step =
				(fixed_t)(((int64_t)texHeight << FIXED_SHIFT) / enemy_draw_size);

			for (int x_pixel = x_loop_start; x_pixel < x_loop_end; x_pixel++) {
				// Only draw if enemy is closer than wall at this x position
				if (distToEnemy < zBuffer[x_pixel]) {
					int tex_x = ((x_pixel - draw_x_start) * texWidth) /
						    enemy_draw_size;
					if (tex_x < 0)
						tex_x = 0;
					if (tex_x >= texWidth)
						tex_x = texWidth - 1;

					fixed_t tex_pos =
						(fixed_t)((int64_t)(y_loop_start - draw_y_start) *
							  step);

					draw_sprite_column_asm(x_pixel, y_loop_start,
							       y_loop_end - 1, tex_pos, step,
							       Textures[texIndex].name, texWidth,
							       texHeight, tex_x, shade_int,
							       is_hurt ? 1 : 0);
				}
			}

			// Draw health bar if damaged
			if (enemies[i].health < enemies[i].max_health) {
				int health_width = (enemy_draw_size * enemies[i].health) /
						   enemies[i].max_health;
				int bar_y = draw_y_start - 3;
				if (bar_y >= 0 && health_width > 2) {
					// Health bar background (missing health)
					for (int x = x_loop_start;
					     x < x_loop_start + enemy_draw_size && x < SCREEN_WIDTH;
					     x++) {
						if (x >= 0) {
							tft_draw_pixel(x, bar_y, RED);
						}
					}
					// Health bar foreground (current health)
					for (int x = x_loop_start;
					     x < x_loop_start + health_width && x < SCREEN_WIDTH;
					     x++) {
						if (x >= 0) {
							tft_draw_pixel(x, bar_y, GREEN);
						}
					}
				}
			}
		}
	}
}

static void renderPickups()
{
	// Define transparent color (magenta: RGB 255, 60, 255)

	for (int i = 0; i < active_pickup_count; i++) {
		if (!pickups[i].active || pickups[i].collected)
			continue;

		// Calculate relative position to player
		float relX = pickups[i].x - player.x;
		float relY = pickups[i].y - player.y;

		float distToPickup = sqrtf(relX * relX + relY * relY);

		// Skip if behind player or too close
		if (relX * cosf(player.angle) + relY * sinf(player.angle) < 0.1f ||
		    distToPickup < 0.1f) {
			continue;
		}

		// Calculate angle from player view to pickup
		float angleToPickup = atan2f(relY, relX);
		float angleDiff = angleToPickup - player.angle;

		// Normalize angleDiff
		if (angleDiff > M_PI)
			angleDiff -= 2 * M_PI;
		if (angleDiff < -M_PI)
			angleDiff += 2 * M_PI;

		// Check if pickup is within FOV
		if (fabsf(angleDiff) < FOV_RADIANS / 2.0f) {
			// Screen X position for the center of the pickup sprite
			int screen_x = (int)(SCREEN_WIDTH / 2.0f +
					     PROJECTION_PLANE_DISTANCE * tanf(angleDiff));

			float projectedSize =
				(TILE_SIZE * 0.5f * PROJECTION_PLANE_DISTANCE / distToPickup);
			int pickup_draw_size = (int)projectedSize;

			// Clamp size
			if (pickup_draw_size < 8)
				pickup_draw_size = 8;
			if (pickup_draw_size > SCREEN_HEIGHT)
				pickup_draw_size = SCREEN_HEIGHT;

			// Top-left corner
			int draw_x_start = screen_x - pickup_draw_size / 2;
			int draw_y_start =
				(int)((SCREEN_HEIGHT / 2.0f) - (pickup_draw_size / 2.0f));

			// Clamp bounds to screen
			int x_loop_start = (draw_x_start < 0) ? 0 : draw_x_start;
			int x_loop_end = (draw_x_start + pickup_draw_size > SCREEN_WIDTH) ?
						 SCREEN_WIDTH :
						 draw_x_start + pickup_draw_size;
			int y_loop_start = (draw_y_start < 0) ? 0 : draw_y_start;
			int y_loop_end = (draw_y_start + pickup_draw_size > SCREEN_HEIGHT) ?
						 SCREEN_HEIGHT :
						 draw_y_start + pickup_draw_size;

			// Choose texture based on pickup type
			int texIndex;
			switch (pickups[i].type) {
			case HEALTH_PACK:
				texIndex = TEX_HEALTH_PICKUP; // TTO_00
				break;
			case AMMO_BOX:
				texIndex = TEX_AMMO_PICKUP; // TTO_01
				break;
			case SHOTGUN_PICKUP:
				texIndex = TEX_SHOTGUN_PICKUP; // TTO_02
				break;
			default:
				continue;
			}

			int texWidth = Textures[texIndex].w;
			int texHeight = Textures[texIndex].h;

			// Calculate shading based on distance
			fixed_t perpPickupDist_fixed = float_to_fixed(distToPickup);
			fixed_t shade =
				FIXED_SCALE - fixed_div(perpPickupDist_fixed, fixed_max_dist);
			if (shade < fixed_min_shade)
				shade = fixed_min_shade;
			int shade_int = (shade >> 8) & 0xFFFF;

			// Draw the pickup texture with depth testing and transparency
			fixed_t step =
				(fixed_t)(((int64_t)texHeight << FIXED_SHIFT) / pickup_draw_size);

			for (int x_pixel = x_loop_start; x_pixel < x_loop_end; x_pixel++) {
				// Only draw if pickup is closer than wall at this x position
				if (distToPickup < zBuffer[x_pixel]) {
					int tex_x = ((x_pixel - draw_x_start) * texWidth) /
						    pickup_draw_size;
					if (tex_x < 0)
						tex_x = 0;
					if (tex_x >= texWidth)
						tex_x = texWidth - 1;

					fixed_t tex_pos =
						(fixed_t)((int64_t)(y_loop_start - draw_y_start) *
							  step);

					draw_sprite_column_asm(x_pixel, y_loop_start,
							       y_loop_end - 1, tex_pos, step,
							       Textures[texIndex].name, texWidth,
							       texHeight, tex_x, shade_int, 0);
				}
			}
		}
	}
}

static void real_game_start(void)
{
	Map_starter(&maps_map1);
	textures_load();

	player.health = 100;
	player.angle = (float)M_PI / 2.0f;
	player.ammo = 15;
	player.alive = true;
	player.gun = BROKE;
	player.shotgun_unlocked = false;
	player.has_moved = false;

	muzzle_flash.visible = false;
	muzzle_flash.timer_start = 0;
	pickup_msg.visible = false;
}

void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void Map_starter(const TileType (*next_map)[MAP_ROWS][MAP_COLS])
{
	load_map(next_map);
	player.angle = (float)M_PI / 2.0f;
	if (player.ammo < 15) {
		player.ammo = 15;
	}
	player.alive = true;

	bullet.hit_visible = false;
	bullet.hit_timer_start = 0;
	bullet.hit_screen_x = 0;
	bullet.hit_screen_y = 0;
	bullet.hit_size = 0;
	bullet.hit_color = 0;

	muzzle_flash.visible = false;
	muzzle_flash.timer_start = 0;
	pickup_msg.visible = false;

	// Clear collected pickups when starting/changing map
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			collected_pickups[y][x] = false;
		}
	}

	// Initialize enemies
	initEnemies();

	// Find player spawn and enemy spawns
	bool player_found = false;
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if ((*next_map)[y][x] == PLAYER_SPAWN && !player_found) {
				player.x = x * TILE_SIZE + TILE_SIZE / 2;
				player.y = y * TILE_SIZE + TILE_SIZE / 2;
				player_found = true;
				if (mode.debug) {
					printf("Player spawned at (%d, %d)\n", x, y);
				}
			} else if ((*next_map)[y][x] == ENEMY1 || (*next_map)[y][x] == ENEMY2) {
				// Spawn enemy at this position
				float enemy_x = x * TILE_SIZE + TILE_SIZE / 2;
				float enemy_y = y * TILE_SIZE + TILE_SIZE / 2;

				// Different enemy types based on tile
				int enemy_health = ENEMY_HEALTH; // ENEMY1: 60 health
				if ((*next_map)[y][x] == ENEMY2) {
					enemy_health = 180; // ENEMY2: 180 health (3 shotgun hits)
				}

				spawnEnemy(enemy_x, enemy_y, enemy_health);

				if (mode.debug) {
					printf("Enemy spawned at (%d, %d) type: %s, health: %d\n",
					       x, y,
					       (*next_map)[y][x] == ENEMY1 ? "ENEMY1" : "ENEMY2",
					       enemy_health);
				}
			}
		}
	}

	if (!player_found) {
		// Default spawn if no player spawn found
		player.x = TILE_SIZE + TILE_SIZE / 2;
		player.y = TILE_SIZE + TILE_SIZE / 2;
		if (mode.debug) {
			printf("No player spawn found, using default\n");
		}
	}

	if (mode.debug) {
		printf("Total enemies spawned: %d\n", active_enemy_count);
	}
	active_pickup_count = 0;
	for (int i = 0; i < MAX_PICKUPS; i++) {
		pickups[i].active = false;
		pickups[i].collected = false;
	}

	// Find and spawn pickups from the map
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			TileType Ntile = (*next_map)[y][x];
			if (Ntile == HEALTH_PACK || Ntile == AMMO_BOX || Ntile == SHOTGUN_PICKUP) {
				if (active_pickup_count < MAX_PICKUPS) {
					pickups[active_pickup_count].x =
						x * TILE_SIZE + TILE_SIZE / 2;
					pickups[active_pickup_count].y =
						y * TILE_SIZE + TILE_SIZE / 2;
					pickups[active_pickup_count].type = Ntile;
					pickups[active_pickup_count].collected =
						collected_pickups[y][x];
					pickups[active_pickup_count].active =
						!collected_pickups[y][x];
					active_pickup_count++;
				}
			}
		}
	}

	if (mode.debug) {
		printf("Total pickups spawned: %d\n", active_pickup_count);
	}
}

static void menu_inputs()
{
	if (sdk_inputs_delta.start == 1) {
		switch (menu.pointer) {
		case 0:
			real_game_start();
			game_state = GAME_PLAYING;
			mode.map_type = CAMPAIGN;
			break;
		case 1:
			game_state = GAME_PLAYING;
			mode.map_type = RANDOM;
			break;
		default:
			break;
		}
	}
	menu.pointer = clamp(menu.pointer += sdk_inputs_delta.vertical, 0, 1);
}

static void dead_state_inputs()
{
	if (sdk_inputs_delta.start == 1) {
		real_game_start();
		game_state = GAME_PLAYING;
	} else if (sdk_inputs_delta.select == 1) {
		game_state = GAME_MENU;
	}
}

void game_input(unsigned dt_usec)
{
	seed += 1;
	float dt = dt_usec / 1000000.0f;

	switch (game_state) {
	case GAME_MENU:
		menu_inputs();
		break;
	case GAME_PLAYING:
		handlePlayerMovement(dt);
		if (player.health <= 0) {
			game_state = GAME_DEAD;
		}
		break;
	case GAME_DEAD:
		dead_state_inputs();
		break;
	default:
		break;
	}
	game_handle_audio(dt, volume);
}

static void debug(void)
{
	tft_draw_string(5, 0, WHITE, "X %-.2f Y %-.2f", player.x, player.y);
	tft_draw_string(5, 10, WHITE, "Angle %-.2f", player.angle);
	tft_draw_string(5, 20, WHITE, "Gun: %i SG: %i", player.gun, player.shotgun_unlocked);
	tft_draw_string(5, 95, WHITE, "Health %i", player.health);
	tft_draw_string(5, 105, WHITE, "Ammo %-i", player.ammo);
	tft_draw_string(5, 85, WHITE, "Enemies: %d", active_enemy_count);

	// Show enemy health info for debugging
	for (int i = 0; i < MAX_ENEMIES && i < 3; i++) {
		if (enemies[i].alive && enemies[i].active) {
			tft_draw_string(90, 10 + i * 10, WHITE, "E%d: %d/%d", enemies[i].type,
					enemies[i].health, enemies[i].max_health);
		}
	}
}

static void render_menu(float dt)
{
	tft_draw_rect(48, 18, 115, 32, WHITE);
	tft_draw_string(50, 20, 0, "KRECDOOM");
	tft_draw_string(60, 40, WHITE, "Campaign");
	tft_draw_string(60, 51, WHITE, "Random");
	tft_draw_string(50, 40 + menu.pointer * 10 + menu.pointer, YELLOW, ">");
}

static void render_dead_state(void) // Removed unused dt parameter
{
	// Draw a semi-dark screen by drawing a semi-transparent black rectangle
	for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
		for (int x = 0; x < SCREEN_WIDTH; x += 2) {
			tft_draw_pixel(x, y, 0);
		}
	}

	// Draw game over message
	tft_draw_string(20, SCREEN_HEIGHT / 2 - 20, RED, "YOU DIED");
	tft_draw_string(20, SCREEN_HEIGHT / 2, WHITE, "Score: %d", player.score);
	tft_draw_string(20, SCREEN_HEIGHT / 2 + 20, WHITE, "START to restart");
	tft_draw_string(20, SCREEN_HEIGHT / 2 + 40, WHITE, "SELECT for menu");
}
void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	switch (game_state) {
	case GAME_MENU:
		tft_fill(0);
		render_menu(dt);
		break;

	case GAME_PLAYING:
		// Fast clear (fills the framebuffer, actual LCD update is handled by the SDK swap)
		tft_fill(GRAY);
		renderGame();
		if (mode.debug) {
			debug();
		}
		player_view_draw();
		break;

	case GAME_DEAD:
		tft_fill(GRAY);
		renderGame(); // Still render the game world in background
		render_dead_state();
		break;
	default:
		break;
	}
}

static void textures_load()
{
	for (int i = 0; i < 19; i++) {
		Textures[i].name = (const unsigned char *)texture_names[i];
		Textures[i].h = texture_heights[i];
		Textures[i].w = texture_widths[i];
	}

	// Load enemy textures - using the exact array names from your headers
	Textures[TEX_ENEMY1].name = (const unsigned char *)TTE_00; // ENEMY1 texture
	Textures[TEX_ENEMY1].w = TTE_00_WIDTH;
	Textures[TEX_ENEMY1].h = TTE_00_HEIGHT;

	Textures[TEX_ENEMY2].name = (const unsigned char *)TTE_01; // ENEMY2 texture
	Textures[TEX_ENEMY2].w = TTE_01_WIDTH;
	Textures[TEX_ENEMY2].h = TTE_01_HEIGHT;

	Textures[TEX_HEALTH_PICKUP].name = (const unsigned char *)TTO_00; // Health pickup
	Textures[TEX_HEALTH_PICKUP].w = TTO_00_WIDTH;
	Textures[TEX_HEALTH_PICKUP].h = TTO_00_HEIGHT;

	Textures[TEX_AMMO_PICKUP].name = (const unsigned char *)TTO_01; // Ammo pickup
	Textures[TEX_AMMO_PICKUP].w = TTO_01_WIDTH;
	Textures[TEX_AMMO_PICKUP].h = TTO_01_HEIGHT;

	Textures[TEX_SHOTGUN_PICKUP].name = (const unsigned char *)TTO_02; // Shotgun pickup
	Textures[TEX_SHOTGUN_PICKUP].w = TTO_02_WIDTH;
	Textures[TEX_SHOTGUN_PICKUP].h = TTO_02_HEIGHT;
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = WHITE,
	};
	sdk_main(&config);
	return 0;
}
