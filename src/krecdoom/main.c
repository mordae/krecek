#include "volume.h"
#include "common.h"
#include "maps.h"
#include "player.h"

#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <tft.h>

// sdk_game_info("krecdoom", &image_cover_png);

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)

TileType (*map)[MAP_WIDTH] = maps_map1;
Player player;

float volume = 0;

void game_handle_audio(float dt, float volume);
void game_player_inputs(float dt);

static void tft_fill_rect(int x, int y, int w, int h, color_t color);
static void tft_draw_vline(int x, int y1, int y2, color_t color);
static void game_map(int map_change);

void game_start(void)
{
	sdk_set_output_gain_db(volume);
	player.x = 1.5f;
	player.y = 1.5f;
	player.angle = 0.0f;
	player.fov = 0.8f;
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_handle_audio(dt, volume);

	int map_change = 0;
	if (sdk_inputs_delta.a == 1)
		map_change = 1;

	game_player_inputs(dt);

	game_map(map_change);
}
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill_rect(0, 0, 160, 60, rgb_to_rgb565(100, 100, 255));
	tft_fill_rect(0, 60, 160, 60, rgb_to_rgb565(50, 50, 50));

	for (int x = 0; x < 160; x++) {
		float cameraX = 2.0f * x / 160.0f - 1.0f;

		float rayDirX = cosf(player.angle) + sinf(player.angle) * cameraX * player.fov;
		float rayDirY = sinf(player.angle) - cosf(player.angle) * cameraX * player.fov;

		int mapX = (int)player.x;
		int mapY = (int)player.y;

		float deltaDistX = fabsf(1.0f / rayDirX);
		float deltaDistY = fabsf(1.0f / rayDirY);

		float sideDistX, sideDistY;
		int stepX, stepY;

		if (rayDirX < 0) {
			stepX = -1;
			sideDistX = (player.x - mapX) * deltaDistX;
		} else {
			stepX = 1;
			sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
		}

		if (rayDirY < 0) {
			stepY = -1;
			sideDistY = (player.y - mapY) * deltaDistY;
		} else {
			stepY = 1;
			sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
		}

		int hit = 0;
		int side = 0;
		while (!hit && mapX >= 0 && mapY >= 0 && mapX < MAP_WIDTH && mapY < MAP_HEIGHT) {
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
			} else {
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			}
			if (map[mapY][mapX] == WALL)
				hit = 1;
		}

		float perpWallDist = side == 0 ? (sideDistX - deltaDistX) :
						 (sideDistY - deltaDistY);
		if (perpWallDist < 0.01f)
			perpWallDist = 0.01f;

		int lineHeight = (int)(120.0f / perpWallDist);
		if (lineHeight > 120)
			lineHeight = 120;

		int drawStart = (120 - lineHeight) / 2;
		int drawEnd = drawStart + lineHeight;

		int shade = 255 - (int)(perpWallDist * 12.0f);
		if (shade < 0)
			shade = 0;

		color_t color;
		if (side == 1) {
			color = rgb_to_rgb565(shade / 2, 0, 0);
		} else {
			color = rgb_to_rgb565(shade, 0, 0);
		}
		tft_draw_vline(x, drawStart, drawEnd, color);
	}
}

static void tft_fill_rect(int x, int y, int w, int h, color_t color)
{
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			tft_draw_pixel(x + j, y + i, color);
		}
	}
}

static void tft_draw_vline(int x, int y1, int y2, color_t color)
{
	if (y1 > y2) {
		int tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	for (int y = y1; y <= y2; y++) {
		tft_draw_pixel(x, y, color);
	}
}
static void game_map(int map_change)
{
	if (map_change > 0) {
		if (map == maps_map2) {
			map = maps_map1;
		} else if (map == maps_map1) {
			map = maps_map2;
		}
	}

	if (map_change < 0) {
		if (map == maps_map2) {
			map = maps_map1;
		} else if (map == maps_map1) {
			map = maps_map2;
		}
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
}
