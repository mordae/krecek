#include "sdk/image.h"
#include "sdk/input.h"
#include "sdk/util.h"
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <tft.h>

#include <pieces.png.h>

#define RED rgb_to_rgb565(255, 0, 0)
#define ORANGE rgb_to_rgb565(255, 191, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define CYAN rgb_to_rgb565(0, 255, 255)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define PURPLE rgb_to_rgb565(255, 0, 255)
#define BLACK rgb_to_rgb565(0, 0, 0)
#define DARKGRAY rgb_to_rgb565(63, 63, 63)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define SPACE_SIZE 4
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

#define BOARD_OFFSET_X 60
#define BOARD_OFFSET_Y -60

#define NEXT_LIST_POS_X 11
#define NEXT_LIST_POS_Y 20
#define HOLD_PIECE_POS_X -5
#define HOLD_PIECE_POS_Y 20

static color_t board[400] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
};

static int active_piece_shape = 0;
static int active_piece_x = 0;
static int active_piece_y = BOARD_HEIGHT;
static int active_piece_orientation = 0;

static int ghost_piece_shape = 0;
static int ghost_piece_x = 0;
static int ghost_piece_y = 0;
static int ghost_piece_orientation = 0;

static int hold_piece = -1; // -1 when hold piece empty

static int movement_lr_das = 150000;
static int movement_lr_arr = 33333;
static int time_moved_lr = 0;

static int level_speeds[7] = {1000000, 500000, 250000, 125000, 62500, 31250, 15625};
static int max_level = 6; // always the number of levels - 1
static int current_level = 0;
static int lines_per_level = 12;
static int lines_cleared = 0;
static int gravity_speed = 1000000; // how much time passes before the active piece falls
static int time_after_falling = 0;

static int lock_time = 500000; // how much time you have on the ground before the piece locks
static int lock_movement = 15; // how many times you can move on the ground before the piece locks
static int lock_time_elapsed = 0;
static int lock_movements_used = 0;
static int lowest_row_reached = 0;

static int piece_colors[7] = { 1, 2, 3, 4, 5, 6, 7 };
static int piece_spawn_x[7] = { 3, 3, 3, 3, 3, 3, 3 };
static int piece_spawn_y[7] = { 18, 18, 17, 18, 18, 18, 18 };

static int active_bag[7] = { 0, 1, 2, 3, 4, 5, 6 };
static int future_bag[7] = { 0, 1, 2, 3, 4, 5, 6 };
static int next_piece_in_bag = 6;

static uint8_t piece_rotation[112] = {
	0b1100,	// z
	0b0110, 0b0000, 0b0000, 0b0010, 0b0110, 0b0100, 0b0000, 0b0000,
	0b1100, 0b0110, 0b0000, 0b0100, 0b1100, 0b1000, 0b0000,

	0b0010, // l
	0b1110, 0b0000, 0b0000, 0b0100, 0b0100, 0b0110, 0b0000, 0b0000,
	0b1110, 0b1000, 0b0000, 0b1100, 0b0100, 0b0100, 0b0000,

	0b0000, // o
	0b0110, 0b0110, 0b0000, 0b0000, 0b0110, 0b0110, 0b0000, 0b0000,
	0b0110, 0b0110, 0b0000, 0b0000, 0b0110, 0b0110, 0b0000,

	0b0110, // s
	0b1100, 0b0000, 0b0000, 0b0100, 0b0110, 0b0010, 0b0000, 0b0000,
	0b0110, 0b1100, 0b0000, 0b1000, 0b1100, 0b0100, 0b0000,

	0b0000, // i
	0b1111, 0b0000, 0b0000, 0b0010, 0b0010, 0b0010, 0b0010, 0b0000,
	0b0000, 0b1111, 0b0000, 0b0100, 0b0100, 0b0100, 0b0100,

	0b1000, // j
	0b1110, 0b0000, 0b0000, 0b0110, 0b0100, 0b0100, 0b0000, 0b0000,
	0b1110, 0b0010, 0b0000, 0b0100, 0b0100, 0b1100, 0b0000,

	0b0100, // t
	0b1110, 0b0000, 0b0000, 0b0100, 0b0110, 0b0100, 0b0000, 0b0000,
	0b1110, 0b0100, 0b0000, 0b0100, 0b1100, 0b0100, 0b0000
};

static int8_t kick_table_x_clockwise[20] = { // based on orientation BEFORE rotation
	0, -1, -1, 0, -1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, -1, -1, 0, -1
};

static int8_t kick_table_y_clockwise[20] = {
	0, 0, -1, 2, 2, 0, 0, 1, -2, -2, 0, 0, -1, 2, 2, 0, 0, 1, -2, -2,
};

static int8_t kick_table_x_counterclockwise[20] = {
	0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1,
};

static int8_t kick_table_y_counterclockwise[20] = {
	0, 0, -1, 2, 2, 0, 0, 1, -2, -2, 0, 0, -1, 2, 2, 0, 0, 1, -2, -2,
};

static int8_t kick_table_I_x_clockwise[20] = {
	// from tetrio's srs+
	0, 1, -2, -2, 1, 0, -1, 2, -1, 2, 0, 2, -1, 2, -1, 0, 1, -2, 1, -2,
};

static int8_t kick_table_I_y_clockwise[20] = {
	0, 0, 0, 1, -2, 0, 0, 0, -2, 1, 0, 0, 0, -1, 2, 0, 0, 0, 2, -1,
};

static int8_t kick_table_I_x_counterclockwise[20] = {
	0, -1, 2, 2, -1, 0, 1, -2, 1, -2, 0, -2, 1, -2, 1, 0, -1, 2, -1, 2,
};

static int8_t kick_table_I_y_counterclockwise[20] = {
	0, 0, 0, 1, -2, 0, 0, 0, 2, -1, 0, 0, 0, -1, 2, 0, 0, 0, 2, -1,
};

static void draw_mino(int x, int y, int color, bool ghost)
{
	if(color != 0) {
		if (!ghost) {
			sdk_draw_tile(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y, &ts_pieces_png, color - 1);
		} else {
			sdk_draw_tile(SPACE_SIZE * x + BOARD_OFFSET_X, SPACE_SIZE * y + BOARD_OFFSET_Y, &ts_pieces_png, 10);
		}
	}
}

static int lookup_rotation_table(int x, int y, int shape, int orientation)
{
	assert(shape >= 0 && shape <= 6);
	assert(orientation >= 0 && orientation <= 3);
	assert(x >= 0 && x <= 3);
	assert(y >= 0 && y <= 3);
	if ((piece_rotation[shape * 16 + orientation * 4 + y] & (8 >> x)) != 0)
		return 1;
	else
		return 0;
}

static bool verify_piece_placement(int pos_x, int pos_y, int shape, int orientation)
{
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, shape, orientation) == 0 &&
			    (pos_x + x < 0 || pos_x + x > BOARD_WIDTH - 1 || pos_y + y < 0 ||
			     pos_y + y > BOARD_HEIGHT * 2 - 1))
				continue;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 &&
			    (pos_x + x < 0 || pos_x + x > BOARD_WIDTH - 1 || pos_y + y < 0 ||
			     pos_y + y > BOARD_HEIGHT * 2 - 1))
				return false;
			if (lookup_rotation_table(x, y, shape, orientation) == 1 &&
			    board[(pos_y + y) * BOARD_WIDTH + pos_x + x] != 0)
				return false;
		}
	}
	return true;
}

static void move_active_piece(int move_x, int move_y)
{
	if (verify_piece_placement(active_piece_x + move_x, active_piece_y + move_y,
				   active_piece_shape, active_piece_orientation) == true) {
		active_piece_x += move_x;
		active_piece_y += move_y;
	}
}

static void rotate_active_piece_clockwise()
{
	int new_orientation;
	new_orientation = active_piece_orientation + 1;
	if (new_orientation > 3)
		new_orientation = 0;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(
			    active_piece_x +
				    kick_table_x_clockwise[active_piece_orientation * 5 + i],
			    active_piece_y +
				    kick_table_y_clockwise[active_piece_orientation * 5 + i],
			    active_piece_shape, new_orientation) == true) {
			active_piece_x += kick_table_x_clockwise[active_piece_orientation * 5 + i];
			active_piece_y += kick_table_y_clockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_counterclockwise()
{
	int new_orientation;
	new_orientation = active_piece_orientation - 1;
	if (new_orientation < 0)
		new_orientation = 3;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(
			    active_piece_x +
				    kick_table_x_counterclockwise[active_piece_orientation * 5 + i],
			    active_piece_y +
				    kick_table_y_counterclockwise[active_piece_orientation * 5 + i],
			    active_piece_shape, new_orientation) == true) {
			active_piece_x +=
				kick_table_x_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_y +=
				kick_table_y_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_I_clockwise()
{
	int new_orientation;
	new_orientation = active_piece_orientation + 1;
	if (new_orientation > 3)
		new_orientation = 0;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(
			    active_piece_x +
				    kick_table_I_x_clockwise[active_piece_orientation * 5 + i],
			    active_piece_y +
				    kick_table_I_y_clockwise[active_piece_orientation * 5 + i],
			    active_piece_shape, new_orientation) == true) {
			active_piece_x +=
				kick_table_I_x_clockwise[active_piece_orientation * 5 + i];
			active_piece_y +=
				kick_table_I_y_clockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void rotate_active_piece_I_counterclockwise()
{
	int new_orientation;
	new_orientation = active_piece_orientation - 1;
	if (new_orientation < 0)
		new_orientation = 3;
	for (int i = 0; i <= 4; i++) {
		if (verify_piece_placement(
			    active_piece_x +
				    kick_table_I_x_counterclockwise[active_piece_orientation * 5 +
								    i],
			    active_piece_y +
				    kick_table_I_y_counterclockwise[active_piece_orientation * 5 +
								    i],
			    active_piece_shape, new_orientation) == true) {
			active_piece_x +=
				kick_table_I_x_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_y +=
				kick_table_I_y_counterclockwise[active_piece_orientation * 5 + i];
			active_piece_orientation = new_orientation;
			return;
		}
	}
}

static void detirmine_ghost_position()
{
	ghost_piece_orientation = active_piece_orientation;
	ghost_piece_shape = active_piece_shape;
	ghost_piece_x = active_piece_x;
	ghost_piece_y = active_piece_y;

	for (int i = 0; i <= 20; i++) {
		if (verify_piece_placement(ghost_piece_x, ghost_piece_y + 1, ghost_piece_shape,
					   ghost_piece_orientation)) {
			ghost_piece_y++;
		}
	}
}

static bool is_row_full(int row)
{
	assert(row >= 0);
	assert(row <= BOARD_HEIGHT * 2 - 1);
	for (int x = 0; x <= 9; x++) {
		if (board[row * BOARD_WIDTH + x] == 0)
			return false;
	}
	return true;
}

static void swap_bag_entries(int a, int b)
{
	int s;
	s = future_bag[b];
	future_bag[b] = future_bag[a];
	future_bag[a] = s;
}

static void generate_future_bag()
{
	for (int i = 0; i <= 6; i++) {
		swap_bag_entries(i, rand() % 7);
	}
}

static bool is_on_ground()
{
	if (!verify_piece_placement(active_piece_x, active_piece_y + 1, active_piece_shape,
				    active_piece_orientation)) {
		return true;
	} else {
		return false;
	}
}

static void lock_active_piece()
{
	detirmine_ghost_position();
	active_piece_y = ghost_piece_y;
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_piece_shape,
						  active_piece_orientation) == 1) {
				board[(active_piece_y + y) * BOARD_WIDTH + active_piece_x + x] =
					piece_colors[active_piece_shape];
			}
		}
	}
	active_piece_orientation = 0;
	active_piece_shape = active_bag[next_piece_in_bag];
	active_piece_x = piece_spawn_x[active_piece_shape];
	active_piece_y = piece_spawn_y[active_piece_shape];
	lock_movements_used = 0;

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
			lines_cleared++;
			if (lines_cleared % lines_per_level == 0) {
				current_level = clampi(++current_level, 0, max_level);
				gravity_speed = level_speeds[current_level];
			}
			for (int y = row_to_remove; y >= 1; y--) {
				for (int x = 0; x <= BOARD_WIDTH - 1; x++) {
					board[y * BOARD_WIDTH + x] =
						board[(y - 1) * BOARD_WIDTH + x];
				}
			}
			for (int x = 0; x <= BOARD_WIDTH - 1; x++)
				board[x] = 0;
		} else {
			break;
		}
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

	active_piece_x = piece_spawn_x[active_piece_shape];
	active_piece_y = piece_spawn_y[active_piece_shape];
}

void game_start(void)
{
}

void game_audio(int __unused nsamples)
{
}

void game_input(unsigned dt_usec)
{
	if (is_on_ground()) {
		lock_time_elapsed += dt_usec;
		if (lock_time_elapsed > lock_time) {
			lock_active_piece();
		}
	} else {
		lock_time_elapsed = 0;
		if (active_piece_y > lowest_row_reached) {
			lowest_row_reached = active_piece_y;
			lock_movements_used = 0;
		}
	}

	if (sdk_inputs.joy_x > 1024 || sdk_inputs.joy_x < -1024) {
		if (time_moved_lr == 0) {
			if (sdk_inputs.joy_x > 0)
				move_active_piece(1, 0);
			else
				move_active_piece(-1, 0);
			time_moved_lr++;
			if (is_on_ground()) {
				lock_movements_used++;
				lock_time_elapsed = 0;
			}
			if (lock_movements_used > lock_movement) {
				lock_active_piece();
			}
		} else {
			if (time_moved_lr > movement_lr_das &&
			    time_moved_lr - movement_lr_das > movement_lr_arr) {
				if (sdk_inputs.joy_x > 0)
					move_active_piece(1, 0);
				else
					move_active_piece(-1, 0);
				time_moved_lr -= movement_lr_arr;
				if (is_on_ground()) {
					lock_movements_used++;
					lock_time_elapsed = 0;
				}
				if (lock_movements_used > lock_movement) {
					lock_active_piece();
				}
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
		move_active_piece(0, 1);
		time_after_falling = 0;
	}

	printf("%i\n", dt_usec);

	if (sdk_inputs_delta.b > 0) {
		if (active_piece_shape != 4) // isn't I piece
			rotate_active_piece_clockwise();
		else
			rotate_active_piece_I_clockwise();

		if (is_on_ground()) {
			lock_movements_used++;
			lock_time_elapsed = 0;
		}
		if (lock_movements_used > lock_movement) {
			lock_active_piece();
		}
	}
	if (sdk_inputs_delta.a > 0) {
		if (active_piece_shape != 4)
			rotate_active_piece_counterclockwise();
		else
			rotate_active_piece_I_counterclockwise();

		if (is_on_ground()) {
			lock_movements_used++;
			lock_time_elapsed = 0;
		}
		if (lock_movements_used > lock_movement) {
			lock_active_piece();
		}
	}

	if (sdk_inputs_delta.y > 0) {
		if (hold_piece == -1) { // hold used for the first time
			hold_piece = active_piece_shape;

			active_piece_orientation = 0;
			active_piece_shape = active_bag[next_piece_in_bag];
			active_piece_x = piece_spawn_x[active_piece_shape];
			active_piece_y = piece_spawn_y[active_piece_shape];

			next_piece_in_bag--;

			if (next_piece_in_bag < 0) {
				next_piece_in_bag = 6;
				for (int i = 0; i <= 6; i++) {
					active_bag[i] = future_bag[i];
				}
				generate_future_bag();
				next_piece_in_bag = 6;
			}
		} else {
			int new_piece;
			new_piece = hold_piece;
			hold_piece = active_piece_shape;

			active_piece_orientation = 0;
			active_piece_shape = new_piece;
			active_piece_x = piece_spawn_x[active_piece_shape];
			active_piece_y = piece_spawn_y[active_piece_shape];
		}
	}

	if (sdk_inputs_delta.x > 0) {
		lock_active_piece();
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int y = 0; y <= BOARD_HEIGHT * 2 - 1; y++) {
		for (int x = 0; x <= BOARD_WIDTH - 1; x++) {
			draw_mino(x, y, board[y * BOARD_WIDTH + x], false);
		}
	}

	tft_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE),
		      BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE),
		      BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE), 175);
	tft_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE),
		      BOARD_OFFSET_X - 1,
		      BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE),
		      175);
	tft_draw_rect(BOARD_OFFSET_X - 1,
		      BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE),
		      BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE),
		      BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE),
		      223);
	tft_draw_rect(BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE),
		      BOARD_OFFSET_Y - 1 + (BOARD_HEIGHT * SPACE_SIZE),
		      BOARD_OFFSET_X + (BOARD_WIDTH * SPACE_SIZE),
		      BOARD_OFFSET_Y + (BOARD_HEIGHT * SPACE_SIZE) + (BOARD_HEIGHT * SPACE_SIZE),
		      223);

	detirmine_ghost_position();
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, ghost_piece_shape,
						  ghost_piece_orientation) == 1)
				draw_mino(ghost_piece_x + x, ghost_piece_y + y,
					  piece_colors[active_piece_shape], true);
		}
	}

	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_piece_shape,
						  active_piece_orientation) == 1)
				draw_mino(active_piece_x + x, active_piece_y + y,
					  piece_colors[active_piece_shape], false);
		}
	}

	int next_piece = next_piece_in_bag;
	for (int y = 0; y <= 3; y++) {
		for (int x = 0; x <= 3; x++) {
			if (lookup_rotation_table(x, y, active_bag[next_piece], 0) == 1)
				draw_mino(NEXT_LIST_POS_X + x, NEXT_LIST_POS_Y + y,
					  piece_colors[active_bag[next_piece]], false);
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
					if (lookup_rotation_table(x, y, active_bag[next_piece],
								  0) == 1)
						draw_mino(NEXT_LIST_POS_X + x,
							  NEXT_LIST_POS_Y + y + (i * 4),
							  piece_colors[active_bag[next_piece]],
							  false);
				}
			}
		} else {
			for (int y = 0; y <= 3; y++) {
				for (int x = 0; x <= 3; x++) {
					if (lookup_rotation_table(x, y, future_bag[next_piece],
								  0) == 1)
						draw_mino(NEXT_LIST_POS_X + x,
							  NEXT_LIST_POS_Y + y + (i * 4),
							  piece_colors[future_bag[next_piece]],
							  false);
				}
			}
		}
	}

	if (hold_piece != -1) {
		for (int y = 0; y <= 3; y++) {
			for (int x = 0; x <= 3; x++) {
				if (lookup_rotation_table(x, y, hold_piece, 0) == 1)
					draw_mino(HOLD_PIECE_POS_X + x, HOLD_PIECE_POS_Y + y,
						  piece_colors[hold_piece], false);
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
