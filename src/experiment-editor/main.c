#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../src/experiment/common.h"

#include <tileset.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

static Tile map[MAP_ROWS][MAP_COLS];

static int sel_x = 0;
static int sel_y = 0;

static float cur_x = 0;
static float cur_y = 0;

void game_start(void)
{
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	(void)dt;

	if (abs(sdk_inputs.joy_x) > 200) {
		cur_x += dt * sdk_inputs.joy_x / 192;
	}

	if (abs(sdk_inputs.joy_y) > 200) {
		cur_y += dt * sdk_inputs.joy_y / 192;
	}

	cur_x = clamp(cur_x, 0, MAP_COLS - 0.001);
	cur_y = clamp(cur_y, 0, MAP_ROWS - 0.001);

	if (abs(sdk_inputs.joy_x) < 200 && abs(sdk_inputs.joy_y) < 200) {
		cur_x = (int)cur_x + 0.5;
		cur_y = (int)cur_y + 0.5;
	}

	sel_x = cur_x;
	sel_y = cur_y;

	if (sdk_inputs_delta.a > 0) {
		map[sel_y][sel_x].tile_id = (map[sel_y][sel_x].tile_id + 1) % ts_tileset_png.count;
	}

	if (sdk_inputs_delta.y > 0) {
		map[sel_y][sel_x].u32 = 0;
	}

	if (sdk_inputs_delta.aux[1] > 0) {
		map[sel_y][sel_x].collides_left = !map[sel_y][sel_x].collides_left;
	}

	if (sdk_inputs_delta.aux[2] > 0) {
		map[sel_y][sel_x].collides_up = !map[sel_y][sel_x].collides_up;
	}

	if (sdk_inputs_delta.aux[3] > 0) {
		map[sel_y][sel_x].collides_down = !map[sel_y][sel_x].collides_down;
	}

	if (sdk_inputs_delta.aux[4] > 0) {
		map[sel_y][sel_x].collides_right = !map[sel_y][sel_x].collides_right;
	}

	if (sdk_inputs_delta.start > 0) {
		printf("Map dump:\n");
		printf("---------\n");
		printf("{\n");
		for (int row = 0; row < MAP_ROWS; row++) {
			printf("\t{");
			for (int col = 0; col < MAP_COLS; col++) {
				if (col) {
					printf(", 0x%08x", map[row][col].u32);
				} else {
					printf("0x%08x", map[row][col].u32);
				}
			}
			printf("},\n");
		}
		printf("}\n");
		printf("---------\n");
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(rgb_to_rgb565(0, 0, 0));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      map[y][x].tile_id);

			if (map[y][x].collides_left) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, x * TILE_SIZE,
					      (y + 1) * TILE_SIZE - 1, rgb_to_rgb565(255, 0, 0));
			}

			if (map[y][x].collides_right) {
				tft_draw_rect((x + 1) * TILE_SIZE - 1, y * TILE_SIZE,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}

			if (map[y][x].collides_up) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, (x + 1) * TILE_SIZE - 1,
					      y * TILE_SIZE, rgb_to_rgb565(255, 0, 0));
			}

			if (map[y][x].collides_down) {
				tft_draw_rect(x * TILE_SIZE, (y + 1) * TILE_SIZE - 1,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}
		}
	}

	tft_draw_rect(sel_x * TILE_SIZE + TILE_SIZE / 2.0 - 1,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0 - 1, sel_x * TILE_SIZE + TILE_SIZE / 2.0,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0, rgb_to_rgb565(255, 63, 63));

	char text[32] = "";

	snprintf(text, sizeof text, "%i", map[sel_y][sel_x].tile_id);

	if (sel_x >= MAP_COLS / 2) {
		if (sel_y >= MAP_ROWS / 2) {
			tft_draw_string_right(sel_x * TILE_SIZE + TILE_SIZE,
					      sel_y * TILE_SIZE - 2 * TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), text);
		} else {
			tft_draw_string_right(sel_x * TILE_SIZE + TILE_SIZE,
					      sel_y * TILE_SIZE + TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), text);
		}
	} else {
		if (sel_y >= MAP_ROWS / 2) {
			tft_draw_string(sel_x * TILE_SIZE, sel_y * TILE_SIZE - 2 * TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), text);
		} else {
			tft_draw_string(sel_x * TILE_SIZE, sel_y * TILE_SIZE + TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), text);
		}
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};
	sdk_main(&config);
}
