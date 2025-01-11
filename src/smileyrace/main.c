#include "sdk/input.h"
#include "sys/types.h"
#include <assert.h>
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

#define CELL_SIZE 6
#define MAX_JOYSTICK_VALUE 2048

#define SCREEN_POS_X 77
#define SCREEN_POS_Y 57

struct player_state {
	int position_x;
	int position_y;
	int speed_x;
	int speed_y;
	int speed_max;
	int power;
	float friction; // how much you slow down when coliding with a wall
};

static struct player_state smiley;
static float push_x = 0.0f;
static float push_y = 0.0f;

static u_int32_t track[43] = {

	0b00000000111111000000000000000000,
	0b00000011111111111000000000000000,
	0b00000111111111111110000000000000,
	0b00001111111111111111000000000000,
	0b00011111110001111111000000000000,
	0b00011111100000111111100000000000,
	0b00111111100000011111100000000000,
	0b00111111100000011111100000000000,
	0b01111111100000011111100000000000,
	0b01111111000000011111100000000000,
	0b01111111000000011111100000000000,
	0b01111110000000111111100000000000,
	0b01111110000001111111000000000000,
	0b01111110000001111110000000000000,
	0b01111110000001111110000000000000,
	0b01111110000001111100000000000000,
	0b11111110000011111000000000000000,
	0b11111110000011111000000000000000,
	0b11111110000011111000000000000000,
	0b11111110000011111000000000000000,
	0b11111110000011111000000000000000,
	0b11111110000001111000000000000000,
	0b11111110000001111100000000000000,
	0b11111110000001111100000000000000,
	0b11111110000001111110000000000000,
	0b11111110000000111111100000000000,
	0b11111111000000011111111000000000,
	0b11111111000000000111111111100000,
	0b11111111000000000001111111111000,
	0b11111111100000000000011111111100,
	0b11111111100000000000011111111110,
	0b11111111100000000000011111111110,
	0b11111111110000000000011111111111,
	0b11111111111000000000111111111111,
	0b11111111111100000001111111111111,
	0b01111111111111111111111111111111,
	0b01111111111111111111111111111110,
	0b01111111111111111111111111111110,
	0b00111111111111111111111111111110,
	0b00011111111111111111111111111100,
	0b00001111111111111111111111110000,
	0b00000011111111111111111111000000,
	0b00000000111111111111100000000000,
};

bool is_cell_full(int x, int y) 
{
	assert(x >= 0 && x <= 31);
	assert(y >= 0 && y <= 42);
	if ((track[y] & 1 << (31 - x)) > 0)
		return true;
	else
		return false;
}

void clamp_speed() 
{
	float speed_proportion_x = smiley.speed_x / (smiley.speed_x + smiley.speed_y);
	float speed_proportion_y = smiley.speed_y / (smiley.speed_x + smiley.speed_y);
	smiley.speed_x = speed_proportion_x * smiley.speed_max;
	smiley.speed_y = speed_proportion_y * smiley.speed_max;
}

bool is_position_colliding(int x, int y) 
{
	int board_pos_x = x / CELL_SIZE;
	int board_pos_y = y / CELL_SIZE;
	int cell_position_x = x % CELL_SIZE;
	int cell_position_y = y % CELL_SIZE;
	
	if (is_cell_full(board_pos_x, board_pos_y)) {
		return true;
	}
	if (cell_position_x > 0 && is_cell_full(board_pos_x + 1, board_pos_y)) {
		return true;
	}
	if (cell_position_y > 0 && is_cell_full(board_pos_x, board_pos_y + 1)) {
		return true;
	}
	if (cell_position_x > 0 && cell_position_y > is_cell_full(board_pos_x + 1, board_pos_y + 1)) {
		return true;
	}
	return false;
}

void game_reset(void)
{
	smiley.position_x = 3 * CELL_SIZE;
	smiley.position_y = 17 * CELL_SIZE;
	smiley.speed_x = 0; smiley.speed_y = 0;
	smiley.speed_max = 1000;
	smiley.power = 100;
	smiley.friction = 0.8f;
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
	push_x = sdk_inputs.joy_x / MAX_JOYSTICK_VALUE;
	push_y = sdk_inputs.joy_y / MAX_JOYSTICK_VALUE;
}

void game_paint(unsigned __unused dt_usec)
{
	smiley.speed_x += push_x * smiley.power;
	smiley.speed_y += push_y * smiley.power;

	if (smiley.speed_x + smiley.speed_y > smiley.speed_max)
		clamp_speed();

	if (!is_position_colliding(smiley.position_x + (smiley.speed_x / 100), smiley.position_y + (smiley.speed_y / 100))) {
		smiley.position_x += smiley.speed_x;
		smiley.position_y += smiley.speed_y;
	}
	
	tft_fill(0);

	int top_left_pixel_x = smiley.position_x - SCREEN_POS_X;
	int top_left_pixel_y = smiley.position_y - SCREEN_POS_Y;
	int top_left_cell_x = top_left_pixel_x / CELL_SIZE;
	int top_left_cell_y = top_left_pixel_y / CELL_SIZE;
	int pixel_offset_x = top_left_pixel_x % CELL_SIZE;
	int pixel_offset_y = top_left_pixel_y % CELL_SIZE;

	for (int y = 0; y < 20 + 1; y++) {
		for (int x = 0; x < 28; x++) {
			int cell_color = 8;
			if (top_left_cell_x + x < 0 || top_left_cell_x + x > 31) {
				cell_color = 0;
			} else if (top_left_cell_y + y < 0 || top_left_cell_y > 42) {
				cell_color = 0;
			} else if (is_cell_full(top_left_cell_x + x, top_left_cell_y + y)) {
				cell_color = 0;
			}
			tft_draw_rect(x * CELL_SIZE - pixel_offset_x, y * CELL_SIZE - pixel_offset_y, 
			              (x + 1) * CELL_SIZE - pixel_offset_x, (y + 1) * CELL_SIZE - pixel_offset_y, 
			              cell_color);
		}
	}

	tft_draw_rect(SCREEN_POS_X, SCREEN_POS_Y, SCREEN_POS_X + CELL_SIZE, SCREEN_POS_Y + CELL_SIZE, GREEN);
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
