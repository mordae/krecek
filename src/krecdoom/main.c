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

float playerAngle = (float)M_PI / 2.0f;

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];

TileType (*currentMap)[MAP_COLS] = maps_map1;

const uint16_t COLOR_CEILING = BLUE;
const uint16_t COLOR_FLOOR = RED;
const uint16_t COLOR_WALL_DARK = rgb_to_rgb565(0x69, 0x69, 0x69);
const uint16_t COLOR_WALL_BRIGHT = rgb_to_rgb565(0xA9, 0xA9, 0xA9);

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

float volume = 0.5f;

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
		playerX += MOVE_SPEED * dt * cosf(playerAngle);
		playerY += MOVE_SPEED * dt * sinf(playerAngle);
	} else if (sdk_inputs.joy_y < -500) {
		playerX -= MOVE_SPEED * dt * cosf(playerAngle);
		playerY -= MOVE_SPEED * dt * sinf(playerAngle);
	}

	if (sdk_inputs.joy_x > 500) {
		playerAngle += ROTATE_SPEED * dt;
	} else if (sdk_inputs.joy_x < -500) {
		playerAngle -= ROTATE_SPEED * dt;
	}
	game_handle_audio(dt, volume);
}

void game_paint(unsigned dt_usec)
{
	tft_fill(0);
	renderGame();
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
