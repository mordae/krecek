#include <ctype.h>
#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

#include <mailbin.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

static int lx, ly, rx, ry;

void game_start(void)
{
}

void game_inbox(sdk_message_t msg)
{
	if (msg.type != SDK_MSG_RF)
		return;

	printf("\x1b[42;30m[%02hhx] ", msg.rf.addr);

	for (int j = 0; j < msg.rf.length; j++)
		printf("%02hhx", msg.rf.data[j]);

	printf("  | ");

	for (int j = 0; j < msg.rf.length; j++) {
		char c = msg.rf.data[j];

		if (isalnum(c) || ispunct(c)) {
			putchar(c);
		} else {
			putchar('.');
		}
	}

	printf("\x1b[0m\n");

	rx = msg.rf.data[0];
	ry = msg.rf.data[1];
}

static void tx_cursor(void)
{
	int sizes[MAILBIN_RF_SLOTS];
	uint32_t addrs[MAILBIN_RF_SLOTS];

	remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size), (uint32_t *)sizes,
			 MAILBIN_RF_SLOTS);

	remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_addr), addrs,
			 MAILBIN_RF_SLOTS);

	for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
		if (sizes[i])
			continue;

		uint8_t buf[16];

		buf[0] = 3;
		buf[1] = 255;
		buf[2] = lx;
		buf[3] = ly;

		remote_poke_many(addrs[i], (void *)buf, 1);
		remote_poke(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size) + i * 4, 4);

		break;
	}
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs.tp > 0.5f) {
		lx = sdk_inputs.tx * TFT_RIGHT;
		ly = sdk_inputs.ty * TFT_BOTTOM;

		tx_cursor();
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_draw_rect(rx - 1, ry - 1, rx + 1, ry + 1, rgb_to_rgb565(255, 0, 0));
	tft_draw_rect(lx - 1, ly - 1, lx + 1, ly + 1, rgb_to_rgb565(0, 255, 0));
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
