#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <icons.png.h>
#include <text.png.h>
#include <select.png.h>

#define GRAY rgb_to_rgb565(63, 63, 63)

// button
struct b {
	float s;  //select
	float g;  //game numer
	float wp; //wall pos
};

static struct b b;

// gotta add tileset for tilemap

typedef enum {
	FLOOR = 0,
	WALL = 1,
} TileType;

TileType map[15][20] = {
	{ 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 1 },
};

void game_reset(void)
{
	b.s = 0;
	b.g = 7;
	b.wp = 60;
}

void game_input(unsigned dt_usec)
{
	if (sdk_inputs_delta.b > 0 || sdk_inputs.joy_x > 300) {
		b.s += 1;
	} else if (sdk_inputs_delta.x > 0 || sdk_inputs.joy_x < -300) {
		b.s -= 1;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	sdk_draw_tile(0, 0, &ts_text_png, 0);
	sdk_draw_tile(35, 15, &ts_icons_png, b.s);
	sdk_draw_tile(-65, 20, &ts_icons_png, b.s - 1);
	sdk_draw_tile(135, 20, &ts_icons_png, b.s + 1);

	for (int i = b.wp; i < b.g * 4 + b.wp; i += 4) {
		sdk_draw_tile(i, 1, &ts_select_png, 0);
	}

	for (int i = b.wp; i < b.g * 4 + b.wp; i += 4) {
		if (b.s * 4 + b.wp == i) {
			sdk_draw_tile(i, 1, &ts_select_png, 1);
		}
	}

	if (b.s > b.g)
		b.s = 0;
	if (b.s < 0)
		b.s = b.g;
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
