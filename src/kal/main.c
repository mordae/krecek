#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <number.png.h>
#include <cover.png.h>
#include <buttons.png.h>
#include <buttonsselect.png.h>
#include <back.png.h>

sdk_game_info("kal", &image_cover_png);

#define GRAY rgb_to_rgb565(63, 63, 63)

static int position;

void game_reset(void)
{
	position = 5;
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	position = 5;

	if (sdk_inputs.joy_x > 300) {
		position = 6;
	} else if (sdk_inputs.joy_x < -300) {
		position = 4;
	} else if (sdk_inputs.joy_y > 300) {
		position = 8;
	} else if (sdk_inputs.joy_y < -300) {
		position = 2;
	}
	if (sdk_inputs.joy_x < -300 && sdk_inputs.joy_y < -300) {
		position = 1;
	} else if (sdk_inputs.joy_x > 300 && sdk_inputs.joy_y < -300) {
		position = 3;
	} else if (sdk_inputs.joy_x < -300 && sdk_inputs.joy_y > 300) {
		position = 7;
	} else if (sdk_inputs.joy_y > 300 && sdk_inputs.joy_x > 300) {
		position = 9;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(1);
	sdk_draw_tile(0, 0, &ts_back_png, 0);

	sdk_draw_tile(5, TFT_BOTTOM - 26, &ts_number_png, position - 1);

	for (int i = 35; i < 36; i++) {
		sdk_draw_tile(i, TFT_BOTTOM - 26, &ts_buttons_png, 0);
		sdk_draw_tile(i + 15 * 1, TFT_BOTTOM - 26, &ts_buttons_png, 1);
		sdk_draw_tile(i + 15 * 2, TFT_BOTTOM - 26, &ts_buttons_png, 2);
		sdk_draw_tile(i + 15 * 3, TFT_BOTTOM - 26, &ts_buttons_png, 3);
		sdk_draw_tile(i + 15 * 4, TFT_BOTTOM - 26, &ts_buttons_png, 4);
		sdk_draw_tile(i + 15 * 5, TFT_BOTTOM - 26, &ts_buttons_png, 5);
		sdk_draw_tile(i + 15 * 6, TFT_BOTTOM - 26, &ts_buttons_png, 6);
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
