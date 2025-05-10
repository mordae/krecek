#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <number.png.h>

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

	sdk_draw_tile(5, TFT_BOTTOM - 26, &ts_number_png, position - 1);

	/*
	sdk_draw_tile(31, TFT_BOTTOM - 26, &ts_buttons_png, 0);
	sdk_draw_tile(31 + 7 * 3, TFT_BOTTOM - 26, &ts_buttons_png, 2);
	sdk_draw_tile(31 + 14 * 3, TFT_BOTTOM - 26, &ts_buttons_png, 4);
	sdk_draw_tile(31 + 21 * 3, TFT_BOTTOM - 26, &ts_buttons_png, 6);
	sdk_draw_tile(31 + 28 * 3, TFT_BOTTOM - 26, &ts_buttons_png, 8);
	*/
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
