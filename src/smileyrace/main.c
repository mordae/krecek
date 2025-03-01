#include "sdk/input.h"
#include "sdk/util.h"
#include "sys/types.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <pico/stdlib.h>

#include <stdbool.h>

#include <sdk.h>
#include <tft.h>

#define RED rgb_to_rgb565(255, 0, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define CELL_SIZE 6
#define MAX_JOYSTICK_VALUE 2048

#define SCREEN_POS_X 77
#define SCREEN_POS_Y 57

struct vector2 {
	float x;
	float y;
};

struct smiley {
	struct vector2 position;
	struct vector2 direction;
	float speed;
	float power;
	int color;
	float max_speed;
};

static struct smiley player;

static u_int32_t track[43] = {
	0b00000000111111000000000000000000, //
	0b00000011111111111000000000000000, //
	0b00000111111111111110000000000000, //
	0b00001111111111111111000000000000, //
	0b00011111110001111111000000000000, //
	0b00011111100000111111100000000000, //
	0b00111111100000011111100000000000, //
	0b00111111100000011111100000000000, //
	0b01111111100000011111100000000000, //
	0b01111111000000011111100000000000, //
	0b01111111000000011111100000000000, //
	0b01111110000000111111100000000000, //
	0b01111110000001111111000000000000, //
	0b01111110000001111110000000000000, //
	0b01111110000001111110000000000000, //
	0b01111110000001111100000000000000, //
	0b11111110000011111100000000000000, //
	0b11111110000011111100000000000000, //
	0b11111110000011111100000000000000, //
	0b11111110000011111100000000000000, //
	0b11111110000011111100000000000000, //
	0b11111110000001111110000000000000, //
	0b11111110000001111110000000000000, //
	0b11111110000001111111000000000000, //
	0b11111110000001111111100000000000, //
	0b11111110000000111111111000000000, //
	0b11111111000000011111111110000000, //
	0b11111111000000000111111111100000, //
	0b11111111000000000001111111111000, //
	0b11111111100000000000111111111100, //
	0b11111111100000000000011111111110, //
	0b11111111100000000000011111111110, //
	0b11111111110000000000011111111111, //
	0b11111111111000000000111111111111, //
	0b11111111111100000001111111111111, //
	0b01111111111111111111111111111111, //
	0b01111111111111111111111111111110, //
	0b01111111111111111111111111111110, //
	0b00111111111111111111111111111110, //
	0b00011111111111111111111111111100, //
	0b00001111111111111111111111110000, //
	0b00000011111111111111111111000000, //
	0b00000000111111111111100000000000, //
};

bool is_cell_full(int x, int y)
{
	if (x < 0 || x > 31) {
		return true;
	}
	if (y < 0 || y > 42) {
		return true;
	}
	if ((track[y] & 1 << (31 - x)) > 0)
		return false;
	else
		return true;
}

float get_vector_magnitude(struct vector2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

struct vector2 normalise(struct vector2 v)
{
	float m = get_vector_magnitude(v);
	if (!m) {
		return v;
	}
	struct vector2 ov;
	ov.x = v.x / m;
	ov.y = v.y / m;
	return ov;
}

bool is_position_valid(struct vector2 pos)
{
	if (pos.x < 0 || pos.y < 0) {
		return false;
	}
	struct vector2 corner_positions[4];
	corner_positions[0].x = pos.x;
	corner_positions[0].y = pos.y;
	corner_positions[1].x = pos.x + CELL_SIZE - 1;
	corner_positions[1].y = pos.y;
	corner_positions[2].x = pos.x;
	corner_positions[2].y = pos.y + CELL_SIZE - 1;
	corner_positions[3].x = pos.x + CELL_SIZE - 1;
	corner_positions[3].y = pos.y + CELL_SIZE - 1;
	for (int i = 0; i < 4; i++) {
		if (is_cell_full((int)corner_positions[i].x / CELL_SIZE,
				 (int)corner_positions[i].y / CELL_SIZE)) {
			return false;
		}
	}
	return true;
}

void game_reset(void)
{
	player = (struct smiley){ .position = (struct vector2){ 3 * CELL_SIZE, 17 * CELL_SIZE },
				  .direction = (struct vector2){ 0, 0 },
				  .speed = 0,
				  .power = 1,
				  .color = 213,
				  .max_speed = 0.5 * CELL_SIZE };
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
	float dt = dt_usec / 1000000.0f;

	if (!dt) {
		return;
	}

	struct vector2 input = (struct vector2){ sdk_inputs.joy_x, sdk_inputs.joy_y };
	if (sdk_inputs.b > 0) {
		input.x = MAX_JOYSTICK_VALUE;
	}
	if (sdk_inputs.x > 0) {
		input.x = -MAX_JOYSTICK_VALUE;
	}
	if (sdk_inputs.a > 0) {
		input.y = MAX_JOYSTICK_VALUE;
	}
	if (sdk_inputs.y > 0) {
		input.y = -MAX_JOYSTICK_VALUE;
	}

	struct vector2 push;
	push.x = player.power * dt * input.x / MAX_JOYSTICK_VALUE;
	push.y = player.power * dt * input.y / MAX_JOYSTICK_VALUE;

	struct vector2 temp;
	temp.x = player.direction.x * player.speed + push.x;
	temp.y = player.direction.y * player.speed + push.y;

	player.speed = get_vector_magnitude(temp);
	player.direction = normalise(temp);

	player.speed = clamp(player.speed, 0, player.max_speed);

	struct vector2 target_position;
	target_position.x = player.position.x + player.speed * player.direction.x;
	target_position.y = player.position.y + player.speed * player.direction.y;
	if (is_position_valid(target_position)) {
		player.position.x = target_position.x;
		player.position.y = target_position.y;
	} else {
		player.speed = 0;
		// game_reset(); // harder game mode
	}
	// printf("posX: %.3f | posY: %.3f | spd: %.3f\n", player.position.x, player.position.y, player.speed);
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	int top_left_pixel_x = player.position.x - SCREEN_POS_X;
	int top_left_pixel_y = player.position.y - SCREEN_POS_Y;
	int top_left_cell_x = top_left_pixel_x / CELL_SIZE;
	int top_left_cell_y = top_left_pixel_y / CELL_SIZE;
	int pixel_offset_x = top_left_pixel_x % CELL_SIZE;
	int pixel_offset_y = top_left_pixel_y % CELL_SIZE;

	for (int y = -1; y < 20 + 1; y++) {
		for (int x = -1; x < 28; x++) {
			int cell_color = 0;
			if (top_left_cell_x + x < 0 || top_left_cell_x + x > 31) {
				cell_color = 8;
			} else if (top_left_cell_y + y < 0 || top_left_cell_y > 42) {
				cell_color = 8;
			} else if (is_cell_full(top_left_cell_x + x, top_left_cell_y + y)) {
				cell_color = 8;
			}
			int ox = x * CELL_SIZE - pixel_offset_x;
			int oy = y * CELL_SIZE - pixel_offset_y;
			if (ox < 0) {
				ox = 0;
			}
			if (oy < 0) {
				oy = 0;
			}
			tft_draw_rect(ox, oy, ox + CELL_SIZE, oy + CELL_SIZE, cell_color);
		}
	}

	tft_draw_rect(SCREEN_POS_X, SCREEN_POS_Y, SCREEN_POS_X + CELL_SIZE - 1,
		      SCREEN_POS_Y + CELL_SIZE - 1, player.color);
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
