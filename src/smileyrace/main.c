#include "sys/types.h"
#include <cassert>
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

struct player_state {
	int position_x;
	int position_y;
	float speed_x;
	float speed_y;
	float speed_max;
	float power;
	float friction; // how much you slow down when coliding with a wall
};

static struct player_state smiley;

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

void game_reset(void)
{
	smiley.position_x = 3 * CELL_SIZE;
	smiley.position_y = 17 * CELL_SIZE;
	smiley.speed_x = 0; smiley.speed_y = 0;
	smiley.speed_max = 3.0f * CELL_SIZE;
	smiley.power = 0.5f;
	smiley.friction = 0.8f;
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
	if (smiley.speed_x + smiley.speed_y > smiley.speed_max)
		clamp_speed();
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);
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
