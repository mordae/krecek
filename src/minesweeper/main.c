#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

#define SPACE_1 239
#define SPACE_2 224
#define SPACE_3 226
#define SPACE_4 228
#define SPACE_5 230
#define SPACE_6 234
#define SPACE_7 335
#define SPACE_8 237
#define MINE 191
#define WHITE 15

#define SPACE_SIZE 8

static uint8_t board[240];
// 0 empty
// 1-9 mubers (0-8)
// 10 mine

void game_reset(void)
{
	for (int i = 0; i < 240; i++) {
		board[i] = 0;
	}
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int __unused nsamples)
{
}

void game_input(unsigned dt_usec)
{
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	// tft_draw_rect(SPACE_SIZE * fruit_x, SPACE_SIZE * fruit_y,
	// 	      SPACE_SIZE * fruit_x + SPACE_SIZE - 1, SPACE_SIZE * fruit_y + SPACE_SIZE - 1,
	// 	      fruit_color);
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
