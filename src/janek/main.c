#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>

#include <tiles.png.h>

#define TILE_SIZE 8
#define MAP_ROWS 8
#define MAP_COLS 8
#define multiply332 \
	(x, f) rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define RED rgb_to_rgb565(255, 0, 0)
#define RED_POWER rgb_to_rgb565(255, 63, 63)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define GREEN_POWER rgb_to_rgb565(63, 255, 63)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

static float volume = 0;
typedef enum {
	GRASS = 0,
} TileType;

TileType map[7][10] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

};
void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void game_reset(void)
{
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	if (sdk_inputs.vol_up) {
		volume += 12.0 * dt;
	}

	if (sdk_inputs.vol_down) {
		volume -= 12.0 * dt;
	}

	if (sdk_inputs_delta.select > 0) {
		game_reset();
		return;
	}
	if (sdk_inputs_delta.vol_sw > 0) {
		if (volume < SDK_GAIN_MIN) {
			volume = 0;
		} else {
			volume = SDK_GAIN_MIN - 1;
		}
	}

	volume = clamp(volume, SDK_GAIN_MIN - 1.0, 6);

	if (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
		sdk_set_output_gain_db(volume);
	}
}
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_png, map[y][x] - 1);
			//	if (!map[y][x]) {
			//		tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE,
			//			      x * TILE_SIZE + TILE_SIZE - 1,
			//			      y * TILE_SIZE + TILE_SIZE - 1,
			//			      rgb_to_rgb565(0, 0, 0));
			//		continue;
			//	}
		}
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = GRAY,
	};
	sdk_main(&config);
}
