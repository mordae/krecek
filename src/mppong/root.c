// root.c
#include <limits.h>  // For INT_MAX
#include <string.h>  // For memcpy, memset
#include <stdio.h>   // Required for snprintf and printf
#include <stdbool.h> // For bool type
#include <sdk.h>     // Explicitly include sdk.h

#include "common.h" // Include common definitions

// --- External Scene Declaration ---
// Declares the 'scene_round' which will contain the actual game logic.
// This allows 'root.c' to transition to the game scene.
extern sdk_scene_t scene_round;

// --- Global Variable Definitions ---
// These variables are declared as 'extern' in common.h and defined here.
GameState game_state;
int game_state_timer;
uint16_t game_our_id;
Player players[NUM_PLAYERS];	       // Array to hold player data (now size 2)
int current_rf_channel;		       // Current RF channel being used
int channel_scan_index;		       // Index for channel scanning (0 for 66, 1 for 67, 2 for 68)
uint16_t scanned_channel_occupancy[3]; // Stores ID if channel is occupied, or 0 if clean.
int selected_channel_index; // Index of the channel currently selected in the Antenna Menu

// Global definition for the menu music melody
sdk_melody_t *menu_melody = NULL;

// Music string for the menu (Wii Shop Channel inspired) - FIXED: Removed '.' from notes
static const char menu_music_str[] = "/i:sine /bpm:90 { "
				     "C4 G3 C4 G3 " // Simple arpeggio
				     "D4 A3 D4 A3 "
				     "E4 B3 E4 B3 "
				     "F4 C4 F4 C4 "
				     "G4 D4 G4 D4 "
				     "F4 C4 F4 C4 "
				     "E4 B3 E4 B3 "
				     "D4 A3 D4 A3 "
				     "}";

// Array of channels to scan
static const int channels_to_scan[] = { SDK_RF_CHANNEL_66, SDK_RF_CHANNEL_67, SDK_RF_CHANNEL_68 };
#define NUM_SCAN_CHANNELS (sizeof(channels_to_scan) / sizeof(channels_to_scan[0]))

// Flag to indicate if a game was found on the current channel during client scan
static bool client_game_found_on_channel = false;
// Flag to indicate if the current channel is busy (for host scan)
static bool host_channel_is_busy_during_scan = false; // Renamed for clarity in scan phase

// For joystick debouncing in menus
static uint32_t last_joystick_menu_move_time = 0;
#define JOYSTICK_MENU_DEBOUNCE_US 200000 // 0.2 seconds debounce for menu navigation

// --- Function Prototypes for static functions ---
static color_t id_to_color(uint16_t id);
static void tx_ready(void);
static void tx_game_start(void);
static void tx_beacon(void);
static void tx_begin(void);

// --- Helper Functions ---

/**
 * @brief Changes the current game state and resets the state timer.
 * @param new_state The new GameState to transition to.
 */
void game_state_change(GameState new_state)
{
	game_state = new_state;
	game_state_timer = 0; // Reset timer upon state change
	printf("DEBUG: Game state changed to %d. Timer reset.\n",
	       new_state); // Print new_state value
}

/**
 * @brief Finds a player in the 'players' array by their ID.
 * Only searches active player slots (P1 and P2).
 * @param id The ID of the player to find.
 * @return A pointer to the Player struct in the 'players' array if found or an empty slot,
 * or NULL if no active player slot is available.
 */
Player *find_player(uint16_t id)
{
	// First, try to find an existing player with the given ID within active slots
	for (int i = 0; i < NUM_ACTIVE_PLAYERS; i++) {
		if (players[i].id == id) {
			return &players[i];
		}
	}

	// If not found, try to find an empty slot within active slots
	for (int i = 0; i < NUM_ACTIVE_PLAYERS; i++) {
		if (players[i].id == 0) {
			return &players[i]; // Return the first available empty active slot
		}
	}

	// If no existing or empty active player slot is found, return NULL
	return NULL;
}

/**
 * @brief Sends a beacon message over RF to announce presence.
 * Rate-limited to prevent excessive transmissions.
 */
static void tx_beacon(void)
{
	// Only send beacons in lobby/connection states or during client channel scan
	if (game_state >= STATE_ROUND_RESET && game_state != STATE_CLIENT_CHANNEL_SCAN)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	// Rate limit beacons
	if (now - last_tx < BEACON_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	// Message format: [MSG_BEACON, high_byte_of_id, low_byte_of_id]
	uint8_t msg[] = { MSG_BEACON, game_our_id >> 8, (uint8_t)game_our_id };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	// printf("DEBUG: Sent BEACON (ID: %04x) on channel %d\n", game_our_id, current_rf_channel); // Commented out for less spam
}

/**
 * @brief Sends a game start (MSG_BEGIN) message over RF.
 * This message includes the IDs of the two active players.
 * Rate-limited to prevent excessive transmissions.
 */
static void tx_begin(void)
{
	// Only send BEGIN in hosting/ready_wait states. Not during channel scan.
	if (game_state != STATE_HOSTING && game_state != STATE_READY_WAIT)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	// Rate limit begin messages
	if (now - last_tx < BEGIN_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	// Construct the message with IDs of the actual playing entities.
	// Message length: 1 (MSG_BEGIN) + 2 * NUM_ACTIVE_PLAYERS (for IDs).
	// For NUM_ACTIVE_PLAYERS = 2, this is 1 + 2*2 = 5 bytes.
	uint8_t msg[1 + 2 * NUM_ACTIVE_PLAYERS];
	msg[0] = MSG_BEGIN;

	// Ensure players[0] and players[1] (host and second player) are included
	msg[1] = players[0].id >> 8;
	msg[2] = (uint8_t)players[0].id;
	msg[3] = players[1].id >> 8;
	msg[4] = (uint8_t)players[1].id;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent BEGIN (P1: %04x, P2: %04x) on channel %d\n", players[0].id,
	       players[1].id, current_rf_channel);
}

/**
 * @brief Sends a ready message over RF to confirm readiness.
 * Message format: [MSG_READY, high_byte_of_id, low_byte_of_id]
 */
static void tx_ready(void)
{
	// Only send READY in lobby/connection states
	if (game_state >= STATE_ROUND_RESET)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	// Rate limit ready messages
	if (now - last_tx < READY_TX_INTERVAL_US) {
		return;
	}

	last_tx = now;

	uint8_t msg[] = { MSG_READY, game_our_id >> 8, (uint8_t)game_our_id };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent READY (ID: %04x) on channel %d\n", game_our_id, current_rf_channel);
}

/**
 * @brief Sends a game start message over RF.
 * This message signals clients to transition to the game.
 */
static void tx_game_start(void)
{
	// Only send GAME_START in lobby/connection states
	if (game_state >= STATE_ROUND_RESET)
		return;

	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	// Rate limit game start messages
	if (now - last_tx < GAME_START_TX_INTERVAL_US) {
		return;
	}
	last_tx = now;

	uint8_t msg[] = { MSG_GAME_START };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Sent GAME_START message on channel %d.\n", current_rf_channel);
}

/**
 * @brief Scene paint function: Draws the current lobby state and player list.
 * @param dt Delta time (unused in this simple paint function).
 * @param depth Current rendering depth (only draw if depth is 0).
 */
static void root_paint(float dt, int depth)
{
	(void)dt;    // Suppress unused parameter warning
	(void)depth; // Suppress unused parameter warning

	if (0 != depth) {
		return; // Only paint for the base depth
	}

	tft_fill(BLACK); // Clear the screen to black

	// Display messages based on current game state
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
		// Center the title
		const char *title_text = "--- Choose Channel ---";
		int title_width_pixels = strlen(title_text) * 6; // Assuming 6 pixels per char
		tft_draw_string((TFT_WIDTH - title_width_pixels) / 2, 0, WHITE, title_text);

		int rect_width = 100; // Wider rectangles
		int rect_height = 30; // Taller rectangles
		int gap = 5;
		int start_x_rect = (TFT_WIDTH - rect_width) / 2; // Center rectangles horizontally
		int start_y_rect = 20;				 // Start below the title

		int dot_size = 10;

		for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
			int y_pos = start_y_rect + i * (rect_height + gap);
			color_t rect_color = LGRAY;   // Default color
			char channel_status_text[20]; // To hold "Clean" or "Host ID"

			// Determine channel status text
			if (scanned_channel_occupancy[i] == 0) {
				snprintf(channel_status_text, sizeof(channel_status_text), "Clean");
			} else {
				snprintf(channel_status_text, sizeof(channel_status_text), "%04x",
					 scanned_channel_occupancy[i]);
			}

			// Draw the rectangle
			tft_draw_rect(start_x_rect, y_pos, start_x_rect + rect_width - 1,
				      y_pos + rect_height - 1, rect_color);

			// Draw channel number (1, 2, 3)
			char channel_num_text[5];
			snprintf(channel_num_text, sizeof(channel_num_text), "%d:", i + 1);
			tft_draw_string(start_x_rect + 5, y_pos + 10, WHITE, channel_num_text);

			// Draw channel status (Clean or Host ID)
			int status_text_x = start_x_rect + 5 + (strlen(channel_num_text) * 6) +
					    5; // Position after "X:"
			tft_draw_string(status_text_x, y_pos + 10, WHITE, channel_status_text);

			// Draw the white dot (cursor) to the left of the selected rectangle
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
		// Center the title
		const char *title_text = "--- Join Channel ---";
		int title_width_pixels = strlen(title_text) * 6; // Assuming 6 pixels per char
		tft_draw_string((TFT_WIDTH - title_width_pixels) / 2, 0, WHITE, title_text);

		int rect_width = 100; // Wider rectangles
		int rect_height = 30; // Taller rectangles
		int gap = 5;
		int start_x_rect = (TFT_WIDTH - rect_width) / 2; // Center rectangles horizontally
		int start_y_rect = 20;				 // Start below the title

		int dot_size = 10;

		for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
			int y_pos = start_y_rect + i * (rect_height + gap);
			color_t rect_color = LGRAY;   // Default color
			char channel_status_text[20]; // To hold "Game Found" or "No Game"

			// Determine channel status text for client
			if (scanned_channel_occupancy[i] != 0) {
				snprintf(channel_status_text, sizeof(channel_status_text),
					 "Game Found");
			} else {
				snprintf(channel_status_text, sizeof(channel_status_text),
					 "No Game");
			}

			// Draw the rectangle
			tft_draw_rect(start_x_rect, y_pos, start_x_rect + rect_width - 1,
				      y_pos + rect_height - 1, rect_color);

			// Draw channel number (1, 2, 3)
			char channel_num_text[5];
			snprintf(channel_num_text, sizeof(channel_num_text), "%d:", i + 1);
			tft_draw_string(start_x_rect + 5, y_pos + 10, WHITE, channel_num_text);

			// Draw channel status (Game Found or No Game)
			int status_text_x = start_x_rect + 5 + (strlen(channel_num_text) * 6) +
					    5; // Position after "X:"
			tft_draw_string(status_text_x, y_pos + 10, WHITE, channel_status_text);

			// Draw the white dot (cursor) to the left of the selected rectangle
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

	case STATE_LOBBY: // Client scanning for host (might be redundant now)
		tft_draw_string(0, 0, LGRAY, "Scanning for host...");
		tft_draw_string(0, 20, LGRAY, "Waiting for game to start.");
		break;

	case STATE_JOINING: // Client received BEGIN message, waiting for confirmation
		tft_draw_string(0, 0, LGRAY, "Game found! Press START.");
		tft_draw_string(0, 20, LGRAY, "Host: P1 %04x", players[0].id);
		// Display current channel
		tft_draw_string(0, 60, LGRAY, "Channel: %d", current_rf_channel);
		// Determine and display client's role
		char role_text[20]; // Buffer for role string
		if (game_our_id == players[0].id) {
			snprintf(role_text, sizeof(role_text), "P1");
		} else if (game_our_id == players[1].id) {
			snprintf(role_text, sizeof(role_text), "P2");
		} else {
			// This case should ideally not happen with 2-player limit, but for safety
			snprintf(role_text, sizeof(role_text), "Spectator");
		}
		tft_draw_string(0, 40, LGRAY, "You are: %s", role_text);
		break;

	case STATE_HOSTING: // Host waiting for clients to join
		tft_draw_string(0, 0, LGRAY, "Waiting for clients...");
		tft_draw_string(0, 20, LGRAY, "Press START to ready."); // Changed prompt
		// Display current channel
		tft_draw_string(0, 40, LGRAY, "Channel: %d", current_rf_channel);
		break;

	case STATE_READY_WAIT: // Host has pressed START, waiting for all to be ready
		tft_draw_string(0, 0, LGRAY, "Waiting for players to ready...");
		// Display current channel
		tft_draw_string(0, 20, LGRAY, "Channel: %d", current_rf_channel);
		break;

	case STATE_PLAYING:
		tft_draw_string(0, 0, WHITE, "Game In Progress!"); // Actual game starts here
		break;

	default:
		break;
	}

	// Only display player slots if not in the main menu, joining, or channel scanning states
	if (game_state != STATE_MENU && game_state != STATE_JOINING &&
	    game_state != STATE_HOST_CHANNEL_SCAN && game_state != STATE_CLIENT_CHANNEL_SCAN &&
	    game_state != STATE_ANTENNA_MENU && game_state != STATE_CLIENT_CHANNEL_SELECT) {
		// Display player slots for P1 and P2
		for (int i = 0; i < NUM_PLAYERS; i++) { // Loop only for NUM_PLAYERS (2)
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

/**
 * @brief Scene handle function: Processes discrete input events.
 * @param event The SDK event that occurred (e.g., button press).
 * @param depth Current scene depth (only handle if depth is 0).
 * @return True if the event was handled, false otherwise.
 */
static bool root_handle(sdk_event_t event, int depth)
{
	if (depth) {
		return false; // Only handle events for the base depth
	}

	// Handle SELECT button to return to main menu from various states
	if (event == SDK_PRESSED_SELECT) {
		if (game_state == STATE_ANTENNA_MENU ||
		    game_state == STATE_CLIENT_CHANNEL_SELECT || // Added for client menu
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
		case SDK_PRESSED_A: // Choose to Host
			printf("DEBUG: Menu - Host Game selected (Pressed A). Starting channel scan.\n");
			game_state_change(STATE_HOST_CHANNEL_SCAN);
			channel_scan_index = 0; // Start with the first channel
			current_rf_channel = channels_to_scan[channel_scan_index];
			sdk_set_rf_channel(current_rf_channel);
			// Clear player slots for host's initial state
			for (int i = 0; i < NUM_PLAYERS; i++) {
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].color = LGRAY;
				players[i].is_ready = false;
			}
			// Clear scanned channel occupancy for a fresh scan
			for (int i = 0; i < (int)NUM_SCAN_CHANNELS;
			     i++) { // Cast to int to fix warning
				scanned_channel_occupancy[i] = 0;
			}
			host_channel_is_busy_during_scan = false; // Reset busy flag for new scan
			return true;
		case SDK_PRESSED_B: // Choose to Join
			printf("DEBUG: Menu - Join Game selected (Pressed B). Starting client channel scan.\n");
			game_state_change(
				STATE_CLIENT_CHANNEL_SCAN); // Transition to client channel scan state
			channel_scan_index = 0;		    // Start with the first channel
			current_rf_channel = channels_to_scan[channel_scan_index];
			sdk_set_rf_channel(current_rf_channel);
			// Clear player slots for client to receive host's info
			for (int i = 0; i < NUM_PLAYERS; i++) {
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].color = LGRAY;
				players[i].is_ready = false;
			}
			// Clear scanned channel occupancy for client's scan results
			for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) {
				scanned_channel_occupancy[i] = 0;
			}
			client_game_found_on_channel = false; // Reset flag for new scan
			return true;
		default:
			return false;
		}
		break; // End of STATE_MENU case

	case STATE_ANTENNA_MENU:
		switch (event) {
		// Joystick movement is handled in root_tick for continuous input with debouncing
		case SDK_PRESSED_START:
			if (scanned_channel_occupancy[selected_channel_index] == 0) {
				// Channel is clean, select it and proceed to hosting
				current_rf_channel = channels_to_scan[selected_channel_index];
				sdk_set_rf_channel(current_rf_channel);
				printf("DEBUG: Host selected clean channel %d. Proceeding to hosting.\n",
				       current_rf_channel);
				game_state_change(STATE_HOSTING);
				// Initialize host's player slot for the game
				players[0].id = game_our_id;
				players[0].color = BLUE;
				players[0].timeout = PLAYER_TIMEOUT;
				players[0].player_idx = 0;
				players[0].is_ready = false;
			} else {
				printf("DEBUG: Host tried to select busy channel %d (occupied by %04x). Cannot host here.\n",
				       channels_to_scan[selected_channel_index],
				       scanned_channel_occupancy[selected_channel_index]);
				// Optionally, display a temporary "Channel busy!" message
			}
			return true;
		default:
			return false; // Event not handled by this specific state's switch
		}
		break; // End of STATE_ANTENNA_MENU case

	case STATE_CLIENT_CHANNEL_SELECT:
		switch (event) {
		// Joystick movement is handled in root_tick for continuous input with debouncing
		case SDK_PRESSED_START:
			if (scanned_channel_occupancy[selected_channel_index] != 0) {
				// Game found on this channel, attempt to join
				current_rf_channel = channels_to_scan[selected_channel_index];
				sdk_set_rf_channel(current_rf_channel);
				printf("DEBUG: Client selected channel %d with game. Attempting to join.\n",
				       current_rf_channel);
				game_state_change(STATE_JOINING);
				// The client will now send READY messages in STATE_JOINING
				// and wait for MSG_BEGIN from the host on this channel.
				// The player slots will be populated by the MSG_BEGIN.
			} else {
				printf("DEBUG: Client tried to select channel %d with no game found. Cannot join.\n",
				       channels_to_scan[selected_channel_index]);
				// Optionally, display a temporary "No game here!" message
			}
			return true;
		default:
			return false; // Event not handled by this specific state's switch
		}
		break; // End of STATE_CLIENT_CHANNEL_SELECT case

	case STATE_HOSTING:
		switch (event) {
		case SDK_PRESSED_START:
			// When START is pressed, this device (host) becomes ready and sends MSG_BEGIN.
			printf("DEBUG: Host pressed START to ready up. Sending BEGIN message.\n");
			game_state_change(
				STATE_READY_WAIT); // Host transitions to waiting for others to be ready

			// Host sets itself as ready
			players[0].id = game_our_id; // Ensure host's ID is set in players[0]
			players[0].color = BLUE;
			players[0].timeout = PLAYER_TIMEOUT;
			players[0].player_idx = 0;
			players[0].is_ready = true;
			tx_begin(); // Send the begin message with assigned players (P1 and P2)
			tx_ready(); // Also send a ready message for itself

			return true;
		default:
			return false; // Event not handled by this specific state's switch
		}
		break; // End of STATE_HOSTING case

	case STATE_JOINING: // Client received BEGIN message, waiting for confirmation
		switch (event) {
		case SDK_PRESSED_START:
			printf("DEBUG: Client pressed START to confirm joining. Sending READY message.\n");
			// Client sets itself as ready
			Player *self_player = find_player(game_our_id);
			if (self_player) {
				self_player->is_ready = true;
			} else {
				printf("ERROR: Client pressed START but could not find its player slot.\n");
			}
			tx_ready(); // Send ready message
			// Client remains in STATE_JOINING until it receives a MSG_READY from all active players
			// (or the host decides to start the game).
			return true;
		default:
			return false; // Event not handled by this specific state's switch
		}
		break; // End of STATE_JOINING case

	case STATE_READY_WAIT: // Host is in this state, waiting for all players to be ready
		// Check if all active players are ready
		bool all_ready = true;
		// P1 (host) must be ready
		if (players[0].id != 0 && !players[0].is_ready) {
			all_ready = false;
		}
		// If P2 exists, P2 must also be ready
		if (players[1].id != 0 && !players[1].is_ready) {
			all_ready = false;
		}

		if (all_ready) {
			printf("DEBUG: All active players are ready (handle check). Sending GAME_START!\n");
			tx_game_start(); // Send game start message to clients
			// HOST: Instead of STATE_PLAYING directly, transition to STATE_ROUND_RESET for initial countdown
			game_state_change(STATE_ROUND_RESET); // Host starts initial countdown
			// Stop menu music before pushing game scene
			if (menu_melody != NULL) {
				sdk_melody_stop_and_release(menu_melody);
				menu_melody = NULL;
			}
			sdk_scene_push(&scene_round);
			game_reset(); // <--- HOST INITIALIZES GAME STATE HERE (calls new_round and sets dx/dy)
		}
		return false; // No direct input handling in this state, relies on tick/inbox or SELECT

	default: // For STATE_LOBBY, STATE_PLAYING, STATE_ROUND_RESET, STATE_HOST_CHANNEL_SCAN, STATE_CLIENT_CHANNEL_SCAN
		return false; // No specific input handling for these states in root_handle beyond SELECT
	}
	return false;
}

/**
 * @brief Scene tick function: Updates continuous game logic.
 * @param jiffies Microseconds elapsed since the last tick.
 * @param depth Current scene depth (only tick if depth is 0).
 */
static void root_tick(int jiffies, int depth)
{
	// Update the game state timer, handling potential overflow by clamping at INT_MAX.
	int new_time = game_state_timer + jiffies;
	game_state_timer = new_time >= 0 ? new_time : INT_MAX;

	if (depth) {
		return; // Only tick for the base depth
	}

	// Timeout inactive players (from index 0 onwards, as all players can time out)
	// The host (players[0]) will only time out if it's not the local device.
	for (int i = 0; i < NUM_PLAYERS; i++) { // Loop only for NUM_PLAYERS (2)
		// Only timeout if the player is not the local device AND has an ID
		if (players[i].id != 0 && players[i].id != game_our_id) {
			players[i].timeout -= jiffies;

			if (players[i].timeout < 0) {
				printf("DEBUG: Player %04x timed out.\n", players[i].id);
				players[i].id = 0;	    // Clear player ID if timed out
				players[i].player_idx = -1; // Clear player_idx assignment
				players[i].color =
					LGRAY; // Reset color to default gray for empty slot
				players[i].is_ready = false; // Reset ready status
			}
		} else if (players[i].id == game_our_id) {
			// If it's our own device, ensure its timeout is always reset
			players[i].timeout = PLAYER_TIMEOUT;
		}
	}

	// Logic based on current game state
	switch (game_state) {
	case STATE_MENU:
		// No specific continuous tick logic for this menu state.
		break;

	case STATE_ANTENNA_MENU:
	case STATE_CLIENT_CHANNEL_SELECT: // Apply joystick debouncing to client channel select too
		// Handle joystick for menu navigation in tick for continuous input
		if (game_state_timer - last_joystick_menu_move_time >= JOYSTICK_MENU_DEBOUNCE_US) {
			if (sdk_inputs.joy_y < -500) { // Joystick up
				selected_channel_index--;
				if (selected_channel_index < 0) {
					selected_channel_index =
						(int)NUM_SCAN_CHANNELS - 1; // Wrap around
				}
				last_joystick_menu_move_time = game_state_timer;
				printf("DEBUG: Channel Menu: Selected channel index: %d (Joystick Up)\n",
				       selected_channel_index);
			} else if (sdk_inputs.joy_y > 500) { // Joystick down
				selected_channel_index++;
				if (selected_channel_index >= (int)NUM_SCAN_CHANNELS) {
					selected_channel_index = 0; // Wrap around
				}
				last_joystick_menu_move_time = game_state_timer;
				printf("DEBUG: Channel Menu: Selected channel index: %d (Joystick Down)\n",
				       selected_channel_index);
			}
		}
		break;

	case STATE_HOST_CHANNEL_SCAN:
		// Host is scanning channels for a clean one
		if (game_state_timer >= CHANNEL_SCAN_TIMEOUT_US) {
			// After timeout, record current channel's status
			if (host_channel_is_busy_during_scan) {
				// If busy, it means we received a MSG_BEGIN from another host
				// The ID would have been stored in scanned_channel_occupancy by inbox
				printf("DEBUG: Host scan: Channel %d is busy (occupied by %04x).\n",
				       current_rf_channel,
				       scanned_channel_occupancy[channel_scan_index]);
			} else {
				printf("DEBUG: Host scan: Channel %d is clean.\n",
				       current_rf_channel);
				scanned_channel_occupancy[channel_scan_index] =
					0; // Explicitly mark as clean
			}

			channel_scan_index++; // Move to the next channel
			if (channel_scan_index <
			    (int)NUM_SCAN_CHANNELS) { // Cast to int to fix warning
				current_rf_channel = channels_to_scan[channel_scan_index];
				sdk_set_rf_channel(current_rf_channel);
				game_state_timer = 0; // Reset timer for new channel scan
				host_channel_is_busy_during_scan =
					false; // Reset busy flag for new channel
				printf("DEBUG: Host scanning channel %d.\n", current_rf_channel);
			} else {
				// All channels checked, move to Antenna Menu for selection
				printf("DEBUG: Host scan complete. Moving to Antenna Menu.\n");
				game_state_change(STATE_ANTENNA_MENU);
				selected_channel_index = 0; // Default selection to first channel
				// current_rf_channel is not set here, it will be set when user selects a channel
			}
		}
		// Host does NOT send beacons during host channel scan; it only listens
		break;

	case STATE_CLIENT_CHANNEL_SCAN:
		// Client is scanning channels for an available game
		tx_beacon(); // Client sends beacons while scanning to find hosts
		if (game_state_timer >= CHANNEL_SCAN_TIMEOUT_US) {
			if (!client_game_found_on_channel) {
				// No game found or game was full on this channel after timeout, try next
				printf("DEBUG: Client: No game found or game was full on channel %d. Trying next.\n",
				       current_rf_channel);
			} else {
				// Game found on this channel, but we don't transition immediately.
				// We just record it and continue scanning other channels for the menu.
				printf("DEBUG: Client: Game found on channel %d. Recorded for selection menu.\n",
				       current_rf_channel);
			}

			channel_scan_index++;
			if (channel_scan_index <
			    (int)NUM_SCAN_CHANNELS) { // Cast to int to fix warning
				current_rf_channel = channels_to_scan[channel_scan_index];
				sdk_set_rf_channel(current_rf_channel);
				game_state_timer = 0; // Reset timer for new channel scan
				client_game_found_on_channel = false; // Reset flag for new channel
				// We don't clear players array here, as we are accumulating scan results for the menu.
				printf("DEBUG: Client scanning channel %d.\n", current_rf_channel);
			} else {
				// All channels checked, move to Client Channel Selection Menu
				printf("DEBUG: Client scan complete. Moving to Client Channel Selection Menu.\n");
				game_state_change(STATE_CLIENT_CHANNEL_SELECT);
				selected_channel_index = 0; // Default selection to first channel
				// current_rf_channel is not set here, it will be set when user selects a channel
			}
		}
		break;

	case STATE_LOBBY:    // This state might become redundant with new scan states
		tx_beacon(); // Keep sending beacons to discover/be discovered
		break;

	case STATE_JOINING:  // Client received BEGIN, waiting for confirmation
		tx_beacon(); // Clients in JOINING state send beacons to remain discoverable by host
		Player *self_player_tick = find_player(game_our_id);
		if (self_player_tick && self_player_tick->is_ready) {
			tx_ready(); // Clients in JOINING state also send READY messages if they have pressed START
		}
		break;

	case STATE_HOSTING:
		tx_begin(); // Keep sending begin messages to clients
		break;

	case STATE_READY_WAIT: // Host is in this state, waiting for all players to be ready
		tx_begin();    // Host keeps sending BEGIN to ensure clients get player assignments
		tx_ready();    // Host keeps sending its own READY status

		// Check if all active players are ready (same logic as in root_handle)
		bool all_ready_tick = true;
		if (players[0].id != 0 && !players[0].is_ready) {
			all_ready_tick = false;
		}
		if (players[1].id != 0 && !players[1].is_ready) {
			all_ready_tick = false;
		}

		if (all_ready_tick) {
			printf("DEBUG: All active players are ready (tick check). Triggering game start via handle!\n");
			// The actual state change and scene push will be triggered by root_handle
			// when the all_ready condition is met and SDK_PRESSED_START is handled.
			// If the host is already in STATE_READY_WAIT, the game start needs to be
			// explicitly triggered by the host's START press, or by an inbox message
			// from a client making the last player ready.
			// We rely on the `root_handle` to call `tx_game_start` and transition.
			// This tick check is primarily for debugging and ensuring consistency.
		}
		break;

	default: // STATE_PLAYING, STATE_ROUND_RESET
		break;
	}
}

/**
 * @brief Converts a player ID to a display color based on its assigned role.
 * This function should be called after player roles (P1, P2) are established.
 * @param id The player ID.
 * @return A color_t value.
 */
static color_t id_to_color(uint16_t id)
{
	// If the ID matches the current P1's ID (the host)
	if (players[0].id != 0 && id == players[0].id) {
		return BLUE; // Host is BLUE
	}
	// If the ID matches the current P2's ID
	if (players[1].id != 0 && id == players[1].id) {
		return GREEN; // Second player is GREEN
	}
	// For any other ID (should ideally not happen with 2-player limit in UI)
	return LGRAY; // Default to LGRAY
}

/**
 * @brief Scene inbox function: Processes incoming radio messages.
 * @param msg The received SDK message.
 * @param depth Current scene depth (only process if depth is 0 and it's an RF message).
 * @return True if the message was handled, false otherwise.
 */
static bool root_inbox(sdk_message_t msg, int depth)
{
	if (depth || msg.type != SDK_MSG_RF) {
		return false; // Only process base depth RF messages
	}

	// Handle MSG_BEACON: A device announcing its presence
	// Expected length: 1 (MSG_BEACON) + 2 (ID) = 3 bytes
	if (3 == msg.rf.length && MSG_BEACON == msg.rf.data[0]) {
		uint16_t id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		if (id == game_our_id) { // Ignore our own beacons
			return true;
		}

		// Host channel scan: If we receive any beacon, it means the channel is not "clean" for hosting
		// We only care about MSG_BEGIN for actual game occupancy, but any beacon means *somebody* is there.
		// If a beacon is received during host scan, it indicates activity.
		if (game_state == STATE_HOST_CHANNEL_SCAN) {
			// We are scanning. If we receive a beacon from another device,
			// it means the channel is not completely clean (someone is there).
			// We don't store the ID from a beacon, only from MSG_BEGIN.
			host_channel_is_busy_during_scan = true;
			printf("DEBUG: Host scan: Beacon from %04x received on channel %d. Channel is active.\n",
			       id, current_rf_channel);
			return true;
		}

		// Only process beacons if in STATE_LOBBY (client scanning) or STATE_HOSTING (host tracking) or READY_WAIT
		if (game_state == STATE_LOBBY || game_state == STATE_HOSTING ||
		    game_state == STATE_READY_WAIT) {
			// If hosting and both P1 and P2 slots are taken, ignore new beacons from unknown IDs
			if ((game_state == STATE_HOSTING || game_state == STATE_READY_WAIT) &&
			    players[0].id != 0 && players[1].id != 0 && id != players[0].id &&
			    id != players[1].id) {
				printf("DEBUG: Host ignoring beacon from %04x (2-player limit reached).\n",
				       id);
				return true; // Message handled (ignored)
			}

			Player *player = find_player(id); // Find or create player entry
			if (player == NULL) {
				printf("DEBUG: No available slot for beacon from %04x.\n", id);
				return true; // No slot, message handled (ignored)
			}

			// Update player info. Color will be set by id_to_color in paint or by MSG_BEGIN
			player->id = id;
			player->timeout = PLAYER_TIMEOUT; // Reset timeout
			player->player_idx = -1;	  // Not assigned a player_idx by beacon
			// Do NOT reset is_ready here, as beacons don't carry ready status
			printf("DEBUG: Received BEACON from %04x. Player timeout reset.\n", id);
			game_state_change(
				game_state); // Re-set state to reset timer on any beacon (for client)
			return true;
		}
		return false; // Ignore beacons in other states (e.g., STATE_MENU, STATE_JOINING, STATE_PLAYING)
	}

	// Handle MSG_BEGIN: A host initiating the game
	// Expected length: 1 (MSG_BEGIN) + 2 * NUM_ACTIVE_PLAYERS (for player IDs)
	// For NUM_ACTIVE_PLAYERS = 2, this is 1 + 2*2 = 5 bytes.
	if ((1 + 2 * NUM_ACTIVE_PLAYERS) == msg.rf.length && MSG_BEGIN == msg.rf.data[0]) {
		uint16_t host_id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		uint16_t player2_id = (msg.rf.data[3] << 8) | msg.rf.data[4];

		// Host channel scan: If we receive a MSG_BEGIN, this channel is busy with an active game
		if (game_state == STATE_HOST_CHANNEL_SCAN) {
			host_channel_is_busy_during_scan = true;
			scanned_channel_occupancy[channel_scan_index] =
				host_id; // Store the ID of the host
			printf("DEBUG: Host scan: MSG_BEGIN from %04x received on channel %d. Channel is busy.\n",
			       host_id, current_rf_channel);
			return true;
		}

		// Client channel scan: Check if this game is joinable
		if (game_state == STATE_CLIENT_CHANNEL_SCAN) {
			// If both slots are taken by OTHER devices, this channel is full for us
			if (host_id != 0 && host_id != game_our_id && player2_id != 0 &&
			    player2_id != game_our_id) {
				printf("DEBUG: Client scan: Game on channel %d is full (P1:%04x, P2:%04x). Cannot join.\n",
				       current_rf_channel, host_id, player2_id);
				scanned_channel_occupancy[channel_scan_index] =
					0; // Mark as not joinable for us
				client_game_found_on_channel =
					false; // Flag for tick to move to next channel
				return true;   // Message handled (channel is full)
			} else {
				// This game is joinable! Store host ID to indicate game found.
				printf("DEBUG: Client scan: Game found on channel %d (P1:%04x, P2:%04x). Marking for selection.\n",
				       current_rf_channel, host_id, player2_id);
				scanned_channel_occupancy[channel_scan_index] =
					host_id; // Store host ID to indicate game found
				client_game_found_on_channel =
					true; // Flag for tick to move to next channel
				return true;
			}
		}

		// Only process MSG_BEGIN if we are a client in STATE_LOBBY (scanning) or STATE_JOINING (to refresh player list)
		if (game_state == STATE_LOBBY || game_state == STATE_JOINING) {
			printf("DEBUG: Received BEGIN message. Current state: %d. Transitioning to STATE_JOINING.\n",
			       game_state);
			game_state_change(STATE_JOINING); // This resets timer

			// Temporarily store local device's ready status before clearing players array
			bool local_is_ready = false;
			Player *self_player_before_clear = find_player(game_our_id);
			if (self_player_before_clear &&
			    self_player_before_clear->id == game_our_id) {
				local_is_ready = self_player_before_clear->is_ready;
			}

			// Clear all existing player entries to rebuild based on host's message
			for (int i = 0; i < NUM_PLAYERS; i++) { // Loop only for NUM_PLAYERS (2)
				memset(&players[i], 0, sizeof(Player));
				players[i].player_idx = -1;
				players[i].is_ready = false; // Reset ready status for all
			}

			// Assign P1 (Host) from the message
			players[0].id = host_id;
			players[0].color = BLUE; // Host is always BLUE
			players[0].timeout = PLAYER_TIMEOUT;
			players[0].player_idx = 0; // Host is player_idx 0
			printf("DEBUG: P1 (Host): %04x\n", host_id);

			// Assign P2 from the message
			players[1].id = player2_id;
			players[1].color = GREEN; // P2 is now GREEN
			players[1].timeout = PLAYER_TIMEOUT;
			players[1].player_idx = 1; // Second player is player_idx 1
			printf("DEBUG: P2: %04x\n", player2_id);

			// Re-assign local device's ready status if it's P1 or P2
			Player *self_player_after_clear = find_player(game_our_id);
			if (self_player_after_clear && self_player_after_clear->id == game_our_id) {
				self_player_after_clear->is_ready = local_is_ready;
			} else if (self_player_after_clear == NULL &&
				   game_our_id != players[0].id && game_our_id != players[1].id) {
				// If our ID is not P1 or P2, and no slot was found, it means we are a spectator
				printf("DEBUG: Local device %04x is a spectator (no active player slot).\n",
				       game_our_id);
			}
			return true;
		}
		return false; // Ignore MSG_BEGIN in other states (e.g., STATE_MENU, STATE_HOSTING, STATE_PLAYING)
	}

	// Handle MSG_READY: A player confirming readiness
	// Expected length: 1 (MSG_READY) + 2 (ID) = 3 bytes
	if (3 == msg.rf.length && MSG_READY == msg.rf.data[0]) {
		uint16_t id = (msg.rf.data[1] << 8) | msg.rf.data[2];
		Player *player = find_player(id);
		if (player && player->id != 0) { // Ensure player exists and is valid
			player->is_ready = true;
			printf("DEBUG: Received READY from %04x. Player status: Ready.\n", id);

			// If we are the host and all active players are now ready, start the game
			if (game_state == STATE_READY_WAIT) {
				bool all_active_players_ready = true;
				// P1 (host) must be ready
				if (players[0].id != 0 && !players[0].is_ready) {
					all_active_players_ready = false;
				}
				// If P2 exists, P2 must also be ready
				if (players[1].id != 0 && !players[1].is_ready) {
					all_active_players_ready = false;
				}

				if (all_active_players_ready) {
					printf("DEBUG: All active players are ready (inbox check). Sending GAME_START!\n");
					tx_game_start(); // Send game start message to clients
					// HOST: Transition to STATE_ROUND_RESET for initial countdown
					game_state_change(STATE_ROUND_RESET);
					// Stop menu music before pushing game scene
					if (menu_melody != NULL) {
						sdk_melody_stop_and_release(menu_melody);
						menu_melody = NULL;
					}
					sdk_scene_push(&scene_round);
					game_reset(); // <--- HOST INITIALIZES GAME STATE HERE (calls new_round and sets dx/dy)
				}
			} else if (game_state ==
				   STATE_JOINING) { // Client receives READY from host/other client
				game_state_change(game_state); // Re-set state to reset timer
			}
			return true;
		}
		return false;
	}

	// Handle MSG_GAME_START: Host signaling clients to start the game
	// Expected length: 1 (MSG_GAME_START) = 1 byte
	if (1 == msg.rf.length && MSG_GAME_START == msg.rf.data[0]) {
		// Only clients in STATE_JOINING should react to this to start the game
		if (game_state == STATE_JOINING) {
			printf("DEBUG: Received GAME_START message. Transitioning to STATE_ROUND_RESET.\n"); // Client transitions to ROUND_RESET
			game_state_change(STATE_ROUND_RESET); // Resets timer
			// Stop menu music before pushing game scene
			if (menu_melody != NULL) {
				sdk_melody_stop_and_release(menu_melody);
				menu_melody = NULL;
			}
			sdk_scene_push(&scene_round); // Client pushes the game scene ONCE here
			game_reset(); // Client also calls game_reset to initialize its ball and paddles
			// Client will then wait for MSG_ROUND_START_COUNTDOWN from host to sync timer
			return true;
		}
		return false;
	}

	return false; // Message not handled
}

/**
 * @brief Scene revealed function: Called when this scene becomes active again after another scene was popped.
 * Resets lobby state and player data.
 */
static void root_revealed(void)
{
	printf("DEBUG: Root scene revealed. Resetting to menu.\n");
	game_state_change(STATE_MENU); // Go back to the main menu

	for (int i = 0; i < NUM_PLAYERS; i++) { // Loop only for NUM_PLAYERS (2)
		memset(&players[i], 0, sizeof(Player));
		players[i].player_idx = -1;
		players[i].color = LGRAY;    // Default to gray for all empty slots
		players[i].is_ready = false; // Reset ready status
	}
	for (int i = 0; i < (int)NUM_SCAN_CHANNELS; i++) { // Cast to int to fix warning
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
