#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <number.png.h>
#include <cover.png.h>
#include <buttons.png.h>
#include <buttonsselect.png.h>
#include <back.png.h>
#include <arrow.png.h>

sdk_game_info("kal", &image_cover_png);

#define GRAY rgb_to_rgb565(63, 63, 63)

#define BUTTONS 7
struct pos {
	int num;
	int oper;
	int x;
};
static struct pos pos;

struct val {
	double up;
	double down;
	double out;
};
static struct val val;

void game_reset(void)
{
	pos.num = 5;
	pos.oper = 0;
	pos.x = 0;

	val.up = 1;
	val.down = 2;
	val.out = 111111111;
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	pos.num = 5;

	// -- 1-9 butons --
	if (sdk_inputs.joy_x > 300) {
		pos.num = 6;
	} else if (sdk_inputs.joy_x < -300) {
		pos.num = 4;
	} else if (sdk_inputs.joy_y > 300) {
		pos.num = 8;
	} else if (sdk_inputs.joy_y < -300) {
		pos.num = 2;
	}
	if (sdk_inputs.joy_x < -300 && sdk_inputs.joy_y < -300) {
		pos.num = 1;
	} else if (sdk_inputs.joy_x > 300 && sdk_inputs.joy_y < -300) {
		pos.num = 3;
	} else if (sdk_inputs.joy_x < -300 && sdk_inputs.joy_y > 300) {
		pos.num = 7;
	} else if (sdk_inputs.joy_y > 300 && sdk_inputs.joy_x > 300) {
		pos.num = 9;
	}

	// -- 1-9 add --
	if (sdk_inputs_delta.a > 0) {
		if (pos.x == 0) {
			if (pos.num == 1) {
				val.up *= 10;
				val.up += 1;
			} else if (pos.num == 2) {
				val.up *= 10;
				val.up += 2;
			} else if (pos.num == 3) {
				val.up *= 10;
				val.up += 3;
			} else if (pos.num == 4) {
				val.up *= 10;
				val.up += 4;
			} else if (pos.num == 5) {
				val.up *= 10;
				val.up += 5;
			} else if (pos.num == 6) {
				val.up *= 10;
				val.up += 6;
			} else if (pos.num == 7) {
				val.up *= 10;
				val.up += 7;
			} else if (pos.num == 8) {
				val.up *= 10;
				val.up += 8;
			} else if (pos.num == 9) {
				val.up *= 10;
				val.up += 9;
			}
		} else if (pos.x == 1) {
			if (pos.num == 1) {
				val.down *= 10;
				val.down += 1;
			} else if (pos.num == 2) {
				val.down *= 10;
				val.down += 2;
			} else if (pos.num == 3) {
				val.down *= 10;
				val.down += 3;
			} else if (pos.num == 4) {
				val.down *= 10;
				val.down += 4;
			} else if (pos.num == 5) {
				val.down *= 10;
				val.down += 5;
			} else if (pos.num == 6) {
				val.down *= 10;
				val.down += 6;
			} else if (pos.num == 7) {
				val.down *= 10;
				val.down += 7;
			} else if (pos.num == 8) {
				val.down *= 10;
				val.down += 8;
			} else if (pos.num == 9) {
				val.down *= 10;
				val.down += 9;
			}
		}
	}

	// <-/->
	if (sdk_inputs_delta.x > 0 && pos.oper > 0) {
		pos.oper -= 1;
	} else if (sdk_inputs_delta.b > 0 && pos.oper < BUTTONS - 1) {
		pos.oper += 1;
	}

	// -- oper button logika
	if (sdk_inputs_delta.y > 0) {
		if (pos.oper == 0) {
			if (sdk_inputs_delta.a > 0) {
				if (pos.x == 0) {
					val.up *= 10;
					val.up += 9;
				} else if (pos.x == 1) {
					val.down *= 10;
					val.down += 9;
				}
			}
		} else if (pos.oper == 1) {
			if (pos.x == 0) {
				val.up = 0;
			} else if (pos.x == 1) {
				val.down = 0;
			}
		} else if (pos.oper == 2) {
			val.out = val.up + val.down;
		} else if (pos.oper == 3) {
			val.out = val.up - val.down;
		} else if (pos.oper == 4) {
			val.out = val.up * val.down;
		} else if (pos.oper == 5) {
			val.out = val.up / val.down;
		} else if (pos.oper == 6) {
			if (pos.x == 0) {
				pos.x = 1;
			} else if (pos.x == 1) {
				pos.x = 0;
			}

		} else {
		}
	}
}

void game_paint(unsigned __unused dt_usec)
{
	// -- pozadi --
	tft_fill(1);
	sdk_draw_tile(0, 0, &ts_back_png, 0);

	// -- cisla --
	sdk_draw_tile(5, TFT_BOTTOM - 21 - 5, &ts_number_png, pos.num - 1);

	// -- operacni tlacitka --
	for (int i = 0; i < BUTTONS; i++)
		sdk_draw_tile(31 + i * 15, TFT_BOTTOM - 26, &ts_buttons_png, i);
	sdk_draw_tile(31 + pos.oper * 15, TFT_BOTTOM - 26, &ts_buttonsselect_png, pos.oper);

	// -- pomocne cary-
	tft_draw_rect(10, 60, TFT_RIGHT - 10, 65, rgb_to_rgb565(128, 128, 128));
	tft_draw_rect(15, 30, TFT_RIGHT - 15, 32, rgb_to_rgb565(64, 64, 64));

	// -- hodnoty --
	tft_draw_string_right(TFT_RIGHT - 20, 17, rgb_to_rgb565(255, 255, 255), "%1.0f", val.up);
	tft_draw_string_right(TFT_RIGHT - 20, 47, rgb_to_rgb565(255, 255, 255), "%1.0f", val.down);
	tft_draw_string_right(TFT_RIGHT - 20, 67, rgb_to_rgb565(255, 255, 255), "%1.2f", val.out);

	// -- arrow draw --
	if (pos.x == 0) {
		sdk_draw_tile(TFT_RIGHT - 20, 20, &ts_arrow_png, 0);
	} else if (pos.x == 1) {
		sdk_draw_tile(TFT_RIGHT - 20, 50, &ts_arrow_png, 0);
	}

	// -- debug --
	tft_draw_string(0, 0, rgb_to_rgb565(255, 255, 255), "%1.0i", pos.num);
	tft_draw_string(0, 15, rgb_to_rgb565(255, 255, 255), "%1.0i", pos.oper);
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
