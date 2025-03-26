#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include "common.h"

#include <tileset.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

#define TILE_SIZE 8

extern int maps_map1[MAP_ROWS][MAP_COLS];

static int (*map)[MAP_COLS] = maps_map1;

void game_reset(void)
{
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int nsamples)
{
	(void)nsamples;
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png, map[y][x]);
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
