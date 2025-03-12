#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <test.png.h>
#include <floor.png.h>
#include <wall.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

typedef enum { floor = 0, wall = 1 } TileType;

TileType map[15][20] = { { 1, 1, 1, 1, 1 } };

sdk_sprite_t test_sprite = {
	.ts = &ts_test_png,
	.x = 40,
	.y = 40,
	.ox = 2,
	.oy = 2,
};

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

static void draw_tile(TileType type, int x, int y)
{
	switch (type) {
	case floor:
		sdk_draw_tile(x, y, &ts_floor_png, 0);
		break;

	case wall:
		sdk_draw_tile(x, y, &ts_wall_png, 0);
		break;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	//test_sprite.x = 2;
	//test_sprite.y = 2;
	//sdk_draw_sprite(&test_sprite);

	//tft_draw_sprite(20, 20, image_test_png.width, image_test_png.height, image_test_png.data,
	//		TRANSPARENT);

	tft_fill(0);

	for (int x = 0; x < TFT_WIDTH / 8; x++) {
		for (int y = 0; y < TFT_HEIGHT / 8; y++) {
			draw_tile(map[y][x], x * 8, y * 8);
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
