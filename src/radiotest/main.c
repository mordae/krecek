#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <tft.h>
#include <sdk.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

void game_start(void)
{
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);
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
