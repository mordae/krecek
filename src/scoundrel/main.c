#include "sdk/image.h"
#include "sdk/input.h"
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>

#include <krecek-cards.png.h>

#define GRAY rgb_to_rgb565(170, 170, 170)

static int deck[52]; // -1 means card was defeated/used
static int deck_position = 0;
static int room_cards[4];
static int cursor_position = 0;
static int weapon;
static int last_weapon_use; // -1 means that the weapon wasn't used yet


void shuffle_deck() {
	for (int c = 0; c < 52; c++) {
		int s = arc4random() % 52;
		int x = deck[c];
		deck[c] = deck[s];
		deck[s] = x;
	}
}

void new_room() {
	for (int i = 0; i < 4; i++) {
		room_cards[i] = deck[deck_position];
		deck_position++;
		if (deck_position >= 52) {
			deck_position = 0;
		}
	}
}

// 0 = hearts | 1 = spades | 2 = diamonds | 3 = clubs | 4 = jokers
int get_card_suit(int c) {
	return c / 52;
}

void game_audio(int __unused nsamples)
{
}

void game_reset(void)
{
	for (int c = 0; c < 52; c++) {
		deck[c] = c;
	}
	shuffle_deck();
	new_room();
	weapon = 0;
	last_weapon_use = 0;
}

void game_input(unsigned __unused dt_usec)
{
	if (sdk_inputs_delta.b > 0) {
		cursor_position++;
		bool position_detirmined = false;
		while (!position_detirmined) {
			if (cursor_position >= 4) {
				cursor_position = 0;
			} else if (room_cards[cursor_position] == -1) {
				cursor_position++;
			} else {
				position_detirmined = true;
			}
		}
	}
	if (sdk_inputs_delta.a > 0) {
		cursor_position--;
		bool position_detirmined = false;
		while (!position_detirmined) {
			if (cursor_position < 0) {
				cursor_position = 3;
			} else if (room_cards[cursor_position] == -1) {
				cursor_position--;
			} else {
				position_detirmined = true;
			}
		}
	}
	if (sdk_inputs_delta.y > 0) {
		
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	sdk_draw_tile(10, 10, &ts_krecek_cards_png, 0);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	printf("%d", 8);
	sdk_main(&config);
}
