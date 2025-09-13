//#include "extras/help.h"
#include "common.h"
#include "extras/volume.h"
#include "sdk/input.h"
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <tft.h>
#include <stdlib.h>

#include <overline.png.h>
#include <pistol.png.h>
#include <shotgun.png.h>

//#include "textures/T_00.h"
//#include "textures/T_01.h"
//#include "textures/T_02.h"
//#include "textures/T_03.h"
//#include "textures/T_04.h"
//#include "textures/T_05.h"
//#include "textures/T_06.h"
//#include "textures/T_07.h"
//#include "textures/T_08.h"
#include "textures/T_09.h"
//#include "textures/T_10.h"
//#include "textures/T_11.h"
//#include "textures/T_12.h"
//#include "textures/T_13.h"
//#include "textures/T_14.h"
//#include "textures/T_15.h"
//#include "textures/T_16.h"
//#include "textures/T_17.h"
//#include "textures/T_18.h"
//#include "textures/T_19.h"

#include <cover.png.h>

sdk_game_info("krecdoom", &image_cover_png);

// Movement joystick + b and x
// Debug_mode start+a
// Chenage Gun start+b
// Map select
typedef enum {
	BROKE = 0,
	PISTOL = 1,
	SHOTGUN,
} Gun;
Gun gun_select[N_GUNS] = { 0, 1, 2 };

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
} Player;
static Player player;

typedef struct {
	bool debug;
	bool map;
	bool low;
	uint16_t tilecolor;
} Mode;
static Mode mode;

typedef struct {
	bool hit_visible;
	uint32_t hit_timer_start;
	int hit_screen_x, hit_screen_y;
	int hit_size;
	uint16_t hit_color;
} Bullet;
static Bullet bullet;
typedef struct {
	int tile_x, tile_y;
	int start_x, start_y;
	int window_start_x, window_start_y;
	int step_x, step_y;
	int hit, side;
	int p_x, p_y;
	float p_center_x, p_center_y;
} MMap;
static MMap mmap;

typedef struct {
	int w, h;		   //texture width/height
	const unsigned char *name; //texture name
} TexureMaps;
static TexureMaps Textures[64];

float volume = 0.5f;
float timer = 0;

static bool isWall(int Tile_x, int Tile_y);
static void shootBullet(float start_angle, float max_range_tiles, int visual_size,
			uint16_t visual_color);
static void handleShooting();
static void Map_1_start_player();
static void Map_2_start_player();
static bool Can_shot(float dt);
static void map_starter_caller();
static void textures_load();
extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
const TileType (*currentMap)[MAP_COLS] = maps_map1;
#define FIXED_SHIFT 16
#define FIXED_SCALE (1 << FIXED_SHIFT)
typedef int32_t fixed_t;

#define float_to_fixed(x) ((fixed_t)((x) * FIXED_SCALE))
#define fixed_to_float(x) ((x) / (float)FIXED_SCALE)
#define fixed_mul(a, b) ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define fixed_div(a, b) ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))

// Precomputed fixed-point values for shading
static fixed_t fixed_tile_size;
static fixed_t fixed_max_dist;
static fixed_t fixed_min_shade;
static fixed_t fixed_side_shade;

void renderGame()
{
	// Raycaster Engine LOL
	// MY 2.5D world

	float rayAngle;

	// Precompute fixed-point values if not already done
	if (fixed_tile_size == 0) {
		fixed_tile_size = float_to_fixed(TILE_SIZE);
		fixed_max_dist = float_to_fixed(15.0f * TILE_SIZE);
		fixed_min_shade = float_to_fixed(0.3f);
		fixed_side_shade = float_to_fixed(0.8f);
	}

	// going each vertical line
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		//cool Math I dont understand to calculate angle
		rayAngle = (player.angle - FOV_RADIANS / 2.0f) + (x * (FOV_RADIANS / SCREEN_WIDTH));

		// player standing in
		int Tile_x = (int)(player.x / TILE_SIZE);
		int Tile_y = (int)(player.y / TILE_SIZE);

		// ray's direction  X and Y
		float rayDirX = cosf(rayAngle);
		float rayDirY = sinf(rayAngle);

		// calculate how far
		float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX) * TILE_SIZE;
		float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY) * TILE_SIZE;

		// Where I am walking
		float sideDistX; // next vertical line
		float sideDistY; // next horizontal line

		mmap.hit = 0; // No wall hit yet

		// how to step and where my initial vertical mmap.side is.
		if (rayDirX < 0.0f) {	  // pointing left
			mmap.step_x = -1; // step left
			sideDistX = (player.x - Tile_x * TILE_SIZE) * deltaDistX / TILE_SIZE;
		} else { // right
			mmap.step_x = 1;
			sideDistX = ((Tile_x + 1) * TILE_SIZE - player.x) * deltaDistX / TILE_SIZE;
		}

		// for Y the same
		if (rayDirY < 0.0f) {	  // pointing up
			mmap.step_y = -1; // step up.
			sideDistY = (player.y - Tile_y * TILE_SIZE) * deltaDistY / TILE_SIZE;
		} else { // down
			mmap.step_y = 1;
			sideDistY = ((Tile_y + 1) * TILE_SIZE - player.y) * deltaDistY / TILE_SIZE;
		}

		// loop the ray is searching for wall
		while (mmap.hit == 0) {
			if (sideDistX < sideDistY) {	 // next X-mmap.side is closer
				sideDistX += deltaDistX; // jump to that X-mmap.side.
				Tile_x += mmap.step_x;	 // next X tile.
				mmap.side = 0;
			} else { // If the next Y-mmap.side is closer..
				sideDistY += deltaDistY;
				Tile_y += mmap.step_y;
				mmap.side = 1;
			}

			// finally mmap.step_y a wall or gone GG
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

		// dont / 0
		if (perpWallDist < 0.001f)
			perpWallDist = 0.001f;

		float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);

		int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
		int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

		// drawing stays inside the screen
		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		// Get wall type from map
		int wallType = currentMap[Tile_y][Tile_x] - 1;
		if (wallType <= 0)
			wallType = 0;

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

		// Get texture dimensions
		int texWidth = Textures[wallType].w;
		int texHeight = Textures[wallType].h;

		// Calculate texture X coordinate
		int texX = (int)(wallX * (float)texWidth);
		if ((mmap.side == 0 && rayDirX > 0) || (mmap.side == 1 && rayDirY < 0)) {
			texX = texWidth - texX - 1;
		}

		// Calculate texture stepping
		float step = 1.0f * texHeight / wallHeight;
		float texPos = (drawStart - SCREEN_HEIGHT / 2.0f + wallHeight / 2.0f) * step;

		// Convert perpWallDist to fixed-point for shading
		fixed_t perpWallDist_fixed = float_to_fixed(perpWallDist);

		// Calculate shading using fixed-point arithmetic
		fixed_t shade = FIXED_SCALE - fixed_div(perpWallDist_fixed, fixed_max_dist);

		// Clamp minimum shade
		if (shade < fixed_min_shade) {
			shade = fixed_min_shade;
		}

		// Darken y-side walls
		if (mmap.side == 1) {
			shade = fixed_mul(shade, fixed_side_shade);
		}

		// Convert shade to 8.8 fixed point for faster multiplication
		int shade_int = (shade >> 8) & 0xFFFF;

		// Draw the textured vertical slice
		for (int y = drawStart; y <= drawEnd; y++) {
			// Calculate texture Y coordinate
			int texY =
				(int)texPos &
				(texHeight -
				 1); // Use bitwise AND for faster wrapping if texture height is a power of 2
			texPos += step;

			// Get the pixel index in the texture data (RGB format, 3 bytes per pixel)
			int pixelIndex = texY * 3 * texWidth + texX * 3;

			// Extract RGB components
			int r = Textures[wallType].name[pixelIndex + 0];
			int g = Textures[wallType].name[pixelIndex + 1];
			int b = Textures[wallType].name[pixelIndex + 2];

			// Apply distance shading using fixed-point arithmetic
			r = (r * shade_int) >> 8;
			g = (g * shade_int) >> 8;
			b = (b * shade_int) >> 8;

			// Clamp values to prevent overflow
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;

			// Draw the pixel
			tft_draw_pixel(x, y, rgb_to_rgb565(r, g, b));
		}
	}
}

bool isWall(int Tile_x, int Tile_y)
{
	// boundaries
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return true;
	}
	return currentMap[Tile_y][Tile_x] == WALL_COMMON;
}

void handlePlayerMovement(float dt)
{
	float player_dx = 0;
	float player_dy = 0;

	//	if (sdk_inputs.joy_y > 500) {
	//		player_dx -= cosf(player.angle) * MOVE_SPEED * dt;
	//		player_dy -= sinf(player.angle) * MOVE_SPEED * dt;
	//	} else if (sdk_inputs.joy_y < -500) {
	//		player_dx += cosf(player.angle) * MOVE_SPEED * dt;
	//		player_dy += sinf(player.angle) * MOVE_SPEED * dt;
	//	}

	player_dx = cosf(player.angle) * MOVE_SPEED * dt * (-1 * (float)sdk_inputs.joy_y / 2048);
	player_dy = sinf(player.angle) * MOVE_SPEED * dt * (-1 * (float)sdk_inputs.joy_y / 2048);

	if (player.strafing) {
		player_dx += cosf(player.angle + M_PI / 2) * MOVE_SPEED * dt *
			     ((float)sdk_inputs.joy_x / 2048);
		player_dy += sinf(player.angle + M_PI / 2) * MOVE_SPEED * dt *
			     ((float)sdk_inputs.joy_x / 2048);
	}

	float player_fx = player.x + player_dx;
	float player_fy = player.y + player_dy;

	int player_tile_fx = player_fx / TILE_SIZE;
	int player_tile_fy = player_fy / TILE_SIZE;

	//Colision
	if (isWall(player_tile_fx, player_tile_fy)) {
		return;
	}

	player.x = player_fx;
	player.y = player_fy;

	int player_tile_x = player.x / TILE_SIZE;
	int player_tile_y = player.y / TILE_SIZE;

	if (currentMap[player_tile_y][player_tile_x] == TELEPORT) {
		if (currentMap == maps_map1) {
			currentMap = maps_map2;
			map_starter_caller();
		} else {
			currentMap = maps_map1;
			map_starter_caller();
		}
	}
}

void player_view_draw()
{
	if (mode.debug) {
		return;
	}
	sdk_draw_tile(0, 0, &ts_overline_png, 1);
	tft_draw_string(22, 105, DRAW_RED, "%-i", player.score);
	tft_draw_string(70, 105, DRAW_RED, "%-i", player.health);
	tft_draw_string(105, 105, DRAW_RED, "%-i", player.ammo);

	// boundaries
	if (player.gun >= N_GUNS)
		player.gun = 0;

	switch (gun_select[player.gun]) {
	case PISTOL:
		sdk_draw_tile(75, 65, &ts_pistol_png, 1);
		break;
	case SHOTGUN:
		sdk_draw_tile(55, 49, &ts_shotgun_png, 1);
		break;
	case BROKE:
		break;
	}
	if (bullet.hit_visible) {
		// time
		uint32_t current_time = time_us_64();
		if (current_time - bullet.hit_timer_start < BULLET_VISUAL_DURATION_MS * 1000) {
			// Draw a square
			for (int dy = -bullet.hit_size / 2; dy <= bullet.hit_size / 2; dy++) {
				for (int dx = -bullet.hit_size / 2; dx <= bullet.hit_size / 2;
				     dx++) {
					int draw_x = bullet.hit_screen_x + dx;
					int draw_y = bullet.hit_screen_y + dy;
					if (draw_x >= 0 && draw_x < SCREEN_WIDTH && draw_y >= 0 &&
					    draw_y < SCREEN_HEIGHT) {
						tft_draw_pixel(draw_x, draw_y, bullet.hit_color);
					}
				}
			}
		} else {
			bullet.hit_visible = false; // Reset if time is up
		}
	}
}

void shootBullet(float current_shoot_angle, float max_range_tiles, int visual_size,
		 uint16_t visual_color)
{
	float rayDirX = cosf(current_shoot_angle);
	float rayDirY = sinf(current_shoot_angle);

	float rayX = player.x;
	float rayY = player.y;

	float max_dist = max_range_tiles * TILE_SIZE; // Max range in world units

	int damage_to_deal = 0;
	if (player.gun == PISTOL) {
		damage_to_deal = PISTOL_DAMAGE;
	} else if (player.gun == SHOTGUN) {
		damage_to_deal = SHOTGUN_DAMAGE;
	}

	for (float dist = 0; dist < max_dist; dist += TILE_SIZE / 4.0f) {
		// Update current ray position
		rayX = player.x + rayDirX * dist;
		rayY = player.y + rayDirY * dist;

		// wall hit
		int current_tile_x = (int)(rayX / TILE_SIZE);
		int current_tile_y = (int)(rayY / TILE_SIZE);

		if (isWall(current_tile_x, current_tile_y)) {
			float perpWallDist = dist; // distance
			if (perpWallDist == 0.0f)
				perpWallDist = 0.001f; // /0

			// screen position for wall hit visualization
			float relativeAngle = current_shoot_angle - player.angle;
			// Normalize angle
			if (relativeAngle > M_PI)
				relativeAngle -= 2 * M_PI;
			if (relativeAngle < -M_PI)
				relativeAngle += 2 * M_PI;

			int screen_x = (int)(SCREEN_WIDTH / 2.0f +
					     PROJECTION_PLANE_DISTANCE * tanf(relativeAngle));
			float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);
			int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
			int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

			// Store
			bullet.hit_visible = true;
			bullet.hit_timer_start = time_us_64();
			bullet.hit_screen_x = screen_x;
			bullet.hit_screen_y = (drawStart + drawEnd) / 2;
			bullet.hit_size = visual_size;
			bullet.hit_color = visual_color;

			if (mode.debug)
				printf("Bullet hit wall (%d, %d), dist %.2f\n", current_tile_x,
				       current_tile_y, dist / TILE_SIZE);

			return;
		}
	}
}
void handleShooting()
{
	if (sdk_inputs_delta.a == 1) {
		if (player.ammo > 0) {
			float shoot_angle = player.angle;
			int visual_size = 10;		// Size of the hit marker
			uint16_t visual_color = YELLOW; // Color of wall hit

			float current_gun_range = 0.0f;

			if (player.gun == PISTOL) {
				current_gun_range = PISTOL_MAX_RANGE_TILES;
			} else if (player.gun == SHOTGUN) {
				current_gun_range = SHOTGUN_MAX_RANGE_TILES;
			} else {
				current_gun_range = PISTOL_MAX_RANGE_TILES;
			}

			shootBullet(shoot_angle, current_gun_range, visual_size, visual_color);
			player.ammo--; // Decrease ammo after shooting
		} else {
			// "click" sound
			if (mode.debug) {
				printf("Out of ammo!\n");
			}
		}
	}
}
void game_start(void)
{
	sdk_set_output_gain_db(volume);
	map_starter_caller();
	textures_load();

	player.health = 100;
	player.x = TILE_SIZE * 1.5f;
	player.y = TILE_SIZE * 1.5f;
	player.angle = (float)M_PI / 2.0f;
	player.ammo = 15;
	player.alive = true;
	player.gun = 1;
}
void map_starter_caller()
{
	if (currentMap == maps_map1) {
		Map_1_start_player();
	}
	if (currentMap == maps_map2) {
		Map_2_start_player();
	}
}

void Map_1_start_player(void)
{
	player.x = TILE_SIZE * 1.5f;
	player.y = TILE_SIZE * 1.5f;
	player.angle = (float)M_PI / 2.0f;
	player.ammo = 15;
	player.alive = true;

	bullet.hit_visible = false;
	bullet.hit_timer_start = 0;
	bullet.hit_screen_x = 0;
	bullet.hit_screen_y = 0;
	bullet.hit_size = 0;
	bullet.hit_color = 0;

	mode.map = false;
}

void Map_2_start_player(void)
{
	player.x = TILE_SIZE * 2.5f;
	player.y = TILE_SIZE * 3.5f;
	player.angle = (float)M_PI / 2.0f;
	player.ammo = 15;
	player.alive = true;

	bullet.hit_visible = false;
	bullet.hit_timer_start = 0;
	bullet.hit_screen_x = 0;
	bullet.hit_screen_y = 0;
	bullet.hit_size = 0;
	bullet.hit_color = 0;

	mode.map = false;
}
void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
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
	}
	if (sdk_inputs_delta.select == 1) {
	}
	if (!player.strafing) {
		player.angle += ROTATE_SPEED * dt * ((float)sdk_inputs.joy_x / 2048);
	}
	if (player.gun > N_GUNS)
		player.gun = 0;
	handleShooting();

	game_handle_audio(dt, volume);
	handlePlayerMovement(dt);
	if (player.health <= 0) {
		map_starter_caller();
	}
}
bool Can_shot(float dt)
{
	timer += dt;
	if (timer >= 1.0f) {
		timer = 0;
		return true;
	} else
		return false;
}
void debug(void)
{
	tft_draw_string(5, 0, WHITE, "X %-.2f Y %-.2f", player.x, player.y);
	tft_draw_string(5, 10, WHITE, "Angle %-.2f", player.angle);
	tft_draw_string(5, 85, WHITE, "Score %-i", player.score);
	tft_draw_string(5, 95, WHITE, "Health %i", player.health);
	tft_draw_string(5, 105, WHITE, "Ammo %-i", player.ammo);
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);

	renderGame();

	player_view_draw();
	if (mode.debug) {
		debug();
	}
}

static void textures_load()
{
	Textures[0].name = T_09;
	Textures[0].h = T_09_HEIGHT;
	Textures[0].w = T_09_WIDTH;
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
