#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

#include <mailbin.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

static int lx, ly, rx, ry;

static int channel = SDK_RF_CHANNEL;

void game_inbox(sdk_message_t msg)
{
	if (SDK_MSG_RF == msg.type) {
		if (2 != msg.rf.length) {
			printf("game_inbox: invalid RF len=%i\n", msg.rf.length);
			return;
		}

		rx = msg.rf.data[0];
		ry = msg.rf.data[1];
	}
}

static void tx_cursor(void)
{
	uint8_t msg[] = { lx, ly };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs.tp > 0.5f) {
		lx = sdk_inputs.tx * TFT_RIGHT;
		ly = sdk_inputs.ty * TFT_BOTTOM;

		tx_cursor();
	}

	if (sdk_inputs_delta.vertical < 0) {
		channel = clamp(channel + 1, SDK_RF_CHANNEL_MIN, SDK_RF_CHANNEL_MAX);
		sdk_set_rf_channel(channel);
	}

	if (sdk_inputs_delta.vertical > 0) {
		channel = clamp(channel - 1, SDK_RF_CHANNEL_MIN, SDK_RF_CHANNEL_MAX);
		sdk_set_rf_channel(channel);
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_draw_string(0, 0, rgb_to_rgb565(127, 0, 0), "%i", channel);

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
