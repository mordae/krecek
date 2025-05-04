#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <icons.png.h>
#include <text.png.h>
#include <select.png.h>

#define GRAY rgb_to_rgb565(63, 63, 63)

#define NUM_GAMES 12

int selected = 11;

void game_reset(void)
{
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.horizontal > 0) {
		selected = (selected + 1) % NUM_GAMES;
	}

	if (sdk_inputs_delta.x > 0 || sdk_inputs_delta.horizontal < 0) {
		selected = (NUM_GAMES + selected - 1) % NUM_GAMES;
	}

	if (selected != (NUM_GAMES - 1)) {
		if (sdk_inputs_delta.start > 0) {
			sdk_reboot_into_slot(selected + 1);
		}
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	if (selected != (NUM_GAMES - 1))
		sdk_draw_tile(1, 0, &ts_text_png, 0);

	int prev = (NUM_GAMES + selected - 1) % NUM_GAMES;
	int next = (selected + 1) % NUM_GAMES;

	sdk_draw_tile(35, 15, &ts_icons_png, selected);
	sdk_draw_tile(-65, 20, &ts_icons_png, prev);
	sdk_draw_tile(135, 20, &ts_icons_png, next);

	int dots_left = (TFT_WIDTH / 2) - (NUM_GAMES * 4 / 2);

	for (int i = 0; i < NUM_GAMES; i++)
		sdk_draw_tile(dots_left + i * 4, 120 - 5, &ts_select_png, selected == i);
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
