#include "sdk/input.h"
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

#define RED 223
#define ORANGE 208
#define YELLOW 210
#define GREEN 212
#define CYAN 215
#define BLUE 217
#define PURPLE 219
#define BLACK 0
#define GRAY 10
#define WHITE 15

#define SPACE_SIZE 5

static uint8_t board[200] = {
	0, 0, 0, 0, 0, 0, 0, 0, 210, 210,
	0, 0, 0, 0, 0, 0, 0, 0, 210, 210,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	215, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	215, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	215, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	215, 219, 0, 0, 0, 0, 0, 0, 0, 0,
	219, 219, 219, 0, 0, 0, 0, 0, 0, 0,
};

static int active_piece_shape = 0;
static int active_piece_x = 0;
static int active_piece_y = 0;
static int active_piece_orientation = 0;
static bool active_piece_has_been_moved = false;

static int piece_colors[7] = {
	RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE
};

static uint8_t piece_orientation[448] = { //todo: store in a more space efficient format
	1, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	
	0, 0, 1, 0,
	0, 1, 1, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	1, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	1, 1, 0, 0,
	1, 0, 0, 0,
	0, 0, 0, 0,

	0, 0, 1, 0,
	1, 1, 1, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	0, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	1, 1, 1, 0,
	1, 0, 0, 0,
	0, 0, 0, 0,

	1, 1, 0, 0,
	0, 1, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 1, 1, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 1, 1, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 1, 1, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 1, 1, 0,
	0, 1, 1, 0,
	0, 0, 0, 0,

	0, 1, 1, 0,
	1, 1, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 1, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 1, 1, 0,
	1, 1, 0, 0,
	0, 0, 0, 0,

	1, 0, 0, 0,
	1, 1, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	1, 1, 1, 1,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 0, 1, 0,
	0, 0, 1, 0,
	0, 0, 1, 0,
	0, 0, 1, 0,

	0, 0, 0, 0,
	0, 0, 0, 0,
	1, 1, 1, 1,
	0, 0, 0, 0,

	0, 1, 0, 0,
	0, 1, 0, 0,
	0, 1, 0, 0,
	0, 1, 0, 0,

	1, 0, 0, 0,
	1, 1, 1, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 1, 1, 0,
	0, 1, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	1, 1, 1, 0,
	0, 0, 1, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	0, 1, 0, 0,
	1, 1, 0, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	1, 1, 1, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	0, 1, 1, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	1, 1, 1, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,

	0, 1, 0, 0,
	1, 1, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 0,
};

static void draw_mino(int x, int y, int color) {
	if (color == 0) {
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 1,
		      SPACE_SIZE * y + SPACE_SIZE - 1, color + 16);
	
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 2,
		      SPACE_SIZE * y + SPACE_SIZE - 2, color);
	} else {
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 2,
		      SPACE_SIZE * y + SPACE_SIZE - 2, color);
	}
}
static void lock_active_piece() {
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (piece_orientation[active_piece_shape * 64 + active_piece_orientation * 16 + y * 4 + x] == 1) {
				board[(active_piece_y + y) * 10 + active_piece_x + x] = piece_colors[active_piece_shape];
			}
		}
	}
}

void game_reset(void)
{
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int __unused nsamples)
{
}

void game_input(unsigned __unused dt_usec)
{
	if (active_piece_has_been_moved == false) {
		active_piece_has_been_moved = true;
		if (sdk_inputs.joy_x > 1024)
			active_piece_x++;
		else if (sdk_inputs.joy_x < -1024)
			active_piece_x--;
		else if (sdk_inputs.joy_y > 1024)
			active_piece_y++;
		else if (sdk_inputs.joy_y < -1024)
			active_piece_y--;
		else {
			active_piece_has_been_moved = false;
		}
	} else {
		if (sdk_inputs.joy_x < 1024 && sdk_inputs.joy_x > -1024) {
			if (sdk_inputs.joy_y < 1024 && sdk_inputs.joy_y > -1024)
				active_piece_has_been_moved = false;
		}
	}
	
	if (sdk_inputs_delta.b > 0)
		active_piece_orientation++;
	if (sdk_inputs_delta.a > 0)
		active_piece_orientation--;
	if (active_piece_orientation < 0)
		active_piece_orientation = 3;
	if (active_piece_orientation > 3)
		active_piece_orientation = 0;

	if (sdk_inputs_delta.y > 0)
		active_piece_shape++;
	if (active_piece_shape > 6)
		active_piece_shape = 0;

	if (sdk_inputs_delta.x > 0) {
		lock_active_piece();
		active_piece_x = 0;
		active_piece_y = 0;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int y = 0; y <= 19; y++) {
		for (int x = 0; x <= 9; x++) {
			draw_mino(x, y, board[y * 10 + x]);
		}
	}

	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (piece_orientation[active_piece_shape * 64 + active_piece_orientation * 16 + y * 4 + x] == 1)
				draw_mino(active_piece_x + x, active_piece_y + y, piece_colors[active_piece_shape]);
		}
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
