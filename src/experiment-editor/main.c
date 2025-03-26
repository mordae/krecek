#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <stdlib.h>
#include <stdio.h>

#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

#include <tileset.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

static uint8_t map[MAP_ROWS][MAP_COLS];

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
		map[sel_y][sel_x] = (map[sel_y][sel_x] + 1) % ts_tileset_png.count;
	}

	if (sdk_inputs_delta.y > 0) {
		map[sel_y][sel_x] = 0;
	}

	if (sdk_inputs_delta.start > 0) {
		printf("Map dump:\n");
		printf("---------\n");
		printf("{\n");
		for (int row = 0; row < MAP_ROWS; row++) {
			printf("\t{");
			for (int col = 0; col < MAP_COLS; col++) {
				if (col) {
					printf(", %i", map[row][col]);
				} else {
					printf("%i", map[row][col]);
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
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png, map[y][x]);
		}
	}

	tft_draw_rect(sel_x * TILE_SIZE + TILE_SIZE / 2.0 - 1,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0 - 1, sel_x * TILE_SIZE + TILE_SIZE / 2.0,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0, rgb_to_rgb565(255, 63, 63));
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
