#include <sdk.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_draw_string(0, 0, WHITE, "tx=%6.4f  ty=%6.4f", sdk_inputs.tx, sdk_inputs.ty);
	tft_draw_string(0, 20, WHITE, "z1=%6.4f  z2=%6.4f", sdk_inputs.t1, sdk_inputs.t2);
	tft_draw_string(0, 40, WHITE, "pr=%6.4f", sdk_inputs.tp);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
