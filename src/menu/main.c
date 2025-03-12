#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <icons.png.h>

#define GRAY rgb_to_rgb565(63, 63, 63)

struct b {
	float s;
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
}

void game_paint(unsigned __unused dt_usec)
{
	sdk_draw_tile(0, 0, &ts_icons_png, 0);
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
