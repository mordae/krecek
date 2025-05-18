#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

#include <mailbin.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

void game_start(void)
{
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs.a > 0 || sdk_inputs_delta.y > 0) {
		uint32_t len = remote_peek(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size));

		if (len) {
			puts("rf tx buffer is full");
		} else {
			uint8_t msg[8] = { 7, 255, 'H', 'e', 'l', 'l', 'o', '!' };
			remote_poke_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_buffer),
					 (void *)msg, sizeof(msg) / 4);
			remote_poke(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size),
				    sizeof(msg));
		}
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
