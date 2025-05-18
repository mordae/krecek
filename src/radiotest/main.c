#include <ctype.h>
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
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs.a > 0 || sdk_inputs_delta.y > 0) {
		int sizes[MAILBIN_RF_SLOTS];
		uint32_t addrs[MAILBIN_RF_SLOTS];

		remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size),
				 (uint32_t *)sizes, MAILBIN_RF_SLOTS);

		remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_addr), addrs,
				 MAILBIN_RF_SLOTS);

		bool sent = false;

		for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
			if (sizes[i])
				continue;

			static int counter = 0;

			char buf[32];
			int len = sprintf(buf + 2, "Hello, %i!", counter++);

			buf[0] = len + 1;
			buf[1] = 255;

			remote_poke_many(addrs[i], (void *)buf, (len + 2 + 3) >> 2);
			remote_poke(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size) + i * 4,
				    len + 2);

			sent = true;
			break;
		}

		if (!sent)
			puts("no free rf tx buffer");
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
