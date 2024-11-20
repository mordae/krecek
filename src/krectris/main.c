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

#define SPACE_SIZE 5

#define BOARD_OFFSET_X 20
#define BOARD_OFFSET_Y 10

static uint8_t board[200] = {
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
static int active_piece_y = 0;
static int active_piece_orientation = 0;
static int movement_lr_das = 150000;
static int movement_lr_arr =  33333;
static int movement_d_arr =   33333;
static int time_moved_lr = 0;
static int time_moved_d = 0;

static int piece_colors[7] = {
	RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE
};

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
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 1,
		      SPACE_SIZE * y + SPACE_SIZE - 1, DARKGRAY);
	
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 2,
		      SPACE_SIZE * y + SPACE_SIZE - 2, BLACK);
	} else {
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 1,
		      SPACE_SIZE * y + SPACE_SIZE - 1, BLACK);
	
	tft_draw_rect(SPACE_SIZE * x, SPACE_SIZE * y,
		      SPACE_SIZE * x + SPACE_SIZE - 2,
		      SPACE_SIZE * y + SPACE_SIZE - 2, color);
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
			if (lookup_rotation_table(x, y, shape, orientation) == 0 && (pos_x + x < 0 || pos_x + x > 9 || pos_y + y < 0 || pos_y + y > 19))
				continue;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 && (pos_x + x < 0 || pos_x + x > 9 || pos_y + y < 0 || pos_y + y > 19))
				return false;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 && board[(pos_y + y) * 10 + pos_x + x] != 0)
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
	static int new_orientation;
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
	static int new_orientation;
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
	static int new_orientation;
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
	static int new_orientation;
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
		if (time_moved_d == 0) {
			move_active_piece(0, 1);
			time_moved_d++;
		} else {
			if (time_moved_d > movement_d_arr) {
				move_active_piece(0, 1);
				time_moved_d -= movement_d_arr = dt_usec;
			}
		}
		time_moved_d += dt_usec;
	} else
		time_moved_d = 0;

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
			if (lookup_rotation_table(x, y, active_piece_shape, active_piece_orientation) == 1)
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
