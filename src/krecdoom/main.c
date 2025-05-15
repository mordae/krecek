#include "volume.h"
#include "common.h"

#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <tft.h>

// sdk_game_info("krecdoom", &image_cover_png);

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define MAP_WIDTH 8
#define MAP_HEIGHT 8

typedef struct {
	float x, y;
	float angle;
	float dx, dy;
	float nx, ny;
	float fov;
	float ray_angle;
	float ray_x, ray_y;
	float distance;
} Player;

static Player player;

typedef enum {
	EMPTY = 0,
	WALL = 1,
} TileType;

TileType world_map[MAP_HEIGHT][MAP_WIDTH] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 0, 0, 0, 1 }, { 1, 0, 1, 0, 1, 0, 0, 1 },
	{ 1, 0, 1, 0, 1, 0, 0, 1 }, { 1, 0, 0, 0, 0, 0, 0, 1 }, { 1, 0, 1, 1, 1, 1, 0, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1 },
};

float volume = 0;

void game_handle_audio(float dt, float volume);
static void tft_fill_rect(int x, int y, int w, int h, color_t color);
static void tft_draw_vline(int x, int y1, int y2, color_t color);

void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_handle_audio(dt, volume);

	float move_step = dt * 2.0f;
	float rot_step = dt * 2.5f;

	if (sdk_inputs.joy_x < -500) {
		player.angle -= rot_step;
	}
	if (sdk_inputs.joy_x > 500) {
		player.angle += rot_step;
	}
	player.dx = cosf(player.angle);
	player.dy = sinf(player.angle);

	if (sdk_inputs.joy_x < -500) {
		player.nx = player.x + player.dx * move_step;
		player.ny = player.y + player.dy * move_step;
		if (world_map[(int)player.ny][(int)player.nx] == 0) {
			player.x = player.nx;
			player.y = player.ny;
		}
	}
	if (sdk_inputs.joy_x > 500) {
		player.nx = player.x - player.dx * move_step;
		player.ny = player.y - player.dy * move_step;
		if (world_map[(int)player.ny][(int)player.nx] == 0) {
			player.x = player.nx;
			player.y = player.ny;
		}
	}
}
void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	tft_fill_rect(0, 0, 160, 60, rgb_to_rgb565(100, 100, 255));
	tft_fill_rect(0, 60, 160, 60, rgb_to_rgb565(50, 50, 50));

	player.fov = 0.8f;

	for (int x = 0; x < 160; x++) {
		player.ray_angle = player.angle - player.fov / 2 + (x / 160.0f) * player.fov;

		player.ray_x = cosf(player.ray_angle);
		player.ray_y = sinf(player.ray_angle);

		player.distance = 0.0f;

		while (player.distance < 16.0f) {
			float test_x = player.x + player.ray_x * player.distance;
			float test_y = player.y + player.ray_y * player.distance;

			if (test_x < 0 || test_y < 0 || test_x >= MAP_WIDTH || test_y >= MAP_HEIGHT)
				break;

			if (world_map[(int)test_y][(int)test_x] != 0)
				break;

			player.distance += 0.05f;
		}
		int wall_height = (int)(120.0f / player.distance);
		if (wall_height > 120)
			wall_height = 120;

		int start_y = (120 - wall_height) / 2;
		int end_y = start_y + wall_height;

		int shade = 255 - (int)(player.distance * 12.0f);
		if (shade < 0)
			shade = 0;
		color_t color = rgb_to_rgb565(shade, 0, 0);

		tft_draw_vline(x, start_y, end_y, color);
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
