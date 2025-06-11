#pragma once
#include <sdk.h>
#include <string.h>

typedef enum MessageType {
	MSG_BEACON = 0x10,
	MSG_BEGIN = 0x11,
	MSG_COORDS = 0x20,
} MessageType;

typedef enum GameState {
	STATE_LOBBY,
	STATE_HOSTING,
	STATE_JOINING,
	STATE_PLAYING,
} GameState;

extern GameState game_state;
extern int game_state_timer;
extern uint16_t game_our_id;

/*
 * Switch to new game state and reset timer.
 * Timer is not reset if we are already in that state.
 */
inline static void game_state_change(GameState new_state)
{
	if (game_state != new_state) {
		game_state = new_state;
		game_state_timer = 0;
	}
}

typedef struct Player {
	uint16_t id;
	color_t color;
	int timeout;
	int worm;
} Player;

#define PLAYER_TIMEOUT 1000000
#define NUM_PLAYERS 8
#define NUM_WORMS 4
extern Player players[NUM_PLAYERS];

/*
 * Locate slot of given player or first empty slot.
 */
inline static Player *find_player(uint16_t id)
{
	Player *slot = NULL;

	for (int i = 0; i < NUM_PLAYERS; i++) {
		if (!slot && !players[i].id)
			slot = &players[i];

		if (players[i].id == id)
			return &players[i];
	}

	return slot;
}
