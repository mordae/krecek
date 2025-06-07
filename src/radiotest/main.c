#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

#include <mailbin.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

struct cursor {
	int id;
	int x, y;
};

#define NUM_CURSORS 8
struct cursor cursors[NUM_CURSORS];

color_t colors[NUM_CURSORS] = {
	rgb_to_rgb565(255, 0, 0),     rgb_to_rgb565(0, 0, 255),	    rgb_to_rgb565(255, 255, 0),
	rgb_to_rgb565(255, 0, 255),   rgb_to_rgb565(0, 255, 255),   rgb_to_rgb565(127, 127, 127),
	rgb_to_rgb565(127, 127, 255), rgb_to_rgb565(255, 127, 127),
};

static int lx, ly;
static uint8_t device_id_16;

static int channel = SDK_RF_CHANNEL;

void game_reset(void)
{
	device_id_16 = (0x9e3779b97f4a7801 * sdk_device_id) >> 48;

	for (int i = 0; i < NUM_CURSORS; i++)
		cursors[i].id = -1;
}

void game_inbox(sdk_message_t msg)
{
	if (SDK_MSG_RF == msg.type) {
		if (4 != msg.rf.length) {
			printf("game_inbox: invalid RF len=%i\n", msg.rf.length);
			return;
		}

		uint16_t id = (msg.rf.data[0] << 8) | msg.rf.data[1];

		for (int i = 0; i < NUM_CURSORS; i++) {
			if (cursors[i].id < 0 || cursors[i].id == id) {
				cursors[i].id = id;
				cursors[i].x = msg.rf.data[2];
				cursors[i].y = msg.rf.data[3];
				break;
			}
		}
	}
}

static void tx_cursor(void)
{
	static uint32_t last_tx;

	uint32_t now = time_us_32();

	if (now - last_tx < 30000)
		return;

	last_tx = now;

	uint8_t msg[] = { device_id_16 >> 8, device_id_16, lx, ly };
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

	for (int i = 0; i < NUM_CURSORS; i++) {
		if (cursors[i].id < 0)
			continue;

		int rx = cursors[i].x;
		int ry = cursors[i].y;
		tft_draw_rect(rx - 1, ry - 1, rx + 1, ry + 1, colors[i]);
	}

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
