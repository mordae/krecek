#ifndef COMMON_H
#define COMMON_H

#include <sdk.h>
#include <tft.h>
#include <stdint.h>  // For uint16_t, uint32_t, int16_t
#include <stdbool.h> // For bool type

// --- Colors ---
#define WHITE rgb_to_rgb565(255, 255, 255)
#define LGRAY rgb_to_rgb565(192, 192, 192) // Light Gray for inactive/third slot
#define BLUE rgb_to_rgb565(0, 0, 255)
#define BLACK rgb_to_rgb565(0, 0, 0) // Black for screen background
// Define GREEN if not already defined (from user's original pong.c)
#ifndef GREEN
#define GREEN rgb_to_rgb565(0, 191, 0)
#endif

// --- Game States ---
typedef enum {
	STATE_MENU,		     // Main menu to choose host or client
	STATE_HOST_CHANNEL_SCAN,     // Host is actively scanning channels to find a clean one
	STATE_CLIENT_CHANNEL_SCAN,   // Client is actively scanning channels to find a game to join
	STATE_ANTENNA_MENU,	     // Host's menu to manually select a channel after scan
	STATE_CLIENT_CHANNEL_SELECT, // Client's menu to manually select a channel after scan
	STATE_LOBBY,		     // Client: Scanning for games to join (might be redundant now)
	STATE_JOINING,		     // Client: Game found, waiting for confirmation to start
	STATE_HOSTING,		     // Host: Hosting a game and waiting for players
	STATE_READY_WAIT, // Host: Host has pressed START, waiting for clients to confirm readiness
	STATE_ROUND_RESET, // Game is paused for a countdown after a score (now also for initial game start)
	STATE_PLAYING, // Game is in progress
} GameState;

// --- Message Types for Radio Communication ---
typedef enum {
	MSG_BEACON,	 // Sent by devices to announce their presence
	MSG_BEGIN,	 // Sent by host to initiate the game with player assignments
	MSG_READY,	 // Sent by client/host to confirm readiness to start
	MSG_GAME_START,	 // Sent by host to tell clients to start the game
	MSG_BALL_STATE,	 // Sent by host to synchronize ball position and velocity periodically
	MSG_BALL_BOUNCE, // Sent by host when ball bounces off a paddle
	MSG_ROUND_START_COUNTDOWN, // Sent by host to signal start of round reset countdown (primary score sync)
	MSG_PADDLE_STATE,	 // Sent by host to synchronize its paddle state (P1)
	MSG_CLIENT_PADDLE_STATE, // Sent by client to synchronize its paddle state (P2)
				 // MSG_SCORE_UPDATE, // Removed
} MessageType;

// --- Game Constants ---
// Total number of player slots displayed in the lobby.
// Changed to 2 for strict 2-player game.
#define NUM_PLAYERS 2

// Number of actual playing entities (e.g., worms, paddles).
// For a 2-player game, this is 2.
#define NUM_ACTIVE_PLAYERS 2 // Renamed from NUM_WORMS for clarity

// Timeout for a player's presence in microseconds (e.g., 5 seconds).
// If a player doesn't send a beacon within this time, they are considered disconnected.
#define PLAYER_TIMEOUT 5000000

// RF Transmission Intervals (in microseconds)
#define BEACON_TX_INTERVAL_US 1000000		     // 1 second
#define BEGIN_TX_INTERVAL_US 1000000		     // 1 second
#define READY_TX_INTERVAL_US 1000000		     // 1 second
#define GAME_START_TX_INTERVAL_US 1500000	     // 1.5 seconds
#define BALL_STATE_TX_INTERVAL_US 250000	     // 0.25 seconds (250,000 microseconds)
#define ROUND_START_COUNTDOWN_TX_INTERVAL_US 1000000 // 1 second (for round start sync)
#define PADDLE_STATE_TX_INTERVAL_US 300000 // Host (P1) sends its paddle state every 0.3 seconds
#define CLIENT_PADDLE_STATE_TX_INTERVAL_US \
	220000 // Client (P2) sends its paddle state every 0.22 seconds

// common.h - Add these definitions
#define CHANNEL_SCAN_TIMEOUT_US 500000 // Reduced from 5s to 1s
#define GAME_MUSIC_VOLUME 30

// Add these message types for sound effects
#define MSG_SOUND_PADDLE_HIT 0x0A
#define MSG_SOUND_WALL_HIT 0x0B
#define MSG_SOUND_SCORE 0x0C

// Specific RF Channels for game
#define SDK_RF_CHANNEL_66 66
#define SDK_RF_CHANNEL_67 67
#define SDK_RF_CHANNEL_68 68

// --- Player Structure ---
// Represents a player in the game, including their ID, display color,
// timeout status, and assigned 'player_idx' (playing entity index).
typedef struct {
	uint16_t id;	// Unique identifier for the player
	color_t color;	// Display color for this player
	int timeout;	// Time until player is considered disconnected
	int player_idx; // Assigned playing entity index (0 for P1, 1 for P2, -1 if spectator)
	bool is_ready;	// True if this player has confirmed readiness
} Player;

// --- Global Variables (Declared here, Defined in root.c) ---
// Current state of the game lobby/matchmaking.
extern GameState game_state;
// Timer for various state transitions or delays.
extern int game_state_timer;
// The unique ID of the current device/player.
extern uint16_t game_our_id;
// Array to store information about all connected players.
extern Player players[NUM_PLAYERS];
// Current RF channel being used for the game
extern int current_rf_channel;
// Index for channel scanning (0 for 66, 1 for 67, 2 for 68)
extern int channel_scan_index;
// Stores the ID of a device found on a channel during scan, or 0 if clean.
extern uint16_t scanned_channel_occupancy[3]; // Max 3 channels (66, 67, 68)
// Index of the channel currently selected in the Antenna Menu
extern int selected_channel_index;

// Global declaration for the menu music melody
extern sdk_melody_t *menu_melody;

// --- External Scene Declarations ---
// Declare scene_root as external so main.c can link to it.
extern sdk_scene_t scene_root;
// Declare scene_round as external so root.c can link to it.
extern sdk_scene_t scene_round;

// --- Function Prototypes ---
// Changes the current game state and resets the state timer.
void game_state_change(GameState new_state);
// Finds a player by ID, or returns an empty slot if not found.
Player *find_player(uint16_t id);

#endif
