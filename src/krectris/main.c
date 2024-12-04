#include "sdk/input.h"
#include "sdk/util.h"
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

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
#define DARKGRAY 2
#define GRAY 10
#define WHITE 15

#define SPACE_SIZE 4
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

#define BOARD_OFFSET_X 60
#define BOARD_OFFSET_Y -68

#define NEXT_LIST_POS_X 11
#define NEXT_LIST_POS_Y 20

static uint8_t board[400] = {
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
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int active_piece_shape = 0;
static int active_piece_x = 0;
static int active_piece_y = BOARD_HEIGHT;
static int active_piece_orientation = 0;

static int movement_lr_das = 150000;
static int movement_lr_arr =  33333;
static int time_moved_lr = 0;

static int gravity_speed = 1000000; // how much time passes before the active piece falls
static int gravity_strength = 1; // how many spaces does the active piece fall
static int time_after_falling = 0;

static int piece_colors[7] = {
	RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE
};

static int active_bag[7] = {0, 1, 2, 3, 4, 5, 6};
static int future_bag[7] = {0, 1, 2, 3, 4, 5, 6};
static int next_piece_in_bag = 6;

static uint8_t piece_rotation[448] = { //todo: store in a more space efficient format
	1, 1, 0, 0, // z
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

	0, 0, 1, 0, // l
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

	0, 0, 0, 0, // o
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

	0, 1, 1, 0, // s
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

	0, 0, 0, 0, // i
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

	1, 0, 0, 0, // j
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

	0, 1, 0, 0, // t
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

static int8_t kick_table_x_clockwise[20] = { // based on orientation BEFORE rotation
	0, -1, -1, 0, -1,
	0, 1, 1, 0, 1,
	0, 1, 1, 0, 1,
	0, -1, -1, 0, -1
};

static int8_t kick_table_y_clockwise[20] = {
	0, 0, -1, 2, 2,
	0, 0, 1, -2, -2,
	0, 0, -1, 2, 2,
	0, 0, 1, -2, -2,
};

static int8_t kick_table_x_counterclockwise[20] = {
	0, 1, 1, 0, 1,
	0, 1, 1, 0, 1,
	0, -1, -1, 0, -1,
	0, -1, -1, 0, -1,
};

static int8_t kick_table_y_counterclockwise[20] = {
	0, 0, -1, 2, 2,
	0, 0, 1, -2, -2,
	0, 0, -1, 2, 2,
	0, 0, 1, -2, -2,
};

static int8_t kick_table_I_x_clockwise[20] = { // from tetrio's srs+
	0, 1, -2, -2, 1,
	0, -1, 2, -1, 2,
	0, 2, -1, 2, -1,
	0, 1, -2, 1, -2,
};

static int8_t kick_table_I_y_clockwise[20] = {
	0, 0, 0, 1, -2,
	0, 0, 0, -2, 1,
	0, 0, 0, -1, 2,
	0, 0, 0, 2, -1,
};

static int8_t kick_table_I_x_counterclockwise[20] = {
	0, -1, 2, 2, -1,
	0, 1, -2, 1, -2,
	0, -2, 1, -2, 1,
	0, -1, 2, -1, 2,
};

static int8_t kick_table_I_y_counterclockwise[20] = {
	0, 0, 0, 1, -2,
	0, 0, 0, 2, -1,
	0, 0, 0, -1, 2,
	0, 0, 0, 2, -1,
};

static void draw_mino(int x, int y, int color) {
	if (color == 0) {
		if (y >= BOARD_HEIGHT) {
			tft_draw_rect(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y,
				      (SPACE_SIZE * x + SPACE_SIZE - 1) + BOARD_OFFSET_X,
				      (SPACE_SIZE * y + SPACE_SIZE - 1) + BOARD_OFFSET_Y, DARKGRAY);
	
			tft_draw_rect(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y,
				      (SPACE_SIZE * x + SPACE_SIZE - 2) + BOARD_OFFSET_X,
				      (SPACE_SIZE * y + SPACE_SIZE - 2) + BOARD_OFFSET_Y, BLACK);
		}
	} else {
	tft_draw_rect(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y,
		      (SPACE_SIZE * x + SPACE_SIZE - 1) + BOARD_OFFSET_X,
		      (SPACE_SIZE * y + SPACE_SIZE - 1) + BOARD_OFFSET_Y, BLACK);
	
	tft_draw_rect(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y,
		      (SPACE_SIZE * x + SPACE_SIZE - 2) + BOARD_OFFSET_X,
		      (SPACE_SIZE * y + SPACE_SIZE - 2) + BOARD_OFFSET_Y, color);
	}
}

static int lookup_rotation_table(int x, int y, int shape, int orientation) {
	assert(shape >= 0 && shape <= 6);
	assert(orientation >= 0 && orientation <= 3);
	assert(x >= 0 && x <= 3);
	assert(y >= 0 && y <= 3);
	if (piece_rotation[shape * 64 + orientation * 16 + y * 4 + x] == 1)
		return 1;
	else
        	return 0;
}

static bool verify_piece_placement(int pos_x, int pos_y, int shape, int orientation) {
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, shape, orientation) == 0 && (pos_x + x < 0 || pos_x + x > BOARD_WIDTH - 1 || pos_y + y < 0 || pos_y + y > BOARD_HEIGHT * 2 - 1))
				continue;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 && (pos_x + x < 0 || pos_x + x > BOARD_WIDTH - 1 || pos_y + y < 0 || pos_y + y > BOARD_HEIGHT * 2 - 1))
				return false;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 && board[(pos_y + y) * BOARD_WIDTH + pos_x + x] != 0)
				return false;
		}
	}
	return true;
}

static void move_active_piece(int move_x, int move_y) {
	if (verify_piece_placement(active_piece_x + move_x, active_piece_y + move_y, active_piece_shape, active_piece_orientation) == true) {
		active_piece_x += move_x;
		active_piece_y += move_y;
	}
}

static void rotate_active_piece_clockwise() {
	int new_orientation;
	new_orientation = active_piece_orientation + 1;
	if (new_orientation > 3)
		new_orientation = 0;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(active_piece_x + kick_table_x_clockwise[active_piece_orientation * 5 + i], active_piece_y + kick_table_y_clockwise[active_piece_orientation * 5 + i], active_piece_shape, new_orientation) == true) {
			active_piece_x += kick_table_x_clockwise[active_piece_orientation * 5 + i];
			active_piece_y += kick_table_y_clockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_counterclockwise() {
	int new_orientation;
	new_orientation = active_piece_orientation - 1;
	if (new_orientation < 0)
		new_orientation = 3;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(active_piece_x + kick_table_x_counterclockwise[active_piece_orientation * 5 + i], active_piece_y + kick_table_y_counterclockwise[active_piece_orientation * 5 + i], active_piece_shape, new_orientation) == true) {
			active_piece_x += kick_table_x_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_y += kick_table_y_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_I_clockwise() {
	int new_orientation;
	new_orientation = active_piece_orientation + 1;
	if (new_orientation > 3)
		new_orientation = 0;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(active_piece_x + kick_table_I_x_clockwise[active_piece_orientation * 5 + i], active_piece_y + kick_table_I_y_clockwise[active_piece_orientation * 5 + i], active_piece_shape, new_orientation) == true) {
			active_piece_x += kick_table_I_x_clockwise[active_piece_orientation * 5 + i];
			active_piece_y += kick_table_I_y_clockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_I_counterclockwise() {
	int new_orientation;
	new_orientation = active_piece_orientation - 1;
	if (new_orientation < 0)
		new_orientation = 3;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(active_piece_x + kick_table_I_x_counterclockwise[active_piece_orientation * 5 + i], active_piece_y + kick_table_I_y_counterclockwise[active_piece_orientation * 5 + i], active_piece_shape, new_orientation) == true) {
			active_piece_x += kick_table_I_x_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_y += kick_table_I_y_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void lock_active_piece() {
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_piece_shape, active_piece_orientation) == 1) {
				board[(active_piece_y + y) * BOARD_WIDTH + active_piece_x + x] = piece_colors[active_piece_shape];
			}
		}
	}
}

static bool is_row_full(int row) {
	assert(row >= 0);
	assert(row <= BOARD_HEIGHT * 2 - 1);
	for (int x = 0; x <= 9; x++) {
		if (board[row * BOARD_WIDTH + x] == 0)
			return false;
	}
	return true;
}

static void swap_bag_entries(int a, int b) {
	int s;
	s = future_bag[b];
	future_bag[b] = future_bag[a];
	future_bag[a] = s;
}

static void generate_future_bag() {
	for (int i = 0; i <= 6; i++) {
		swap_bag_entries(i, rand() % 7);
	}
}


void game_reset(void)
{
	generate_future_bag();
	for (int i = 0; i <= 6; i++) {
		active_bag[i] = future_bag[i];
	}
	generate_future_bag();
	active_piece_shape = active_bag[next_piece_in_bag];
	next_piece_in_bag--;
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int __unused nsamples)
{
}

void game_input(unsigned dt_usec)
{
	if (sdk_inputs.joy_x > 1024 || sdk_inputs.joy_x < -1024) {
		if (time_moved_lr == 0) {
			if (sdk_inputs.joy_x > 0)
				move_active_piece(1, 0);
			else
				move_active_piece(-1, 0);
			time_moved_lr++;
		} else {
			if (time_moved_lr > movement_lr_das && time_moved_lr - movement_lr_das > movement_lr_arr) {
				if (sdk_inputs.joy_x > 0)
					move_active_piece(1, 0);
				else
					move_active_piece(-1, 0);
				time_moved_lr -= movement_lr_arr + dt_usec;
				
			}
		}
		time_moved_lr += dt_usec;
	} else
		time_moved_lr = 0;
	
	if (sdk_inputs.joy_y > 1024) {
		time_after_falling += dt_usec * 20;
	} else {
		time_after_falling += dt_usec;
	}
	if (time_after_falling > gravity_speed) {
		move_active_piece(0, gravity_strength);
		time_after_falling = 0;
	}

	if (sdk_inputs_delta.b > 0) {
		if (active_piece_shape != 4) // isn't I piece
			rotate_active_piece_clockwise();
		else
			rotate_active_piece_I_clockwise();
	}
	if (sdk_inputs_delta.a > 0) {
		if (active_piece_shape != 4)
			rotate_active_piece_counterclockwise();
		else
			rotate_active_piece_I_counterclockwise();
	}

	if (sdk_inputs_delta.y > 0)
		active_piece_shape++;
	if (active_piece_shape > 6)
		active_piece_shape = 0;

	if (sdk_inputs_delta.x > 0) {
		for (int i = 0; i <= BOARD_HEIGHT * 2 - 1; i++) {
			move_active_piece(0, 1);
		}
		lock_active_piece();
		active_piece_x = 0;
		active_piece_y = BOARD_HEIGHT;
		active_piece_orientation = 0;
		active_piece_shape = active_bag[next_piece_in_bag];

		next_piece_in_bag--;
		
		if (next_piece_in_bag < 0) {
			next_piece_in_bag = 6;
			for (int i = 0; i <= 6; i++) {
				active_bag[i] = future_bag[i];
			}
			generate_future_bag();
			next_piece_in_bag = 6;
		}

		for (int i = 0; i <= 3; i++) {
			int row_to_remove = -1;
			for (int row = BOARD_HEIGHT * 2 - 1; row >= 0; row--) {
				if (is_row_full(row)) {
					row_to_remove = row;
					break;
				}
			}
			if (row_to_remove >= 0) {
				for (int y = row_to_remove; y >= 1; y--) {
					for (int x = 0; x <= BOARD_WIDTH - 1; x++) {
						board[y * BOARD_WIDTH + x] = board[(y - 1) * BOARD_WIDTH + x];
					}
				}
				for (int x = 0; x <= BOARD_WIDTH - 1; x++)
					board[x] = 0;
			} else {
				break;
			}
		}
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int y = 0; y <= BOARD_HEIGHT * 2 - 1; y++) {
		for (int x = 0; x <= BOARD_WIDTH - 1; x++) {
			draw_mino(x, y, board[y * BOARD_WIDTH + x]);
		}
	}

	tft_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE), BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE), BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE), 175);
	tft_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE), BOARD_OFFSET_X -1, BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE), 175);
	tft_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE), BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE), BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE), 223);
	tft_draw_rect(BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE), BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE), BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE), BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE), 223);

	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_piece_shape, active_piece_orientation) == 1)
				draw_mino(active_piece_x + x, active_piece_y + y, piece_colors[active_piece_shape]);
		}
	}

	int next_piece = next_piece_in_bag;
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_bag[next_piece], active_piece_orientation) == 1)
				draw_mino(NEXT_LIST_POS_X + x, NEXT_LIST_POS_Y + y, piece_colors[active_bag[next_piece]]);
		}
	}

	bool use_future_bag = false;
	for (int i = 1; i <= 3; i++) {
		next_piece--;
		if (next_piece < 0) {
			next_piece = 6;
			use_future_bag = true;
		}
		if (!use_future_bag) {
			for (int y = 0; y <= 3; y++) {
				for (int x = 0; x <= 3; x++) {
					if (lookup_rotation_table(x, y, future_bag[next_piece], active_piece_orientation) == 1)
						draw_mino(NEXT_LIST_POS_X + x, NEXT_LIST_POS_Y + y + (i * 4), piece_colors[future_bag[next_piece]]);
				}
			}
		} else {
			for (int y = 0; y <= 3; y++) {
				for (int x = 0; x <= 3; x++) {
					if (lookup_rotation_table(x, y, active_bag[next_piece], active_piece_orientation) == 1)
						draw_mino(NEXT_LIST_POS_X + x, NEXT_LIST_POS_Y + y + (i * 4), piece_colors[active_bag[next_piece]]);
				}
			}
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
