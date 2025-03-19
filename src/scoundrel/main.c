#include "sdk/image.h"
#include "sdk/input.h"
#include "sdk/util.h"
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>

#include <krecek-cards.png.h>
#include <health-counter.png.h>

#define GRAY rgb_to_rgb565(170, 170, 170)
#define RED rgb_to_rgb565(255, 64, 64)
#define HEARTS 0
#define SPADES 1
#define DIAMONDS 2
#define CLUBS 3

static int deck[52]; // -1 means card was dealt
static int deck_position = 0; // -1 means card was played
static int room_cards[4] = {-1, -1, -1, -1};
static int cursor_position = 0;
static int weapon;
static int last_weapon_use; // last card defeated with weapon. -1 means that the weapon wasn't used yet
static int player_health;
static bool using_weapon = false;


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
		while (deck[deck_position] < 0 || deck[deck_position] >= 52) {
			deck_position++;
			if (deck_position >= 52) {
				deck_position = 0;
			}
		}
		if (room_cards[i] < 0) {
			room_cards[i] = deck[deck_position];
			deck_position++;
			if (deck_position >= 52) {
				deck_position = 0;
			}
		}
	}
}

// 0 = hearts | 1 = spades | 2 = diamonds | 3 = clubs | 4 = jokers
int get_card_suit(int c) {
	return c / 13;
}

int get_card_value(int c) {
	return c % 13 + 2;
}

void game_audio(int __unused nsamples)
{
}

void game_reset(void)
{
	player_health = 20;
	for (int c = 0; c < 52; c++) {
		deck[c] = c;
	}
	shuffle_deck();
	new_room();
	weapon = -1;
	last_weapon_use = -1;
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
		const int card_value = get_card_value(room_cards[cursor_position]);
		const int card_suit = get_card_suit(room_cards[cursor_position]);
		printf("suit: %i | value: %i\n", card_suit, card_value);
		if (card_suit == HEARTS) {
			player_health = MIN(player_health + card_value, 20);
			room_cards[cursor_position] = -1;
		} else if (card_suit == DIAMONDS) {
			weapon = room_cards[cursor_position];
			last_weapon_use = -1;
			room_cards[cursor_position] = -1;
		} else { // spades or clubs
			if (using_weapon && (get_card_value(last_weapon_use) >= card_value || last_weapon_use == -1)) {
				player_health -= MAX(card_value - get_card_value(weapon), 0);
				last_weapon_use = room_cards[cursor_position];
				room_cards[cursor_position] = -1;
			} else if (!using_weapon) {
				player_health -= card_value;
				room_cards[cursor_position] = -1;
			}
		}
	}
	if (sdk_inputs_delta.x > 0) {
		using_weapon = !using_weapon;
	}
	int used_cards = 0;
	for (int i = 0; i < 4; i++) {
		if (room_cards[i] < 0)
			used_cards++;
	}
	if (used_cards >= 3) {
		new_room();
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	sdk_draw_tile(100, 100, &ts_health_counter_png, clampi(player_health, 0, 20));

	tft_draw_rect(8 + 30 * cursor_position, 8, 32 + 30 * cursor_position, 36, RED);
	for (int i = 0; i < 4; i++) {
		if (room_cards[i] > -1 && room_cards[i] < 52) {
			sdk_draw_tile(10 + 30 * i, 10, &ts_krecek_cards_png, room_cards[i]);
		}
	}
	if (weapon != -1) {
		sdk_draw_tile(10, 70, &ts_krecek_cards_png, weapon);
		if (last_weapon_use != -1)
			sdk_draw_tile(40, 70, &ts_krecek_cards_png, last_weapon_use);
	}
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
