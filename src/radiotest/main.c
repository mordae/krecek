#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>

#include "../sdk/cc1101.h"

#define WHITE rgb_to_rgb565(255, 255, 255)

void game_start(void)
{
	cc1101_init();
}

static float rssi;

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.start > 0) {
		uint8_t msg[] = { 14,  42,  'h', 'e', 'l', 'l', 'o', ',',
				  ' ', 'w', 'o', 'r', 'l', 'd', '!' };
		cc1101_transmit(msg, sizeof(msg));
		puts("transmitted");
	}

	if (sdk_inputs_delta.a > 0) {
		cc1101_receive();
		puts("receiving");
	}

	char buf[64];
	size_t len = 0;
	if (cc1101_poll(buf, &len))
		printf("received %zu\n", len);

	rssi = cc1101_get_rssi();
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_draw_string(0, 0, WHITE, "%5.1f", rssi);
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
