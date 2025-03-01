#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

// gotta add tileset for tilemap

typedef enum { floor = 0, wall = 1 } TileType;

TileType map[15][20] = { { 0, 0, 0, 0, 0 }, { 1, 1, 1, 1, 1 } };

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
