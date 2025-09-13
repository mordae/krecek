// root.c
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sdk.h>
#include "common.h"

extern sdk_scene_t scene_round;

GameState game_state;
int game_state_timer;
uint16_t game_our_id;
Player players[NUM_PLAYERS];
int current_rf_channel;
int channel_scan_index;
uint16_t scanned_channel_occupancy[3];
int selected_channel_index;

sdk_melody_t *menu_melody = NULL;
sdk_melody_t *game_melody = NULL;

static const char menu_music_str[] =
	"/i:square /bpm:60 /pl"
	"{"
	"F _ F _ F _ E _ D _ C _ D _ E _ F _ G _ A _ G _ F _ E _ D _ C _"
	"F _ F _ F _ E _ D _ C _ D _ E _ F _ G _ A _ G _ F _ E _ D _ C _"
	"A _ G _ F _ E _ D _ C _ D _ E _ F _ G _ A _ G _ F _ E _ D _ C _"
	"G _ F _ E _ D _ C _ B _ C _ D _ E _ F _ G _ F _ E _ D _ C _ B _"
	"}";

static const char game_music_str[] = "/i:square /bpm:120 /pl"
				     "{"
				     "C E G C5 | E5 G C6 E6 | G C7 E7 G7 | C8 }"
				     "}";

static const int channels_to_scan[] = { SDK_RF_CHANNEL_66, SDK_RF_CHANNEL_67, SDK_RF_CHANNEL_68 };
#define NUM_SCAN_CHANNELS (sizeof(channels_to_scan) / sizeof(channels_to_scan[0]))

static bool client_game_found_on_channel = false;
static bool host_channel_is_busy_during_scan = false;

static uint32_t last_joystick_menu_move_time = 0;
#define JOYSTICK_MENU_DEBOUNCE_US 200000

static color_t id_to_color(uint16_t id);
static void tx_ready(void);
static void tx_game_start(void);
static void tx_beacon(void);
static void tx_begin(void);

void game_state_change(GameState new_state)
{
	game_state = new_state;
	game_state_timer = 0;
	printf("DEBUG: Game state changed to %d. Timer reset.\n", new_state);
}

Player *find_player(uint16_t id)
{
	for (int i = 0; i < NUM_ACTIVE_PLAYERS; i++) {
		if (players[i].id == id) {
			return &players[i];
		}
	}

	for (int i = 0; i < NUM_ACTIVE_PLAYERS; i++) {
		if (players[i].id == 0) {
			return &players[i];
		}
	}

	return NULL;
}

static void tx_beacon(void)
{
	if (game_state >= STATE_ROUND_RESET && game_state != STATE_CLIENT_CHANNEL_SCAN)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < BEACON_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	uint8_t msg[] = { MSG_BEACON, game_our_id >> 8, (uint8_t)game_our_id };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
}

static void tx_begin(void)
{
	if (game_state != STATE_HOSTING && game_state != STATE_READY_WAIT)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < BEGIN_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	uint8_t msg[1 + 2 * NUM_ACTIVE_PLAYERS];
	msg[0] = MSG_BEGIN;
	msg[1] = players[0].id >> 8;
	msg[2] = (uint8_t)players[0].id;
	msg[3] = players[1].id >> 8;
	msg[4] = (uint8_t)players[1].id;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent BEGIN (P1: %04x, P2: %04x) on channel %d\n", players[0].id,
	       players[1].id, current_rf_channel);
}

static void tx_ready(void)
{
	if (game_state >= STATE_ROUND_RESET)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < READY_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	uint8_t msg[] = { MSG_READY, game_our_id >> 8, (uint8_t)game_our_id };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent READY (ID: %04x) on channel %d\n", game_our_id, current_rf_channel);
}

static void tx_game_start(void)
{
	if (game_state >= STATE_ROUND_RESET)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < GAME_START_TX_INTERVAL_US) {
		return;
	}
	last_tx = now;

	uint8_t msg[] = { MSG_GAME_START };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent GAME_START message on channel %d.\n", current_rf_channel);
}

static void root_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth) {
		return;
	}

	tft_fill(BLACK);

	switch (game_state) {
	case STATE_MENU:
		tft_draw_string(0, 0, WHITE, "--- Main Menu ---");
		tft_draw_string(0, 40, LGRAY, "Host Game (A)");
		tft_draw_string(0, 60, LGRAY, "Join Game (B)");
		break;

	case STATE_HOST_CHANNEL_SCAN:
		tft_draw_string(0, 0, LGRAY, "Scanning for clean channel...");
		tft_draw_string(0, 20, LGRAY, "Checking Channel: %d", current_rf_channel);
		break;

	case STATE_CLIENT_CHANNEL_SCAN:
		tft_draw_string(0, 0, LGRAY, "Scanning for game...");
		tft_draw_string(0, 20, LGRAY, "Checking Channel: %d", current_rf_channel);
		break;

	case STATE_ANTENNA_MENU: {
		const char *title_text = "--- Choose Channel ---";
		int title_width_pixels = strlen(title_text) * 6;
		tft_draw_string((TFT_WIDTH - title_width_pixels) / 2, 0, WHITE, title_text);

		int rect_width = 100;
		int rect_height = 30;
		int gap = 5;
		int start_x_rect = (TFT_WIDTH - rect_width) / 2;
		int start_y_rect = 20;

		int dot_size = 10;

		for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
			int y_pos = start_y_rect + i * (rect_height + gap);
			color_t rect_color = LGRAY;
			char channel_status_text[20];

			if (scanned_channel_occupancy[i] == 0) {
				snprintf(channel_status_text, sizeof(channel_status_text), "Clean");
			} else {
				snprintf(channel_status_text, sizeof(channel_status_text), "%04x",
					 scanned_channel_occupancy[i]);
			}

			tft_draw_rect(start_x_rect, y_pos, start_x_rect + rect_width - 1,
				      y_pos + rect_height - 1, rect_color);

			char channel_num_text[5];
			snprintf(channel_num_text, sizeof(channel_num_text), "%d:", i + 1);
			tft_draw_string(start_x_rect + 5, y_pos + 10, WHITE, channel_num_text);

			int status_text_x = start_x_rect + 5 + (strlen(channel_num_text) * 6) + 5;
			tft_draw_string(status_text_x, y_pos + 10, WHITE, channel_status_text);

			if (i == selected_channel_index) {
				tft_draw_rect(start_x_rect - dot_size - 5,
					      y_pos + (rect_height / 2) - (dot_size / 2),
					      start_x_rect - 5 - 1,
					      y_pos + (rect_height / 2) + (dot_size / 2) - 1,
					      WHITE);
			}
		}
		break;
	}

	case STATE_CLIENT_CHANNEL_SELECT: {
		const char *title_text = "--- Join Channel ---";
		int title_width_pixels = strlen(title_text) * 6;
		tft_draw_string((TFT_WIDTH - title_width_pixels) / 2, 0, WHITE, title_text);

		int rect_width = 100;
		int rect_height = 30;
		int gap = 5;
		int start_x_rect = (TFT_WIDTH - rect_width) / 2;
		int start_y_rect = 20;

		int dot_size = 10;

		for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
			int y_pos = start_y_rect + i * (rect_height + gap);
			color_t rect_color = LGRAY;
			char channel_status_text[20];

			if (scanned_channel_occupancy[i] != 0) {
				snprintf(channel_status_text, sizeof(channel_status_text),
					 "Game Found");
			} else {
				snprintf(channel_status_text, sizeof(channel_status_text),
					 "No Game");
			}

			tft_draw_rect(start_x_rect, y_pos, start_x_rect + rect_width - 1,
				      y_pos + rect_height - 1, rect_color);

			char channel_num_text[5];
			snprintf(channel_num_text, sizeof(channel_num_text), "%d:", i + 1);
			tft_draw_string(start_x_rect + 5, y_pos + 10, WHITE, channel_num_text);

			int status_text_x = start_x_rect + 5 + (strlen(channel_num_text) * 6) + 5;
			tft_draw_string(status_text_x, y_pos + 10, WHITE, channel_status_text);

			if (i == selected_channel_index) {
				tft_draw_rect(start_x_rect - dot_size - 5,
					      y_pos + (rect_height / 2) - (dot_size / 2),
					      start_x_rect - 5 - 1,
					      y_pos + (rect_height / 2) + (dot_size / 2) - 1,
					      WHITE);
			}
		}
		break;
	}

	case STATE_LOBBY:
		tft_draw_string(0, 0, LGRAY, "Scanning for host...");
		tft_draw_string(0, 20, LGRAY, "Waiting for game to start.");
		break;

	case STATE_JOINING:
		tft_draw_string(0, 0, LGRAY, "Game found! Press START.");
		tft_draw_string(0, 20, LGRAY, "Host: P1 %04x", players[0].id);
		tft_draw_string(0, 60, LGRAY, "Channel: %d", current_rf_channel);
		char role_text[20];
		if (game_our_id == players[0].id) {
			snprintf(role_text, sizeof(role_text), "P1");
		} else if (game_our_id == players[1].id) {
			snprintf(role_text, sizeof(role_text), "P2");
		} else {
			snprintf(role_text, sizeof(role_text), "Spectator");
		}
		tft_draw_string(0, 40, LGRAY, "You are: %s", role_text);
		break;

	case STATE_HOSTING:
		tft_draw_string(0, 0, LGRAY, "Waiting for clients...");
		tft_draw_string(0, 20, LGRAY, "Press START to ready.");
		tft_draw_string(0, 40, LGRAY, "Channel: %d", current_rf_channel);
		break;

	case STATE_READY_WAIT:
		tft_draw_string(0, 0, LGRAY, "Waiting for players to ready...");
		tft_draw_string(0, 20, LGRAY, "Channel: %d", current_rf_channel);
		break;

	case STATE_PLAYING:
		tft_draw_string(0, 0, WHITE, "Game In Progress!");
		break;

	default:
		break;
	}

	if (game_state != STATE_MENU && game_state != STATE_JOINING &&
	    game_state != STATE_HOST_CHANNEL_SCAN && game_state != STATE_CLIENT_CHANNEL_SCAN &&
	    game_state != STATE_ANTENNA_MENU && game_state != STATE_CLIENT_CHANNEL_SELECT) {
		for (int i = 0; i < NUM_PLAYERS; i++) {
			color_t display_color;
			char player_text[25];

			if (players[i].id != 0) {
				display_color = id_to_color(players[i].id);
				if (players[i].is_ready) {
					snprintf(player_text, sizeof(player_text), "%04x (Ready)",
						 players[i].id);
				} else {
					snprintf(player_text, sizeof(player_text), "%04x",
						 players[i].id);
				}
			} else {
				display_color = LGRAY;
				snprintf(player_text, sizeof(player_text), "----");
			}
			tft_draw_string(0, 60 + i * 20, display_color, "P%d: %s", i + 1,
					player_text);
		}
	}
}

static bool root_handle(sdk_event_t event, int depth)
{
	if (depth) {
		return false;
	}

	if (event == SDK_PRESSED_SELECT) {
		if (game_state == STATE_ANTENNA_MENU || game_state == STATE_CLIENT_CHANNEL_SELECT ||
		    game_state == STATE_HOSTING || game_state == STATE_JOINING ||
		    game_state == STATE_READY_WAIT) {
			printf("DEBUG: SELECT pressed. Returning to Main Menu.\n");
			game_state_change(STATE_MENU);
			return true;
		}
	}

	switch (game_state) {
	case STATE_MENU:
		switch (event) {
		case SDK_PRESSED_A:
			printf("DEBUG: Menu - Host Game selected (Pressed A). Starting channel scan.\n");
			game_state_change(STATE_HOST_CHANNEL_SCAN);
			channel_scan_index = 0;
			current_rf_channel = channels_to_scan[channel_scan_index];
			sdk_set_rf_channel(current_rf_channel);
			for (int i = 0; i < NUM_PLAYERS; i++) {
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].color = LGRAY;
				players[i].is_ready = false;
			}
			for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
				scanned_channel_occupancy[i] = 0;
			}
			host_channel_is_busy_during_scan = false;
			return true;
		case SDK_PRESSED_B:
			printf("DEBUG: Menu - Join Game selected (Pressed B). Starting client channel scan.\n");
			game_state_change(STATE_CLIENT_CHANNEL_SCAN);
			channel_scan_index = 0;
			current_rf_channel = channels_to_scan[channel_scan_index];
			sdk_set_rf_channel(current_rf_channel);
			for (int i = 0; i < NUM_PLAYERS; i++) {
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].color = LGRAY;
				players[i].is_ready = false;
			}
			for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
				scanned_channel_occupancy[i] = 0;
			}
			client_game_found_on_channel = false;
			return true;
		default:
			return false;
		}
		break;

	case STATE_ANTENNA_MENU:
		switch (event) {
		case SDK_PRESSED_START:
			if (scanned_channel_occupancy[selected_channel_index] == 0) {
				current_rf_channel = channels_to_scan[selected_channel_index];
				sdk_set_rf_channel(current_rf_channel);
				printf("DEBUG: Host selected clean channel %d. Proceeding to hosting.\n",
				       current_rf_channel);
				game_state_change(STATE_HOSTING);
				players[0].id = game_our_id;
				players[0].color = BLUE;
				players[0].timeout = PLAYER_TIMEOUT;
				players[0].player_idx = 0;
				players[0].is_ready = false;
			} else {
				printf("DEBUG: Host tried to select busy channel %d (occupied by %04x). Cannot host here.\n",
				       channels_to_scan[selected_channel_index],
				       scanned_channel_occupancy[selected_channel_index]);
			}
			return true;
		default:
			return false;
		}
		break;

	case STATE_CLIENT_CHANNEL_SELECT:
		switch (event) {
		case SDK_PRESSED_START:
			if (scanned_channel_occupancy[selected_channel_index] != 0) {
				current_rf_channel = channels_to_scan[selected_channel_index];
				sdk_set_rf_channel(current_rf_channel);
				printf("DEBUG: Client selected channel %d with game. Attempting to join.\n",
				       current_rf_channel);
				game_state_change(STATE_JOINING);
			} else {
				printf("DEBUG: Client tried to select channel %d with no game found. Cannot join.\n",
				       channels_to_scan[selected_channel_index]);
			}
			return true;
		default:
			return false;
		}
		break;

	case STATE_HOSTING:
		switch (event) {
		case SDK_PRESSED_START:
			printf("DEBUG: Host pressed START to ready up. Sending BEGIN message.\n");
			game_state_change(STATE_READY_WAIT);
			players[0].id = game_our_id;
			players[0].color = BLUE;
			players[0].timeout = PLAYER_TIMEOUT;
			players[0].player_idx = 0;
			players[0].is_ready = true;
			tx_begin();
			tx_ready();
			return true;
		default:
			return false;
		}
		break;

	case STATE_JOINING:
		switch (event) {
		case SDK_PRESSED_START:
			printf("DEBUG: Client pressed START to confirm joining. Sending READY message.\n");
			Player *self_player = find_player(game_our_id);
			if (self_player) {
				self_player->is_ready = true;
			} else {
				printf("ERROR: Client pressed START but could not find its player slot.\n");
			}
			tx_ready();
			return true;
		default:
			return false;
		}
		break;

	case STATE_READY_WAIT:
		bool all_ready = true;
		if (players[0].id != 0 && !players[0].is_ready) {
			all_ready = false;
		}
		if (players[1].id != 0 && !players[1].is_ready) {
			all_ready = false;
		}

		if (all_ready) {
			printf("DEBUG: All active players are ready (handle check). Sending GAME_START!\n");
			tx_game_start();
			game_state_change(STATE_ROUND_RESET);
			if (menu_melody != NULL) {
				sdk_melody_stop_and_release(menu_melody);
				menu_melody = NULL;
			}
			sdk_scene_push(&scene_round);
			game_reset();
		}
		return false;

	default:
		return false;
	}
	return false;
}

static void root_tick(int jiffies, int depth)
{
	int new_time = game_state_timer + jiffies;
	game_state_timer = new_time >= 0 ? new_time : INT_MAX;

	if (depth) {
		return;
	}

	for (int i = 0; i < NUM_PLAYERS; i++) {
		if (players[i].id != 0 && players[i].id != game_our_id) {
			players[i].timeout -= jiffies;

			if (players[i].timeout < 0) {
				printf("DEBUG: Player %04x timed out.\n", players[i].id);
				players[i].id = 0;
				players[i].player_idx = -1;
				players[i].color = LGRAY;
				players[i].is_ready = false;
			}
		} else if (players[i].id == game_our_id) {
			players[i].timeout = PLAYER_TIMEOUT;
		}
	}

	switch (game_state) {
	case STATE_MENU:
		break;

	case STATE_ANTENNA_MENU:
	case STATE_CLIENT_CHANNEL_SELECT:
		if (game_state_timer - last_joystick_menu_move_time >= JOYSTICK_MENU_DEBOUNCE_US) {
			if (sdk_inputs.joy_y < -500) {
				selected_channel_index--;
				if (selected_channel_index < 0) {
					selected_channel_index = (int)NUM_SCAN_CHANNELS - 1;
				}
				last_joystick_menu_move_time = game_state_timer;
				printf("DEBUG: Channel Menu: Selected channel index: %d (Joystick Up)\n",
				       selected_channel_index);
			} else if (sdk_inputs.joy_y > 500) {
				selected_channel_index++;
				if (selected_channel_index >= (int)NUM_SCAN_CHANNELS) {
					selected_channel_index = 0;
				}
				last_joystick_menu_move_time = game_state_timer;
				printf("DEBUG: Channel Menu: Selected channel index: %d (Joystick Down)\n",
				       selected_channel_index);
			}
		}
		break;

	case STATE_HOST_CHANNEL_SCAN:
		if (game_state_timer >= CHANNEL_SCAN_TIMEOUT_US) {
			if (host_channel_is_busy_during_scan) {
				printf("DEBUG: Host scan: Channel %d is busy (occupied by %04x).\n",
				       current_rf_channel,
				       scanned_channel_occupancy[channel_scan_index]);
			} else {
				printf("DEBUG: Host scan: Channel %d is clean.\n",
				       current_rf_channel);
				scanned_channel_occupancy[channel_scan_index] = 0;
			}

			channel_scan_index++;
			if (channel_scan_index < (int)NUM_SCAN_CHANNELS) {
				current_rf_channel = channels_to_scan[channel_scan_index];
				sdk_set_rf_channel(current_rf_channel);
				game_state_timer = 0;
				host_channel_is_busy_during_scan = false;
				printf("DEBUG: Host scanning channel %d.\n", current_rf_channel);
			} else {
				printf("DEBUG: Host scan complete. Moving to Antenna Menu.\n");
				game_state_change(STATE_ANTENNA_MENU);
				selected_channel_index = 0;
			}
		}
		break;

	case STATE_CLIENT_CHANNEL_SCAN:
		tx_beacon();
		if (game_state_timer >= CHANNEL_SCAN_TIMEOUT_US) {
			if (!client_game_found_on_channel) {
				printf("DEBUG: Client: No game found or game was full on channel %d. Trying next.\n",
				       current_rf_channel);
			} else {
				printf("DEBUG: Client: Game found on channel %d. Recorded for selection menu.\n",
				       current_rf_channel);
			}

			channel_scan_index++;
			if (channel_scan_index < (int)NUM_SCAN_CHANNELS) {
				current_rf_channel = channels_to_scan[channel_scan_index];
				sdk_set_rf_channel(current_rf_channel);
				game_state_timer = 0;
				client_game_found_on_channel = false;
				printf("DEBUG: Client scanning channel %d.\n", current_rf_channel);
			} else {
				printf("DEBUG: Client scan complete. Moving to Client Channel Selection Menu.\n");
				game_state_change(STATE_CLIENT_CHANNEL_SELECT);
				selected_channel_index = 0;
			}
		}
		break;

	case STATE_LOBBY:
		tx_beacon();
		break;

	case STATE_JOINING:
		tx_beacon();
		Player *self_player_tick = find_player(game_our_id);
		if (self_player_tick && self_player_tick->is_ready) {
			tx_ready();
		}
		break;

	case STATE_HOSTING:
		tx_begin();
		break;

	case STATE_READY_WAIT:
		tx_begin();
		tx_ready();

		bool all_ready_tick = true;
		if (players[0].id != 0 && !players[0].is_ready) {
			all_ready_tick = false;
		}
		if (players[1].id != 0 && !players[1].is_ready) {
			all_ready_tick = false;
		}

		if (all_ready_tick) {
			printf("DEBUG: All active players are ready (tick check). Triggering game start via handle!\n");
		}
		break;

	default:
		break;
	}
}

static color_t id_to_color(uint16_t id)
{
	if (players[0].id != 0 && id == players[0].id) {
		return BLUE;
	}
	if (players[1].id != 0 && id == players[1].id) {
		return GREEN;
	}
	return LGRAY;
}

static bool root_inbox(sdk_message_t msg, int depth)
{
	if (depth || msg.type != SDK_MSG_RF) {
		return false;
	}

	if (3 == msg.rf.length && MSG_BEACON == msg.rf.data[0]) {
		uint16_t id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		if (id == game_our_id) {
			return true;
		}

		if (game_state == STATE_HOST_CHANNEL_SCAN) {
			host_channel_is_busy_during_scan = true;
			printf("DEBUG: Host scan: Beacon from %04x received on channel %d. Channel is active.\n",
			       id, current_rf_channel);
			return true;
		}

		if (game_state == STATE_LOBBY || game_state == STATE_HOSTING ||
		    game_state == STATE_READY_WAIT) {
			if ((game_state == STATE_HOSTING || game_state == STATE_READY_WAIT) &&
			    players[0].id != 0 && players[1].id != 0 && id != players[0].id &&
			    id != players[1].id) {
				printf("DEBUG: Host ignoring beacon from %04x (2-player limit reached).\n",
				       id);
				return true;
			}

			Player *player = find_player(id);
			if (player == NULL) {
				printf("DEBUG: No available slot for beacon from %04x.\n", id);
				return true;
			}

			player->id = id;
			player->timeout = PLAYER_TIMEOUT;
			player->player_idx = -1;
			printf("DEBUG: Received BEACON from %04x. Player timeout reset.\n", id);
			game_state_change(game_state);
			return true;
		}
		return false;
	}

	if ((1 + 2 * NUM_ACTIVE_PLAYERS) == msg.rf.length && MSG_BEGIN == msg.rf.data[0]) {
		uint16_t host_id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		uint16_t player2_id = (msg.rf.data[3] << 8) | msg.rf.data[4];

		if (game_state == STATE_HOST_CHANNEL_SCAN) {
			host_channel_is_busy_during_scan = true;
			scanned_channel_occupancy[channel_scan_index] = host_id;
			printf("DEBUG: Host scan: MSG_BEGIN from %04x received on channel %d. Channel is busy.\n",
			       host_id, current_rf_channel);
			return true;
		}

		if (game_state == STATE_CLIENT_CHANNEL_SCAN) {
			if (host_id != 0 && host_id != game_our_id && player2_id != 0 &&
			    player2_id != game_our_id) {
				printf("DEBUG: Client scan: Game on channel %d is full (P1:%04x, P2:%04x). Cannot join.\n",
				       current_rf_channel, host_id, player2_id);
				scanned_channel_occupancy[channel_scan_index] = 0;
				client_game_found_on_channel = false;
				return true;
			} else {
				printf("DEBUG: Client scan: Game found on channel %d (P1:%04x, P2:%04x). Marking for selection.\n",
				       current_rf_channel, host_id, player2_id);
				scanned_channel_occupancy[channel_scan_index] = host_id;
				client_game_found_on_channel = true;
				return true;
			}
		}

		if (game_state == STATE_LOBBY || game_state == STATE_JOINING) {
			printf("DEBUG: Received BEGIN message. Current state: %d. Transitioning to STATE_JOINING.\n",
			       game_state);
			game_state_change(STATE_JOINING);

			bool local_is_ready = false;
			Player *self_player_before_clear = find_player(game_our_id);
			if (self_player_before_clear &&
			    self_player_before_clear->id == game_our_id) {
				local_is_ready = self_player_before_clear->is_ready;
			}

			for (int i = 0; i < NUM_PLAYERS; i++) {
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].is_ready = false;
			}

			players[0].id = host_id;
			players[0].color = BLUE;
			players[0].timeout = PLAYER_TIMEOUT;
			players[0].player_idx = 0;
			printf("DEBUG: P1 (Host): %04x\n", host_id);

			players[1].id = player2_id;
			players[1].color = GREEN;
			players[1].timeout = PLAYER_TIMEOUT;
			players[1].player_idx = 1;
			printf("DEBUG: P2: %04x\n", player2_id);

			Player *self_player_after_clear = find_player(game_our_id);
			if (self_player_after_clear && self_player_after_clear->id == game_our_id) {
				self_player_after_clear->is_ready = local_is_ready;
			} else if (self_player_after_clear == NULL &&
				   game_our_id != players[0].id && game_our_id != players[1].id) {
				printf("DEBUG: Local device %04x is a spectator (no active player slot).\n",
				       game_our_id);
			}
			return true;
		}
		return false;
	}

	if (3 == msg.rf.length && MSG_READY == msg.rf.data[0]) {
		uint16_t id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		Player *player = find_player(id);
		if (player && player->id != 0) {
			player->is_ready = true;
			printf("DEBUG: Received READY from %04x. Player status: Ready.\n", id);

			if (game_state == STATE_READY_WAIT) {
				bool all_active_players_ready = true;
				if (players[0].id != 0 && !players[0].is_ready) {
					all_active_players_ready = false;
				}
				if (players[1].id != 0 && !players[1].is_ready) {
					all_active_players_ready = false;
				}

				if (all_active_players_ready) {
					printf("DEBUG: All active players are ready (inbox check). Sending GAME_START!\n");
					tx_game_start();
					game_state_change(STATE_ROUND_RESET);
					if (menu_melody != NULL) {
						sdk_melody_stop_and_release(menu_melody);
						menu_melody = NULL;
					}
					sdk_scene_push(&scene_round);
					game_reset();
				}
			} else if (game_state == STATE_JOINING) {
				game_state_change(game_state);
			}
			return true;
		}
		return false;
	}

	if (1 == msg.rf.length && MSG_GAME_START == msg.rf.data[0]) {
		if (game_state == STATE_JOINING) {
			printf("DEBUG: Received GAME_START message. Transitioning to STATE_ROUND_RESET.\n");
			game_state_change(STATE_ROUND_RESET);
			if (menu_melody != NULL) {
				sdk_melody_stop_and_release(menu_melody);
				menu_melody = NULL;
			}
			sdk_scene_push(&scene_round);
			game_reset();
			return true;
		}
		return false;
	}

	return false;
}

static void root_revealed(void)
{
	printf("DEBUG: Root scene revealed. Resetting to menu.\n");
	game_state_change(STATE_MENU);

	for (int i = 0; i < NUM_PLAYERS; i++) {
		memset(&players[i], 0, sizeof(Player));
		players[i].player_idx = -1;
		players[i].color = LGRAY;
		players[i].is_ready = false;
	}
	for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
		scanned_channel_occupancy[i] = 0;
	}
	printf("DEBUG: Player slots and channel occupancy cleared. Waiting for menu selection.\n");

	if (menu_melody == NULL) {
		menu_melody = sdk_melody_play_get(menu_music_str);
	}
}

static void root_pushed(void)
{
	game_our_id = (sdk_device_id * 0x9e3779b97f4a77ff) >> 48;

	if (!game_our_id) {
		game_our_id = ~0;
	}

	sdk_set_rf_channel(42);
	current_rf_channel = 42;

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
