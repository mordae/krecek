#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <tft.h>
#include <sdk.h>

#include "../sdk/cc1101.h"

void game_start(void)
{
	cc1101_init();
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.start > 0) {
		uint8_t msg[] = { 14,  42,  'h', 'e', 'l', 'l', 'o', ',',
				  ' ', 'w', 'o', 'r', 'l', 'd', '!' };
		cc1101_transmit(msg, sizeof(msg));
	}
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
