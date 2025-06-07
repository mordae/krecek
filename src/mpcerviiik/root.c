#include <sdk.h>

#include "common.h"

#define WHITE rgb_to_rgb565(255, 255, 255)
#define LGRAY rgb_to_rgb565(192, 192, 192)
#define BLUE rgb_to_rgb565(0, 0, 255)

extern sdk_scene_t scene_round;

GameState game_state;
Player players[NUM_PLAYERS];
int game_slot;

static uint16_t our_id;

static void tx_beacon(void)
{
	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx > 250000) {
		uint8_t msg[] = { 0xbc, our_id >> 8, our_id };
		sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
		last_tx = now;
	}
}

static void root_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth)
		return;

	tft_fill(0);

	if (STATE_LOBBY == game_state) {
		tx_beacon();
		tft_draw_string(0, 0, LGRAY, "Scanning...");
		tft_draw_string(0, 20, LGRAY, "Press START to host.");

		for (int i = 0; i < NUM_PLAYERS; i++) {
			if (!players[i].id)
				continue;

			tft_draw_string(0, 40 + i * 20, players[i].color, "%04x", players[i].id);
		}
	} else {
		tft_draw_string(0, 0, LGRAY, "Starting...");
	}
}

static bool root_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	switch (event) {
	case SDK_PRESSED_START:
		game_state = STATE_HOSTING;
		sdk_scene_push(&scene_round);
		return true;

	default:
		return false;
	}
}

static color_t id_to_color(uint16_t id)
{
	if (id & 0b1000010000010000)
		return id;

	return ~id;
}

static bool root_inbox(sdk_message_t msg, int depth)
{
	if (depth || msg.type != SDK_MSG_RF)
		return false;

	if (3 == msg.rf.length && 0xbc == msg.rf.data[0]) {
		int id = (msg.rf.data[1] << 8) | msg.rf.data[2];

		for (int i = 1; i < NUM_PLAYERS; i++) {
			if (!players[i].id || players[i].id == id) {
				players[i].id = id;
				players[i].color = id_to_color(id);
				break;
			}
		}

		return true;
	}

	if (9 == msg.rf.length && 0xbe == msg.rf.data[0]) {
		for (int i = 0; i < NUM_PLAYERS; i++) {
			uint16_t id = (msg.rf.data[1 + i * 2] << 8) | msg.rf.data[2 + i * 2];

			if (id == our_id) {
				game_state = STATE_PLAYING;
				game_slot = i;
				sdk_scene_push(&scene_round);
			}

			players[i].id = id;
			players[i].color = id_to_color(id);
		}

		return true;
	}

	return false;
}

static void root_revealed(void)
{
	game_state = STATE_LOBBY;

	for (int i = 0; i < NUM_PLAYERS; i++)
		players[i].id = 0;

	players[0].id = our_id;
	players[0].color = id_to_color(our_id);

	game_slot = 0;
}

static void root_pushed(void)
{
	our_id = (sdk_device_id * 0x9e3779b97f4a77ff) >> 48;

	if (!our_id)
		our_id = ~0;

	sdk_set_rf_channel(44);
	root_revealed();
}

sdk_scene_t scene_root = {
	.paint = root_paint,
	.handle = root_handle,
	.pushed = root_pushed,
	.revealed = root_revealed,
	.inbox = root_inbox,
};
