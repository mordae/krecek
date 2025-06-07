#pragma once
#include "tft.h"

typedef enum GameState {
	STATE_LOBBY,
	STATE_HOSTING,
	STATE_PLAYING,
} GameState;

extern GameState game_state;

typedef struct Player {
	uint16_t id;
	color_t color;
} Player;

#define NUM_PLAYERS 4
extern Player players[NUM_PLAYERS];

extern int game_slot;
