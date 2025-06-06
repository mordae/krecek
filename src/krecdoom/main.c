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
	int tile_x, tile_y;
	int start_x, start_y;
	int window_start_x, window_start_y;
	int step_x, step_y;
	int hit, side;
	int p_x, p_y;
	float p_center_x, p_center_y;
} MMap;
static MMap mmap;

float volume = 0.5f;

bool isWall(int Tile_x, int Tile_y);
void drawMenuMap();
extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
TileType (*currentMap)[MAP_COLS] = maps_map1;

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
	if (player.gun > N_GUNS)
		player.gun = 0;

	switch (gun_select[player.gun]) {
	case PISTOL:
		sdk_draw_tile(65, 65, &ts_pistol_png, 1);
		break;
	case SHOTGUN:
		sdk_draw_tile(55, 49, &ts_shotgun_png, 1);
		break;
	case BROKE:
		break;
	}
}

void game_start(void)
{
	sdk_set_output_gain_db(volume);
	player.x = TILE_SIZE * 1.5f;
	player.y = TILE_SIZE * 1.5f;
	player.angle = (float)M_PI / 2.0f;
	player.health = 100;
	player.alive = true;
	player.gun = 1;

	mode.debug = false;
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
	if (sdk_inputs_delta.b == 1 && sdk_inputs.start) {
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
	game_handle_audio(dt, volume);
	handlePlayerMovement(dt);
}
void debug(void)
{
	drawMinimap_Debug();
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
	if (mode.map)
		drawMenuMap();
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = WHITE,
	};
	sdk_main(&config);
	return 0;
}
