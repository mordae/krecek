#include "common.h"
#include "extras/inludes.h"
#include "include_maps.h"
#include "maps.h"
#include "sdk/input.h"

//*
//*   TODO: RANDOM GENERATE
//*

typedef int32_t fixed_t;
static fixed_t fixed_tile_size;
static fixed_t fixed_max_dist;
static fixed_t fixed_min_shade;
static fixed_t fixed_side_shade;

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
	int type; // 1 for ENEMY1, 2 for ENEMY2
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

Gun gun_select[N_GUNS] = { 0, 1, 2 };
static Player player;
static Enemy enemies[MAX_ENEMIES];
static int active_enemy_count = 0;
static Mode mode;
static Bullet bullet;
static MuzzleFlash muzzle_flash;
static PickupMessage pickup_msg;
static MMap mmap;
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
static void Map_starter(const TileType map[MAP_ROWS][MAP_COLS]);
static void handleShooting();
static bool Can_shot(float dt);
static void map_starter_caller();
static void textures_load();
static void handlePickup(int tile_x, int tile_y);
extern void game_handle_audio(float dt, float volume);
const TileType (*currentMap)[MAP_COLS] = maps_map1;

static void renderPickups();

// Enemy functions
static void initEnemies();
static void spawnEnemy(float x, float y, int health);
static void updateEnemies(float dt);
static void renderEnemies();
static uint16_t rgb565_darken(uint16_t color, float factor)
{
	uint8_t r = (color >> 11) & 0x1F;
	uint8_t g = (color >> 5) & 0x3F;
	uint8_t b = color & 0x1F;

	r = (uint8_t)(r * factor);
	g = (uint8_t)(g * factor);
	b = (uint8_t)(b * factor);

	return (r << 11) | (g << 5) | b;
}

static void renderGame()
{
	// Draw floor and ceiling
	for (int y = 0; y < SCREEN_HEIGHT / 2; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			tft_draw_pixel(x, y, GRAY); // Ceiling
		}
	}
	for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			tft_draw_pixel(x, y, GRAY); // Floor
		}
	}

	float rayAngle;

	// Precompute fixed-point values if not already done
	if (fixed_tile_size == 0) {
		fixed_tile_size = float_to_fixed(TILE_SIZE);
		fixed_max_dist = float_to_fixed(15.0f * TILE_SIZE);
		fixed_min_shade = float_to_fixed(0.3f);
		fixed_side_shade = float_to_fixed(0.8f);
	}

	for (int x = 0; x < SCREEN_WIDTH; x++) {
		zBuffer[x] = 1e30f;
	}

	// going each vertical line
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		rayAngle = (player.angle - FOV_RADIANS / 2.0f) + (x * (FOV_RADIANS / SCREEN_WIDTH));

		int Tile_x = (int)(player.x / TILE_SIZE);
		int Tile_y = (int)(player.y / TILE_SIZE);

		float rayDirX = cosf(rayAngle);
		float rayDirY = sinf(rayAngle);

		float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX) * TILE_SIZE;
		float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY) * TILE_SIZE;

		float sideDistX;
		float sideDistY;

		mmap.hit = 0;

		if (rayDirX < 0.0f) {
			mmap.step_x = -1;
			sideDistX = (player.x - Tile_x * TILE_SIZE) * deltaDistX / TILE_SIZE;
		} else {
			mmap.step_x = 1;
			sideDistX = ((Tile_x + 1) * TILE_SIZE - player.x) * deltaDistX / TILE_SIZE;
		}

		if (rayDirY < 0.0f) {
			mmap.step_y = -1;
			sideDistY = (player.y - Tile_y * TILE_SIZE) * deltaDistY / TILE_SIZE;
		} else {
			mmap.step_y = 1;
			sideDistY = ((Tile_y + 1) * TILE_SIZE - player.y) * deltaDistY / TILE_SIZE;
		}

		while (mmap.hit == 0) {
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				Tile_x += mmap.step_x;
				mmap.side = 0;
			} else {
				sideDistY += deltaDistY;
				Tile_y += mmap.step_y;
				mmap.side = 1;
			}

			if (isWall(Tile_x, Tile_y)) {
				mmap.hit = 1;
			}
		}

		float perpWallDist;
		if (mmap.side == 0) {
			perpWallDist = (sideDistX - deltaDistX);
		} else {
			perpWallDist = (sideDistY - deltaDistY);
		}
		perpWallDist *= cosf(rayAngle - player.angle);

		if (perpWallDist < 0.001f)
			perpWallDist = 0.001f;

		// Store in zBuffer
		zBuffer[x] = perpWallDist;

		float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);

		int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
		int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		int wallType = currentMap[Tile_y][Tile_x] - 1;
		if (wallType <= 0)
			wallType = 0;

		float wallX;
		if (mmap.side == 0) {
			float rayDist = perpWallDist / cosf(rayAngle - player.angle);
			wallX = player.y + rayDist * rayDirY;
		} else {
			float rayDist = perpWallDist / cosf(rayAngle - player.angle);
			wallX = player.x + rayDist * rayDirX;
		}
		wallX /= TILE_SIZE;
		wallX -= floorf(wallX);

		int texWidth = Textures[wallType].w;
		int texHeight = Textures[wallType].h;

		int texX = (int)(wallX * (float)texWidth);
		if ((mmap.side == 0 && rayDirX > 0) || (mmap.side == 1 && rayDirY < 0)) {
			texX = texWidth - texX - 1;
		}

		float step = 1.0f * texHeight / wallHeight;
		float texPos = (drawStart - SCREEN_HEIGHT / 2.0f + wallHeight / 2.0f) * step;

		fixed_t perpWallDist_fixed = float_to_fixed(perpWallDist);

		fixed_t shade = FIXED_SCALE - fixed_div(perpWallDist_fixed, fixed_max_dist);

		if (shade < fixed_min_shade) {
			shade = fixed_min_shade;
		}

		if (mmap.side == 1) {
			shade = fixed_mul(shade, fixed_side_shade);
		}

		int shade_int = (shade >> 8) & 0xFFFF;

		for (int y = drawStart; y <= drawEnd; y++) {
			int texY = (int)texPos & (texHeight - 1);
			texPos += step;

			int pixelIndex = texY * 3 * texWidth + texX * 3;

			int r = Textures[wallType].name[pixelIndex + 0];
			int g = Textures[wallType].name[pixelIndex + 1];
			int b = Textures[wallType].name[pixelIndex + 2];

			r = (r * shade_int) >> 8;
			g = (g * shade_int) >> 8;
			b = (b * shade_int) >> 8;

			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;

			tft_draw_pixel(x, y, rgb_to_rgb565(r, g, b));
		}
	}

	// Draw enemies
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
	TileType tile = currentMap[Tile_y][Tile_x];
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
	TileType tile = currentMap[Tile_y][Tile_x];
	// Enemy tiles are not pickups
	return (tile == HEALTH_PACK || tile == AMMO_BOX || tile == SHOTGUN_PICKUP);
}

static void handlePickup(int tile_x, int tile_y)
{
	TileType tile = currentMap[tile_y][tile_x];
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
		}
	}
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
	if (sdk_inputs_delta.x == 1 && sdk_inputs.start) {
		if (currentMap == maps_map1) {
			currentMap = maps_map2;
			map_starter_caller();
		} else {
			currentMap = maps_map1;
			map_starter_caller();
		}
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
	if (currentMap[player_tile_y][player_tile_x] == TELEPORT) {
		if (currentMap == maps_map1) {
			map_starter_caller();
		} else {
			map_starter_caller();
		}
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
		}
	}
}

static void renderEnemies()
{
	// Define transparent color (magenta: RGB 255, 60, 255) - matches your texture files
	const int TRANS_R = 255;
	const int TRANS_G = 60;
	const int TRANS_B = 255;

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
			for (int y_pixel = y_loop_start; y_pixel < y_loop_end; y_pixel++) {
				for (int x_pixel = x_loop_start; x_pixel < x_loop_end; x_pixel++) {
					// Only draw if enemy is closer than wall at this x position
					if (distToEnemy < zBuffer[x_pixel]) {
						// Calculate texture coordinates
						float texX = (float)(x_pixel - draw_x_start) /
							     enemy_draw_size;
						float texY = (float)(y_pixel - draw_y_start) /
							     enemy_draw_size;

						int tex_x = (int)(texX * texWidth);
						int tex_y = (int)(texY * texHeight);

						// Ensure texture coordinates are within bounds
						if (tex_x < 0)
							tex_x = 0;
						if (tex_x >= texWidth)
							tex_x = texWidth - 1;
						if (tex_y < 0)
							tex_y = 0;
						if (tex_y >= texHeight)
							tex_y = texHeight - 1;

						// Get pixel from texture
						int pixelIndex = tex_y * 3 * texWidth + tex_x * 3;

						int r = Textures[texIndex].name[pixelIndex + 0];
						int g = Textures[texIndex].name[pixelIndex + 1];
						int b = Textures[texIndex].name[pixelIndex + 2];

						// Check for transparency (magenta: 255, 60, 255) - exact match from your files
						if (r == TRANS_R && g == TRANS_G && b == TRANS_B) {
							continue; // Skip transparent pixels
						}

						// Apply red tint if enemy is hurt
						if (is_hurt) {
							// Boost red, reduce green and blue for hurt effect
							r = (r + 100 > 255) ? 255 : r + 100;
							g = g / 2;
							b = b / 2;
						}

						// Apply distance shading
						r = (r * shade_int) >> 8;
						g = (g * shade_int) >> 8;
						b = (b * shade_int) >> 8;

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						tft_draw_pixel(x_pixel, y_pixel,
							       rgb_to_rgb565(r, g, b));
					}
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
	const int TRANS_R = 255;
	const int TRANS_G = 60;
	const int TRANS_B = 255;

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
			for (int y_pixel = y_loop_start; y_pixel < y_loop_end; y_pixel++) {
				for (int x_pixel = x_loop_start; x_pixel < x_loop_end; x_pixel++) {
					// Only draw if pickup is closer than wall at this x position
					if (distToPickup < zBuffer[x_pixel]) {
						// Calculate texture coordinates
						float texX = (float)(x_pixel - draw_x_start) /
							     pickup_draw_size;
						float texY = (float)(y_pixel - draw_y_start) /
							     pickup_draw_size;

						int tex_x = (int)(texX * texWidth);
						int tex_y = (int)(texY * texHeight);

						// Ensure texture coordinates are within bounds
						if (tex_x < 0)
							tex_x = 0;
						if (tex_x >= texWidth)
							tex_x = texWidth - 1;
						if (tex_y < 0)
							tex_y = 0;
						if (tex_y >= texHeight)
							tex_y = texHeight - 1;

						// Get pixel from texture
						int pixelIndex = tex_y * 3 * texWidth + tex_x * 3;

						int r = Textures[texIndex].name[pixelIndex + 0];
						int g = Textures[texIndex].name[pixelIndex + 1];
						int b = Textures[texIndex].name[pixelIndex + 2];

						// Check for transparency (magenta: 255, 60, 255)
						if (r == TRANS_R && g == TRANS_G && b == TRANS_B) {
							continue; // Skip transparent pixels
						}

						// Apply distance shading
						r = (r * shade_int) >> 8;
						g = (g * shade_int) >> 8;
						b = (b * shade_int) >> 8;

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						tft_draw_pixel(x_pixel, y_pixel,
							       rgb_to_rgb565(r, g, b));
					}
				}
			}
		}
	}
}

static void real_game_start(void)
{
	currentMap = maps_map1;
	Map_starter(maps_map1);
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

static void map_starter_caller()
{
	if (currentMap == maps_map2) {
		currentMap = maps_map3;
		Map_starter(maps_map3);
		return;
	}
	if (currentMap == maps_map1) {
		Map_starter(maps_map2);
		currentMap = maps_map2;
		return;
	}
	if (currentMap == maps_map3) {
		currentMap = maps_map1;
		Map_starter(maps_map1);
		return;
	}
}

static void Map_starter(const TileType map[MAP_ROWS][MAP_COLS])
{
	player.angle = (float)M_PI / 2.0f;
	player.ammo = 15;
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
			if (map[y][x] == PLAYER_SPAWN && !player_found) {
				player.x = x * TILE_SIZE + TILE_SIZE / 2;
				player.y = y * TILE_SIZE + TILE_SIZE / 2;
				player_found = true;
				if (mode.debug) {
					printf("Player spawned at (%d, %d)\n", x, y);
				}
			} else if (map[y][x] == ENEMY1 || map[y][x] == ENEMY2) {
				// Spawn enemy at this position
				float enemy_x = x * TILE_SIZE + TILE_SIZE / 2;
				float enemy_y = y * TILE_SIZE + TILE_SIZE / 2;

				// Different enemy types based on tile
				int enemy_health = ENEMY_HEALTH; // ENEMY1: 60 health
				if (map[y][x] == ENEMY2) {
					enemy_health = 180; // ENEMY2: 180 health (3 shotgun hits)
				}

				spawnEnemy(enemy_x, enemy_y, enemy_health);

				if (mode.debug) {
					printf("Enemy spawned at (%d, %d) type: %s, health: %d\n",
					       x, y, map[y][x] == ENEMY1 ? "ENEMY1" : "ENEMY2",
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
			TileType tile = map[y][x];
			if (tile == HEALTH_PACK || tile == AMMO_BOX || tile == SHOTGUN_PICKUP) {
				if (active_pickup_count < MAX_PICKUPS) {
					pickups[active_pickup_count].x =
						x * TILE_SIZE + TILE_SIZE / 2;
					pickups[active_pickup_count].y =
						y * TILE_SIZE + TILE_SIZE / 2;
					pickups[active_pickup_count].type = tile;
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
		real_game_start();
		game_state = GAME_PLAYING;
	}
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
	mode.is_seen += dt;
	if (mode.is_seen < 0.5) {
		tft_draw_string(60, 50, RED, "START");
	} else if (mode.is_seen > 1.0) {
		mode.is_seen = 0;
	}
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
	tft_draw_string(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 20, RED, "YOU DIED");
	tft_draw_string(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2, WHITE, "Final Score: %d",
			player.score);
	tft_draw_string(SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT / 2 + 20, WHITE,
			"Press START to restart");
	tft_draw_string(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 + 40, WHITE,
			"Press SELECT for menu");
}
void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	tft_fill(0);
	switch (game_state) {
	case GAME_MENU:
		render_menu(dt);
		break;

	case GAME_PLAYING:
		renderGame();
		if (mode.debug) {
			debug();
		}
		player_view_draw();
		break;

	case GAME_DEAD:
		renderGame(); // Still render the game world in background
		render_dead_state();
		break;
	}
}

static void textures_load()
{
	for (int i = 0; i < 19; i++) {
		Textures[i].name = texture_names[i];
		Textures[i].h = texture_heights[i];
		Textures[i].w = texture_widths[i];
	}

	// Load enemy textures - using the exact array names from your headers
	Textures[TEX_ENEMY1].name = TTE_00; // ENEMY1 texture
	Textures[TEX_ENEMY1].w = TTE_00_WIDTH;
	Textures[TEX_ENEMY1].h = TTE_00_HEIGHT;

	Textures[TEX_ENEMY2].name = TTE_01; // ENEMY2 texture
	Textures[TEX_ENEMY2].w = TTE_01_WIDTH;
	Textures[TEX_ENEMY2].h = TTE_01_HEIGHT;

	Textures[TEX_HEALTH_PICKUP].name = TTO_00; // Health pickup
	Textures[TEX_HEALTH_PICKUP].w = TTO_00_WIDTH;
	Textures[TEX_HEALTH_PICKUP].h = TTO_00_HEIGHT;

	Textures[TEX_AMMO_PICKUP].name = TTO_01; // Ammo pickup
	Textures[TEX_AMMO_PICKUP].w = TTO_01_WIDTH;
	Textures[TEX_AMMO_PICKUP].h = TTO_01_HEIGHT;

	Textures[TEX_SHOTGUN_PICKUP].name = TTO_02; // Shotgun pickup
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
