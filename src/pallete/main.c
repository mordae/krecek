#include "sdk/input.h"
#include "sdk/util.h"
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdio.h>

#include <sdk.h>
#include <tft.h>

#define SPACE_SIZE 6
#define GRAY 8

static int cursor_x = 0;
static int cursor_y = 0;

void game_reset(void)
{
}

void game_start(void)
{
}

void game_input(unsigned __unused dt_usec)
{
	if (sdk_inputs_delta.a > 0) { // right
		cursor_y++;
	} else if (sdk_inputs_delta.b > 0) { // down
		cursor_x++;
	} else if (sdk_inputs_delta.y > 0) { // left
		cursor_y--;
	} else if (sdk_inputs_delta.x > 0) { // up
		cursor_x--;
	}
	cursor_x = clamp(cursor_x, 0, 15);
	cursor_y = clamp(cursor_y, 0, 15);
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int y = 0; y <= 15; y++) {
		for (int x = 0; x <= 15; x++) {
			tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
				      SPACE_SIZE * x + SPACE_SIZE - 1,
				      SPACE_SIZE * y + SPACE_SIZE - 1, (y * 16 + x));
		}
	}

	tft_draw_rect(SPACE_SIZE * cursor_x + 2, SPACE_SIZE * cursor_y + 2,
		      SPACE_SIZE * cursor_x + SPACE_SIZE - 3,
		      SPACE_SIZE * cursor_y + SPACE_SIZE - 3, (cursor_y * 16 + cursor_x) ^ 0xff);

	char buf[64];
	snprintf(buf, sizeof buf, "%i", (cursor_y * 16 + cursor_x));
	tft_draw_string_center(132, 0, GRAY, buf);

	tft_draw_rect(115, 50, 145, 80, (cursor_y * 16 + cursor_x));
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
