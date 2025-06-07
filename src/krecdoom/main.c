#include "help.h"
#include "common.h"
#include "volume.h"
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <tft.h>

#include <overline.png.h>
#include <pistol.png.h>
#include <shotgun.png.h>

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
} Player;
static Player player;

typedef struct {
	bool debug;
	bool map;
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

static Enemy enemies[MAX_ENEMIES];

float volume = 0.5f;
float timer = 0;

bool isWall(int Tile_x, int Tile_y);
void drawMenuMap();
void shootBullet(float start_angle, float max_range_tiles, int visual_size, uint16_t visual_color);
void drawEnemies();
void handleEnemyAI(float dt);
void handleShooting();
void checkPlayerStatus();
void Map_1_start_enemis(void);
void Map_2_start_enemis(void);
void Map_1_start_player();
void Map_2_start_player();
bool Can_shot(float dt);
void map_starter_caller();

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
const TileType (*currentMap)[MAP_COLS] = maps_map1;

void renderGame()
{
	// Raycaster Engine LOL
	// MY 2.5D world

	float rayAngle;

	// going each vertical line`
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
			sideDistX = (player.x - Tile_x * TILE_SIZE) * deltaDistX /
				    TILE_SIZE; // left grid line.
		} else {		       // right
			mmap.step_x = 1;
			sideDistX = ((Tile_x + 1) * TILE_SIZE - player.x) * deltaDistX / TILE_SIZE;
		}

		// for Y the same
		if (rayDirY < 0.0f) {	  // pointing up
			mmap.step_y = -1; // step up.
			sideDistY = (player.y - Tile_y * TILE_SIZE) * deltaDistY /
				    TILE_SIZE; // upper grid line.
		} else {		       // down
			mmap.step_y = 1;
			sideDistY = ((Tile_y + 1) * TILE_SIZE - player.y) * deltaDistY / TILE_SIZE;
		}

		//  loop the ray is searching for wall
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
		// fixed fisheye
		float perpWallDist;
		if (mmap.side == 0) {
			perpWallDist = (Tile_x * TILE_SIZE - player.x +
					(1.0f - mmap.step_x) * TILE_SIZE / 2.0f) /
				       rayDirX;
		} else {
			perpWallDist = (Tile_y * TILE_SIZE - player.y +
					(1.0f - mmap.step_y) * TILE_SIZE / 2.0f) /
				       rayDirY;
		}

		// dont / 0
		if (perpWallDist == 0.0f)
			perpWallDist = 0.001f;

		// Store the calculated wall distance in the Z-buffer ---
		zBuffer[x] = perpWallDist;

		// How tall slice
		float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);

		// where on the screen this wall slice
		int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
		int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

		// drawing stays inside the screen
		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		// What will be The wall??????????????? COLOR
		uint16_t wallColor;
		if (mmap.side == 0) {
			wallColor = COLOR_WALL_BRIGHT;
		} else {
			wallColor = COLOR_WALL_DARK;
		}

		// DRAWIG WALLLLLLLLLLLLLLLLL
		for (int y = drawStart; y <= drawEnd; y++) {
			tft_draw_pixel(x, y, wallColor);
		}

		// Drawing CEILINGGGGGGGGGG
		for (int y = 0; y < drawStart; y++) {
			tft_draw_pixel(x, y, COLOR_CEILING);
		}
		// drawing FLOORRRRRRRRRRRRRR
		for (int y = drawEnd + 1; y < SCREEN_HEIGHT; y++) {
			tft_draw_pixel(x, y, COLOR_FLOOR);
		}
	}
}
void drawMinimap_Debug()
{
	//THE GREATEST IDEA

	// right corner
	mmap.start_x = SCREEN_WIDTH - MINIMAP_WIDTH - MINIMAP_PADDING;
	mmap.start_y = SCREEN_HEIGHT - MINIMAP_HEIGHT - MINIMAP_PADDING;

	// tile
	mmap.tile_x = (int)(player.x / TILE_SIZE);
	mmap.tile_y = (int)(player.y / TILE_SIZE);

	// top-left corner going show on the minimap.
	mmap.window_start_x = mmap.tile_x - (MINIMAP_VIEW_TILES / 2);
	mmap.window_start_y = mmap.tile_y - (MINIMAP_VIEW_TILES / 2);

	// I'm making sure the minimap window doesn't go off the left or top edge of our map
	if (mmap.window_start_x < 0)
		mmap.window_start_x = 0;
	if (mmap.window_start_y < 0)
		mmap.window_start_y = 0;

	// I'm making sure the minimap window doesn't go off the right or bottoom edge of our map
	if (mmap.window_start_x + MINIMAP_VIEW_TILES > MAP_COLS) {
		mmap.window_start_x = MAP_COLS - MINIMAP_VIEW_TILES;
		if (mmap.window_start_x < 0)
			mmap.window_start_x = 0;
	}
	if (mmap.window_start_y + MINIMAP_VIEW_TILES > MAP_ROWS) {
		mmap.window_start_y = MAP_ROWS - MINIMAP_VIEW_TILES;
		if (mmap.window_start_y < 0)
			mmap.window_start_y = 0;
	}

	// drawing all the visible map tiles
	for (int y_minimap_idx = 0; y_minimap_idx < MINIMAP_VIEW_TILES; y_minimap_idx++) {
		for (int x_minimap_idx = 0; x_minimap_idx < MINIMAP_VIEW_TILES; x_minimap_idx++) {
			// map coordinates
			int map_tile_x = mmap.window_start_x + x_minimap_idx;
			int map_tile_y = mmap.window_start_y + y_minimap_idx;

			// empty space or not?
			if (isWall(map_tile_x, map_tile_y)) {
				mode.tilecolor = COLOR_MINIMAP_WALL; // wall white
			} else {
				mode.tilecolor = COLOR_MINIMAP_EMPTY; // empty black
			}

			// drawing minimap
			for (int py = 0; py < MINIMAP_TILE_SIZE; py++) {
				for (int px = 0; px < MINIMAP_TILE_SIZE; px++) {
					tft_draw_pixel(
						mmap.start_x + x_minimap_idx * MINIMAP_TILE_SIZE +
							px,
						mmap.start_y + y_minimap_idx * MINIMAP_TILE_SIZE +
							py,
						mode.tilecolor);
				}
			}
		}
	}

	// player on the minimap.
	// converting player into tiny minimap pixels
	player.relative_x = player.x / TILE_SIZE - mmap.window_start_x;
	player.relative_y = player.y / TILE_SIZE - mmap.window_start_y;

	mmap.p_x = mmap.start_x +
		   (int)(player.relative_x * MINIMAP_TILE_SIZE - (PLAYER_DOT_SIZE / 2.0f));
	mmap.p_y = mmap.start_y +
		   (int)(player.relative_y * MINIMAP_TILE_SIZE - (PLAYER_DOT_SIZE / 2.0f));

	// yellow square
	// making sure the player is in
	if (mmap.p_x >= mmap.start_x &&
	    mmap.p_x + PLAYER_DOT_SIZE <= mmap.start_x + MINIMAP_WIDTH &&
	    mmap.p_y >= mmap.start_y &&
	    mmap.p_y + PLAYER_DOT_SIZE <= mmap.start_y + MINIMAP_HEIGHT) {
		for (int py = 0; py < PLAYER_DOT_SIZE; py++) {
			for (int px = 0; px < PLAYER_DOT_SIZE; px++) {
				tft_draw_pixel(mmap.p_x + px, mmap.p_y + py, COLOR_MINIMAP_PLAYER);
			}
		}
	}
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (enemies[i].alive) {
			// enemy relative to the minimap
			float enemy_relative_x = enemies[i].x / TILE_SIZE - mmap.window_start_x;
			float enemy_relative_y = enemies[i].y / TILE_SIZE - mmap.window_start_y;

			// Convert relative position to pixel coordinates on the minimap
			int enemy_p_x = mmap.start_x + (int)(enemy_relative_x * MINIMAP_TILE_SIZE -
							     (PLAYER_DOT_SIZE / 2.0f));
			int enemy_p_y = mmap.start_y + (int)(enemy_relative_y * MINIMAP_TILE_SIZE -
							     (PLAYER_DOT_SIZE / 2.0f));

			// Check if enemy dot is within the minimap visible bounds
			if (enemy_p_x >= mmap.start_x &&
			    enemy_p_x + PLAYER_DOT_SIZE <= mmap.start_x + MINIMAP_WIDTH &&
			    enemy_p_y >= mmap.start_y &&
			    enemy_p_y + PLAYER_DOT_SIZE <= mmap.start_y + MINIMAP_HEIGHT) {
				for (int py = 0; py < PLAYER_DOT_SIZE; py++) {
					for (int px = 0; px < PLAYER_DOT_SIZE; px++) {
						tft_draw_pixel(enemy_p_x + px, enemy_p_y + py, RED);
					}
				}
			}
		}
	}

	// field of view
	mmap.p_center_x = mmap.p_x + (PLAYER_DOT_SIZE / 2.0f);
	mmap.p_center_y = mmap.p_y + (PLAYER_DOT_SIZE / 2.0f);

	float step_size = 1.0f;

	//  leftmost view ray.
	float current_ray_x = mmap.p_center_x;
	float current_ray_y = mmap.p_center_y;
	float left_ray_angle = player.angle - (FOV_RADIANS / 2.0f); // angle.
	float ray_dir_x = cosf(left_ray_angle);
	float ray_dir_y = sinf(left_ray_angle);

	// view ray until wall
	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		// boundaries
		if (map_pixel_x < mmap.start_x || map_pixel_x >= mmap.start_x + MINIMAP_WIDTH ||
		    map_pixel_y < mmap.start_y || map_pixel_y >= mmap.start_y + MINIMAP_HEIGHT) {
			break;
		}

		// convert world map
		int tile_x_world =
			mmap.window_start_x + (map_pixel_x - mmap.start_x) / MINIMAP_TILE_SIZE;
		int tile_y_world =
			mmap.window_start_y + (map_pixel_y - mmap.start_y) / MINIMAP_TILE_SIZE;

		if (isWall(tile_x_world, tile_y_world)) {
			break; // Yes, a wall
		}
		tft_draw_pixel(map_pixel_x, map_pixel_y, COLOR_MINIMAP_LINE);
		current_ray_x += ray_dir_x * step_size;
		current_ray_y += ray_dir_y * step_size;
	}

	// right
	current_ray_x = mmap.p_center_x;
	current_ray_y = mmap.p_center_y;
	float right_ray_angle = player.angle + (FOV_RADIANS / 2.0f);
	ray_dir_x = cosf(right_ray_angle);
	ray_dir_y = sinf(right_ray_angle);

	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		if (map_pixel_x < mmap.start_x || map_pixel_x >= mmap.start_x + MINIMAP_WIDTH ||
		    map_pixel_y < mmap.start_y || map_pixel_y >= mmap.start_y + MINIMAP_HEIGHT) {
			break;
		}

		int tile_x_world =
			mmap.window_start_x + (map_pixel_x - mmap.start_x) / MINIMAP_TILE_SIZE;
		int tile_y_world =
			mmap.window_start_y + (map_pixel_y - mmap.start_y) / MINIMAP_TILE_SIZE;

		if (isWall(tile_x_world, tile_y_world)) {
			break;
		}

		tft_draw_pixel(map_pixel_x, map_pixel_y, COLOR_MINIMAP_LINE);

		current_ray_x += ray_dir_x * step_size;
		current_ray_y += ray_dir_y * step_size;
	}
}

// Map timeeeeeeeeeeee
void drawMenuMap()
{
	// center screen
	int map_start_x = (SCREEN_WIDTH - MENU_MAP_WIDTH) / 2;
	int map_start_y = (SCREEN_HEIGHT - MENU_MAP_HEIGHT) / 2;

	// How big Tile
	float tile_pixel_width = (float)MENU_MAP_WIDTH / MAP_COLS;
	float tile_pixel_height = (float)MENU_MAP_HEIGHT / MAP_ROWS;

	// Drawing wall in map
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (currentMap[y][x] != EMPTY) {
				mode.tilecolor = WHITE;
			} else {
				mode.tilecolor = BLACK;
			}

			// rectangle for a wall
			for (int py = 0; py < tile_pixel_height; py++) {
				for (int px = 0; px < tile_pixel_width; px++) {
					tft_draw_pixel(
						map_start_x + (int)(x * tile_pixel_width) + px,
						map_start_y + (int)(y * tile_pixel_height) + py,
						mode.tilecolor);
				}
			}
		}
	}

	// world position to map.
	int map_p_x = map_start_x +
		      (int)((player.x / TILE_SIZE) * tile_pixel_width - (PLAYER_DOT_SIZE / 2.0f));
	int map_p_y = map_start_y +
		      (int)((player.y / TILE_SIZE) * tile_pixel_height - (PLAYER_DOT_SIZE / 2.0f));

	// boundaries
	if (map_p_x >= map_start_x && map_p_x + PLAYER_DOT_SIZE <= map_start_x + MENU_MAP_WIDTH &&
	    map_p_y >= map_start_y && map_p_y + PLAYER_DOT_SIZE <= map_start_y + MENU_MAP_HEIGHT) {
		for (int py = 0; py < PLAYER_DOT_SIZE; py++) {
			for (int px = 0; px < PLAYER_DOT_SIZE; px++) {
				tft_draw_pixel(map_p_x + px, map_p_y + py, YELLOW);
			}
		}
	}
}

bool isWall(int Tile_x, int Tile_y)
{
	// boundaries
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return true;
	}
	return currentMap[Tile_y][Tile_x] == WALL;
}

void handlePlayerMovement(float dt)
{
	float player_dx = 0;
	float player_dy = 0;

	if (sdk_inputs.joy_y > 500) {
		player_dx -= cosf(player.angle) * MOVE_SPEED * dt;
		player_dy -= sinf(player.angle) * MOVE_SPEED * dt;
	} else if (sdk_inputs.joy_y < -500) {
		player_dx += cosf(player.angle) * MOVE_SPEED * dt;
		player_dy += sinf(player.angle) * MOVE_SPEED * dt;
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

		// enemy hit
		float enemy_collision_radius = TILE_SIZE * 0.4f;

		for (int j = 0; j < MAX_ENEMIES; j++) {
			if (enemies[j].alive) {
				float enemy_dist_x = rayX - enemies[j].x;
				float enemy_dist_y = rayY - enemies[j].y;
				float dist_to_enemy_center = sqrtf(enemy_dist_x * enemy_dist_x +
								   enemy_dist_y * enemy_dist_y);

				if (dist_to_enemy_center < enemy_collision_radius) {
					// Bullet hit
					enemies[j].health -= damage_to_deal; // Apply damage
					if (enemies[j].health <= 0) {
						enemies[j].alive = false; // Enemy is dead
						enemies[j].health =
							0; // Ensure health doesn't go negative
						player.score += 100; // Award score for kill
					}

					bullet.hit_visible = true;
					bullet.hit_timer_start = time_us_64();

					// position of the exact hit point
					float hit_relX = rayX - player.x;
					float hit_relY = rayY - player.y;
					float hit_angle = atan2f(hit_relY, hit_relX);
					float hit_angle_diff = hit_angle - player.angle;
					// Normalize angle
					if (hit_angle_diff > M_PI)
						hit_angle_diff -= 2 * M_PI;
					if (hit_angle_diff < -M_PI)
						hit_angle_diff += 2 * M_PI;

					float hit_dist_from_player = dist; // Use the ray distance
					if (hit_dist_from_player == 0.0f)
						hit_dist_from_player = 0.001f;

					// Calculate screen X for the hit
					bullet.hit_screen_x = (int)(SCREEN_WIDTH / 2.0f +
								    PROJECTION_PLANE_DISTANCE *
									    tanf(hit_angle_diff));

					bullet.hit_screen_y = (int)((SCREEN_HEIGHT / 2.0f));
					bullet.hit_size = visual_size;
					bullet.hit_color = (enemies[j].alive) ? GREEN : RED;

					if (mode.debug)
						printf("Bullet hit %d at (%.2f, %.2f), health now %d\n",
						       j, enemies[j].x, enemies[j].y,
						       enemies[j].health);

					return; // Bullet hit an enemy, stop
				}
			}
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
		Map_1_start_enemis();
		Map_1_start_player();
	}
	if (currentMap == maps_map2) {
		Map_2_start_enemis();
		Map_2_start_player();
	}
}
void Map_1_start_enemis(void)
{
	enemies[0].x = TILE_SIZE * 6.0f;
	enemies[0].y = TILE_SIZE * 6.0f;
	enemies[0].alive = true;
	enemies[0].state = ENEMY_IDLE;
	enemies[0].last_attack_time = 0;

	enemies[1].x = TILE_SIZE * 15.0f;
	enemies[1].y = TILE_SIZE * 15.0f;
	enemies[1].alive = true;
	enemies[1].state = ENEMY_IDLE;
	enemies[1].last_attack_time = 0;

	enemies[2].x = TILE_SIZE * 18.0f;
	enemies[2].y = TILE_SIZE * 3.0f;
	enemies[2].alive = true;
	enemies[2].state = ENEMY_IDLE;
	enemies[2].last_attack_time = 0;

	enemies[3].x = TILE_SIZE * 10.0f;
	enemies[3].y = TILE_SIZE * 6.0f;
	enemies[3].alive = true;
	enemies[3].state = ENEMY_IDLE;
	enemies[3].last_attack_time = 0;

	enemies[4].x = TILE_SIZE * 15.0f;
	enemies[4].y = TILE_SIZE * 18.0f;
	enemies[4].alive = true;
	enemies[4].state = ENEMY_IDLE;
	enemies[4].last_attack_time = 0;

	enemies[5].x = TILE_SIZE * 18.0f;
	enemies[5].y = TILE_SIZE * 18.0f;
	enemies[5].alive = true;
	enemies[5].state = ENEMY_IDLE;
	enemies[5].last_attack_time = 0;

	enemies[6].x = TILE_SIZE * 6.0f;
	enemies[6].y = TILE_SIZE * 16.0f;
	enemies[6].alive = true;
	enemies[6].state = ENEMY_IDLE;
	enemies[6].last_attack_time = 0;
}

void Map_2_start_enemis(void)
{
	enemies[0].x = TILE_SIZE * 10.0f;
	enemies[0].y = TILE_SIZE * 10.0f;
	enemies[0].alive = true;
	enemies[0].state = ENEMY_IDLE;
	enemies[0].last_attack_time = 0;

	enemies[1].x = TILE_SIZE * 15.0f;
	enemies[1].y = TILE_SIZE * 15.0f;
	enemies[1].alive = true;
	enemies[1].state = ENEMY_IDLE;
	enemies[1].last_attack_time = 0;
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
		if (mode.map) {
			mode.map = false;
		} else
			mode.map = true;
	}
	if (sdk_inputs.joy_x > 500 || sdk_inputs.b) {
		player.angle += ROTATE_SPEED * dt;
	} else if (sdk_inputs.joy_x < -500 || sdk_inputs.x) {
		player.angle -= ROTATE_SPEED * dt;
	}
	if (player.gun > N_GUNS)
		player.gun = 0;
	handleShooting();

	game_handle_audio(dt, volume);
	handlePlayerMovement(dt);
	handleEnemyAI(dt);
	if (player.health <= 0) {
		map_starter_caller();
	}
}
void handleEnemyAI(float dt)
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (enemies[i].alive && player.alive) { // Enemy only acts if both are alive
			float dx = player.x - enemies[i].x;
			float dy = player.y - enemies[i].y;
			float dist_to_player = sqrtf(dx * dx + dy * dy);

			// Update enemy state based on vision range
			if (dist_to_player < ENEMY_VISION_RANGE_TILES * TILE_SIZE) {
				enemies[i].state = ENEMY_CHASING;
			} else {
				enemies[i].state = ENEMY_IDLE;
			}

			if (enemies[i].state == ENEMY_CHASING) {
				// --- Enemy Movement (Chasing) ---
				float angleToPlayer = atan2f(dy, dx); // Angle from enemy to player

				float move_amount = ENEMY_MOVE_SPEED * dt;

				float desired_dx = cosf(angleToPlayer) * move_amount;
				float desired_dy = sinf(angleToPlayer) * move_amount;

				float next_enemy_x = enemies[i].x + desired_dx;
				float next_enemy_y = enemies[i].y + desired_dy;

				// Simple collision handling for enemy movement
				int current_enemy_tile_x = (int)(enemies[i].x / TILE_SIZE);
				int current_enemy_tile_y = (int)(enemies[i].y / TILE_SIZE);

				int next_tile_x_full_move = (int)(next_enemy_x / TILE_SIZE);
				int next_tile_y_full_move = (int)(next_enemy_y / TILE_SIZE);

				if (!isWall(next_tile_x_full_move, next_tile_y_full_move)) {
					enemies[i].x = next_enemy_x;
					enemies[i].y = next_enemy_y;
				} else {
					if (!isWall((int)(next_enemy_x / TILE_SIZE),
						    current_enemy_tile_y)) {
						enemies[i].x = next_enemy_x;
					}
					if (!isWall(current_enemy_tile_x,
						    (int)(next_enemy_y / TILE_SIZE))) {
						enemies[i].y = next_enemy_y;
					}
				}

				uint32_t current_time = dt * 100000.0f;
				if (dist_to_player < ENEMY_ATTACK_RANGE_TILES * TILE_SIZE &&
				    (current_time - enemies[i].last_attack_time >
				     ENEMY_ATTACK_COOLDOWN_MS)) {
					// Ray from enemy to player
					float LOS_dx = player.x - enemies[i].x;
					float LOS_dy = player.y - enemies[i].y;
					float LOS_dist_to_player =
						sqrtf(LOS_dx * LOS_dx + LOS_dy * LOS_dy);

					// avoid  / 0
					float LOS_rayDirX = (LOS_dist_to_player == 0.0f) ?
								    0 :
								    LOS_dx / LOS_dist_to_player;
					float LOS_rayDirY = (LOS_dist_to_player == 0.0f) ?
								    0 :
								    LOS_dy / LOS_dist_to_player;

					bool line_of_sight_clear = true;
					for (float step_dist = TILE_SIZE * 0.1f;
					     step_dist < LOS_dist_to_player - TILE_SIZE * 0.1f;
					     step_dist += TILE_SIZE * 0.5f) { // Step by half a tile
						float check_x =
							enemies[i].x + LOS_rayDirX * step_dist;
						float check_y =
							enemies[i].y + LOS_rayDirY * step_dist;

						if (isWall((int)(check_x / TILE_SIZE),
							   (int)(check_y / TILE_SIZE))) {
							line_of_sight_clear = false;
							break; // Wall check
						}
					}

					if (line_of_sight_clear) {
						Can_shot(dt);
						if (Can_shot(dt)) {
							player.health -= ENEMY_DAMAGE;
							enemies[i].last_attack_time = current_time;
						}

						if (mode.debug) {
							printf("Enemy attacked Player health: %d\n",
							       player.health);
						}
					}
				}
			}
		}
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
	drawMinimap_Debug();
	tft_draw_string(5, 0, WHITE, "X %-.2f Y %-.2f", player.x, player.y);
	tft_draw_string(5, 10, WHITE, "Angle %-.2f", player.angle);
	if (enemies[0].alive == true)
		tft_draw_string(5, 75, WHITE, "Enemy State: %s",
				enemies[5].state == ENEMY_IDLE ? "IDLE" : "CHASING");

	tft_draw_string(5, 85, WHITE, "Score %-i", player.score);
	tft_draw_string(5, 95, WHITE, "Health %i", player.health);
	tft_draw_string(5, 105, WHITE, "Ammo %-i", player.ammo);
}
void drawEnemies()
{
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (enemies[i].alive) {
			// Calculate relative position of enemy to player
			float relX = enemies[i].x - player.x;
			float relY = enemies[i].y - player.y;

			float distToEnemy = sqrtf(relX * relX + relY * relY);

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

				float projectedSize =
					(TILE_SIZE * PROJECTION_PLANE_DISTANCE / distToEnemy);
				int enemy_draw_size = (int)projectedSize;

				// Clamp min and max
				if (enemy_draw_size < 1)
					enemy_draw_size = 1;
				if (enemy_draw_size > SCREEN_HEIGHT * 2)
					enemy_draw_size = SCREEN_HEIGHT * 2; //cap

				// top-left corner
				int draw_x_start = screen_x - enemy_draw_size / 2;
				int draw_y_start =
					(int)((SCREEN_HEIGHT / 2.0f) - (enemy_draw_size / 2.0f));

				// Clamp bounds to screen
				int x_loop_start = MAX(0, draw_x_start);
				int x_loop_end = MIN(SCREEN_WIDTH, draw_x_start + enemy_draw_size);
				int y_loop_start = MAX(0, draw_y_start);
				int y_loop_end = MIN(SCREEN_HEIGHT, draw_y_start + enemy_draw_size);

				// Draw the green square
				for (int y_pixel = y_loop_start; y_pixel < y_loop_end; y_pixel++) {
					for (int x_pixel = x_loop_start; x_pixel < x_loop_end;
					     x_pixel++) {
						if (distToEnemy < zBuffer[x_pixel]) {
							tft_draw_pixel(x_pixel, y_pixel, GREEN);
						}
					}
				}
			}
		}
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);
	renderGame();
	drawEnemies();
	player_view_draw();
	if (mode.debug) {
		debug();
	}
	if (mode.map)
		drawMenuMap();
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
