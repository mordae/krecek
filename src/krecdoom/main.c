#include "common.h"
#include "volume.h"
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <tft.h>

float playerX = TILE_SIZE * 1.5f;
float playerY = TILE_SIZE * 1.5f;
float volume = 0.5f;
float playerAngle = (float)M_PI / 2.0f;
bool debug_mode = false;
bool isWall(int Tile_x, int Tile_y);

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];

TileType (*currentMap)[MAP_COLS] = maps_map1;

void renderGame()
{
	// Its raycaster Engine time

	float rayAngle;

	// Now every pixel I will cast a ray
	// For is loop that Deepseek give to me
	// And now I know I define int atd. in fuction
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		// Some cool Math I dont uderstende but the wiki told me its good
		// YES I read wiki
		rayAngle = (playerAngle - FOV_RADIANS / 2.0f) + (x * (FOV_RADIANS / SCREEN_WIDTH));

		// This is Where on what tile I am
		int Tile_x = (int)(playerX / TILE_SIZE);
		int Tile_y = (int)(playerY / TILE_SIZE);

		// From some video that told me I need to use cosf
		float rayDirX = cosf(rayAngle);
		float rayDirY = sinf(rayAngle);

		// More wiki math
		float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX) * TILE_SIZE;
		float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY) * TILE_SIZE;

		// store to add to `sideDistX` or `sideDistY` for steps
		int stepX;
		int stepY;

		// store distance ray from start to next X or Y side.
		float sideDistX;
		float sideDistY;

		int hit = 0;
		int side;

		// If ray is left (- X), stepX is -1.
		// If right (+ X), stepX is 1.
		if (rayDirX < 0.0f) {
			stepX = -1;
			sideDistX = (playerX - Tile_x * TILE_SIZE) * deltaDistX / TILE_SIZE;
		} else {
			stepX = 1;
			sideDistX = ((Tile_x + 1) * TILE_SIZE - playerX) * deltaDistX / TILE_SIZE;
		}

		if (rayDirY < 0.0f) {
			stepY = -1;
			sideDistY = (playerY - Tile_y * TILE_SIZE) * deltaDistY / TILE_SIZE;
		} else {
			stepY = 1;
			sideDistY = ((Tile_y + 1) * TILE_SIZE - playerY) * deltaDistY / TILE_SIZE;
		}

		// DDA loop
		while (hit == 0) {
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				Tile_x += stepX;
				side = 0;
			} else {
				sideDistY += deltaDistY;
				Tile_y += stepY;
				side = 1;
			}

			if (Tile_x >= 0 && Tile_x < MAP_COLS && Tile_y >= 0 && Tile_y < MAP_ROWS) {
				if (currentMap[Tile_y][Tile_x] != EMPTY) {
					hit = 1;
				}
			} else {
				break;
			}
			if (Tile_x >= 0 && Tile_x < MAP_COLS && Tile_y >= 0 && Tile_y < MAP_ROWS) {
				if (isWall(Tile_x, Tile_y)) {
					hit = 1;
				}
			} else {
				//To dont be infinit if I break this game
				break;
			}
		}

		// WALL IS A HIT
		// distance ray's angle is real now
		float perpWallDist;
		if (side == 0) {
			perpWallDist =
				(Tile_x * TILE_SIZE - playerX + (1.0f - stepX) * TILE_SIZE / 2.0f) /
				rayDirX;
		} else {
			perpWallDist =
				(Tile_y * TILE_SIZE - playerY + (1.0f - stepY) * TILE_SIZE / 2.0f) /
				rayDirY;
		}

		// If zero dont be zero
		if (perpWallDist == 0.0f)
			perpWallDist = 0.001f;

		// height of the wall slice
		// Tall walls are close short walls are far
		float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);

		// Start and end of the Drawing a wall
		int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
		int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

		// clamped
		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		uint16_t wallColor; // Deepseek told me
		if (side == 0) {
			wallColor = COLOR_WALL_BRIGHT;
		} else {
			wallColor = COLOR_WALL_DARK;
		}

		// DRAW WALLLLLLLLLLLLL
		// top to bottom.
		for (int y = drawStart; y <= drawEnd; y++) {
			tft_draw_pixel(x, y, wallColor);
		}

		// DRAW CEILING
		for (int y = 0; y < drawStart; y++) {
			tft_draw_pixel(x, y, COLOR_CEILING);
		}
		// DRAW FLOORRRRRRRR
		for (int y = drawEnd + 1; y < SCREEN_HEIGHT; y++) {
			tft_draw_pixel(x, y, COLOR_FLOOR);
		}
	}
}

void drawMinimap_Debug()
{
	// I took the minimap dimensions and calculated its top-left position for the bottom-right corner.
	int minimap_start_x = SCREEN_WIDTH - MINIMAP_WIDTH - MINIMAP_PADDING;
	int minimap_start_y = SCREEN_HEIGHT - MINIMAP_HEIGHT - MINIMAP_PADDING;

	// I'm drawing the map tiles.
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			uint16_t tileColor;
			// I checked if the current map tile is not empty.
			if (currentMap[y][x] != EMPTY) {
				tileColor = COLOR_MINIMAP_WALL; // White for walls
			} else {
				tileColor =
					COLOR_MINIMAP_EMPTY; // Black for empty spaces (the "rest" of the map)
			}
			for (int py = 0; py < MINIMAP_TILE_SIZE; py++) {
				for (int px = 0; px < MINIMAP_TILE_SIZE; px++) {
					tft_draw_pixel(minimap_start_x + x * MINIMAP_TILE_SIZE + px,
						       minimap_start_y + y * MINIMAP_TILE_SIZE + py,
						       tileColor);
				}
			}
		}
	}

	// Now, I'm drawing the player on the minimap.
	// I converted the player's world coordinates to minimap pixel coordinates.
	int minimap_player_x = (int)(playerX / TILE_SIZE * MINIMAP_TILE_SIZE);
	int minimap_player_y = (int)(playerY / TILE_SIZE * MINIMAP_TILE_SIZE);

	// I offset it by the minimap's starting position.
	minimap_player_x += minimap_start_x;
	minimap_player_y += minimap_start_y;

	// I draw a 3x3 yellow block for the player.
	for (int py = 0; py < 3; py++) {
		for (int px = 0; px < 3; px++) {
			tft_draw_pixel(
				minimap_player_x + px, minimap_player_y + py,
				COLOR_MINIMAP_PLAYER); // I took the pixel coordinates and drew a yellow pixel.
		}
	}

	// Now, I'm drawing the player's looking direction lines (FOV borders).
	// I calculated the center of the player's 3x3 block.
	float player_center_x = minimap_player_x + 1.0f; // Center of a 3x3 block (approx)
	float player_center_y = minimap_player_y + 1.0f; // Center of a 3x3 block (approx)

	// Define a small step size for tracing the FOV rays
	float step_size = 1.0f; // Step by 1 pixel at a time on the minimap

	// Trace Leftmost FOV ray
	float current_ray_x = player_center_x;
	float current_ray_y = player_center_y;
	float left_ray_angle = playerAngle - (FOV_RADIANS / 2.0f);
	float ray_dir_x = cosf(left_ray_angle);
	float ray_dir_y = sinf(left_ray_angle);

	// Now, I'm tracing the left FOV ray until it hits a wall or goes out of bounds.
	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		// Check if the current pixel is within the minimap boundaries.
		if (map_pixel_x < minimap_start_x ||
		    map_pixel_x >= minimap_start_x + MINIMAP_WIDTH ||
		    map_pixel_y < minimap_start_y ||
		    map_pixel_y >= minimap_start_y + MINIMAP_HEIGHT) {
			break; // Gone out of minimap bounds
		}

		// Convert minimap pixel to world map tile coordinates
		int tile_x = (map_pixel_x - minimap_start_x) / MINIMAP_TILE_SIZE;
		int tile_y = (map_pixel_y - minimap_start_y) / MINIMAP_TILE_SIZE;

		// Check if hit a wall in the actual world map
		if (tile_x >= 0 && tile_x < MAP_COLS && tile_y >= 0 && tile_y < MAP_ROWS &&
		    currentMap[tile_y][tile_x] != EMPTY) {
			break; // Hit a wall, stop drawing
		}
		tft_draw_pixel(map_pixel_x, map_pixel_y,
			       COLOR_MINIMAP_LINE); // I drew a green pixel for the line.

		current_ray_x += ray_dir_x * step_size; // Move the ray forward
		current_ray_y += ray_dir_y * step_size;
	}

	// Trace Rightmost FOV ray
	current_ray_x = player_center_x;
	current_ray_y = player_center_y;
	float right_ray_angle = playerAngle + (FOV_RADIANS / 2.0f);
	ray_dir_x = cosf(right_ray_angle);
	ray_dir_y = sinf(right_ray_angle);

	// Now, I'm tracing the right FOV ray until it hits a wall or goes out of bounds.
	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		// Check if the current pixel is within the minimap boundaries.
		if (map_pixel_x < minimap_start_x ||
		    map_pixel_x >= minimap_start_x + MINIMAP_WIDTH ||
		    map_pixel_y < minimap_start_y ||
		    map_pixel_y >= minimap_start_y + MINIMAP_HEIGHT) {
			break; // Gone out of minimap bounds
		}

		// Convert minimap pixel to world map tile coordinates
		int tile_x = (map_pixel_x - minimap_start_x) / MINIMAP_TILE_SIZE;
		int tile_y = (map_pixel_y - minimap_start_y) / MINIMAP_TILE_SIZE;

		// Check if hit a wall in the actual world map
		if (tile_x >= 0 && tile_x < MAP_COLS && tile_y >= 0 && tile_y < MAP_ROWS &&
		    currentMap[tile_y][tile_x] != EMPTY) {
			break; // Hit a wall, stop drawing
		}

		tft_draw_pixel(map_pixel_x, map_pixel_y,
			       COLOR_MINIMAP_LINE); // I drew a green pixel for the line.

		current_ray_x += ray_dir_x * step_size; // Move the ray forward
		current_ray_y += ray_dir_y * step_size;
	}
}
bool isWall(int Tile_x, int Tile_y)
{
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return true;
	}
	switch (currentMap[Tile_y][Tile_y]) {
	case WALL:
	default:
		return false; // I took this tile type and decided it is not a solid wall.
	}
}

void game_start(void)
{
	sdk_set_output_gain_db(volume);
	playerX = TILE_SIZE * 1.5f;
	playerY = TILE_SIZE * 1.5f;
	playerAngle = (float)M_PI / 2.0f;
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs.joy_y > 500) {
		playerX -= MOVE_SPEED * dt * cosf(playerAngle);
		playerY -= MOVE_SPEED * dt * sinf(playerAngle);
	} else if (sdk_inputs.joy_y < -500) {
		playerX += MOVE_SPEED * dt * cosf(playerAngle);
		playerY += MOVE_SPEED * dt * sinf(playerAngle);
	}

	if (sdk_inputs.joy_x > 500) {
		playerAngle += ROTATE_SPEED * dt;
	} else if (sdk_inputs.joy_x < -500) {
		playerAngle -= ROTATE_SPEED * dt;
	}

	if (sdk_inputs.a || sdk_inputs.start) {
		debug_mode = true;
	}
	game_handle_audio(dt, volume);
}

void game_paint(unsigned dt_usec)
{
	tft_fill(0);
	renderGame();
	if (debug_mode) {
		drawMinimap_Debug();
	}
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
