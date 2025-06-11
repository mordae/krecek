#include <limits.h>

#include "common.h"

#define WHITE rgb_to_rgb565(255, 255, 255)
#define LGRAY rgb_to_rgb565(192, 192, 192)
#define BLUE rgb_to_rgb565(0, 0, 255)

extern sdk_scene_t scene_round;

GameState game_state;
int game_state_timer;
uint16_t game_our_id;

Player players[NUM_PLAYERS];

static void tx_beacon(void)
{
	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < 250000)
		return;

	last_tx = now;

	uint8_t msg[] = { MSG_BEACON, game_our_id >> 8, game_our_id };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	last_tx = now;
}

static void tx_begin(void)
{
	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < 100000) {
		return;
	}

	last_tx = now;

	for (int i = NUM_WORMS; i < NUM_PLAYERS; i++) {
		if (!players[i].id)
			continue;

		for (int j = 1; j < NUM_WORMS; j++) {
			if (players[j].id)
				continue;

			memcpy(&players[j], &players[i], sizeof(Player));
			memset(&players[i], 0, sizeof(Player));
			break;
		}
	}

	for (int i = 0; i < NUM_WORMS; i++)
		players[i].worm = i;

	uint8_t msg[] = {
		MSG_BEGIN,			   //
		players[0].id >> 8, players[0].id, //
		players[1].id >> 8, players[1].id, //
		players[2].id >> 8, players[2].id, //
		players[3].id >> 8, players[3].id, //
	};

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
}

static void root_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth)
		return;

	tft_fill(0);

	switch (game_state) {
	case STATE_LOBBY:
		tft_draw_string(0, 0, LGRAY, "Scanning...");
		tft_draw_string(0, 20, LGRAY, "Press START to host.");
		break;

	case STATE_JOINING:
		tft_draw_string(0, 0, LGRAY, "Joining...");
		break;

	case STATE_HOSTING:
		tft_draw_string(0, 0, LGRAY, "Hosting...");

	default:
		break;
	}

	static_assert(NUM_PLAYERS == 8);

	for (int i = 0; i < 4; i++) {
		if (players[i].id)
			tft_draw_string(0, 40 + i * 20, players[i].color, "%04x", players[i].id);

		if (players[4 + i].id)
			tft_draw_string(80, 40 + i * 20, players[4 + i].color, "%04x",
					players[4 + i].id);
	}
}

static bool root_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	switch (event) {
	case SDK_PRESSED_START:
		game_state_change(STATE_HOSTING);
		tx_begin();
		return true;

	default:
		return false;
	}
}

static void root_tick(int jiffies, int depth)
{
	int new_time = game_state_timer + jiffies;
	game_state_timer = new_time >= 0 ? new_time : INT_MAX;

	if (depth)
		return;

	for (int i = 1; i < NUM_PLAYERS; i++) {
		players[i].timeout -= jiffies;

		if (players[i].timeout < 0)
			players[i].id = 0;
	}

	switch (game_state) {
	case STATE_LOBBY:
		tx_beacon();
		break;

	case STATE_JOINING:
		tx_beacon();

		if (game_state_timer >= 1000000) {
			game_state_change(STATE_PLAYING);
			sdk_scene_push(&scene_round);
		}

		break;

	case STATE_HOSTING:
		tx_begin();

		if (game_state_timer >= 1000000) {
			game_state_change(STATE_PLAYING);
			sdk_scene_push(&scene_round);
		}

		break;

	default:
		break;
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

	if (3 == msg.rf.length && MSG_BEACON == msg.rf.data[0]) {
		int id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		Player *player = find_player(id);

		player->id = id;
		player->color = id_to_color(id);
		player->timeout = PLAYER_TIMEOUT;
		return true;
	}

	if (9 == msg.rf.length && MSG_BEGIN == msg.rf.data[0]) {
		game_state_change(STATE_JOINING);

		for (int i = 0; i < NUM_PLAYERS; i++)
			players[i].worm = -1;

		for (int i = 0; i < NUM_WORMS; i++) {
			uint16_t id = (msg.rf.data[1 + i * 2] << 8) | msg.rf.data[2 + i * 2];
			Player *player = find_player(id);

			player->id = id;
			player->color = id_to_color(id);
			player->timeout = PLAYER_TIMEOUT;
			player->worm = i;
		}

		return true;
	}

	return false;
}

static void root_revealed(void)
{
	game_state_change(STATE_LOBBY);

	players[0].id = game_our_id;
	players[0].color = id_to_color(game_our_id);

	for (int i = 1; i < NUM_PLAYERS; i++)
		players[i].id = 0;
}

static void root_pushed(void)
{
	game_our_id = (sdk_device_id * 0x9e3779b97f4a77ff) >> 48;

	if (!game_our_id)
		game_our_id = ~0;

	sdk_set_rf_channel(44);
	root_revealed();
}

sdk_scene_t scene_root = {
	.paint = root_paint,
	.handle = root_handle,
	.tick = root_tick,
	.pushed = root_pushed,
	.revealed = root_revealed,
	.inbox = root_inbox,
};
