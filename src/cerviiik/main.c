/*
 * Copyright (C) Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pico/stdlib.h>

#include <math.h>
#include <string.h>

#include <sdk.h>
#include <tft.h>

#define RED 255
#define YELLOW 241
#define GREEN 244
#define BLUE 234
#define GRAY 8
#define WHITE 15

#define ANGLE_DELTA (5.0f / 180.0f * M_PI)

struct worm {
	float x, y;
	float angle;
	float speed;
	uint8_t color;
	bool alive;
};

#define WIDTH 160
#define HEIGHT 120
static int8_t grid[WIDTH][HEIGHT];

#define NUM_WORMS 4
static struct worm worms[NUM_WORMS];
static struct worm worms_init[NUM_WORMS] = {
	{
		.x = WIDTH / 2.0f - 5,
		.y = HEIGHT / 2.0f - 5,
		.angle = 225.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = RED,
		.alive = true,
	},
	{
		.x = WIDTH / 2.0f + 5,
		.y = HEIGHT / 2.0f + 5,
		.angle = 45.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = GREEN,
		.alive = true,
	},
	{
		.x = WIDTH / 2.0f + 5,
		.y = HEIGHT / 2.0f - 5,
		.angle = 315.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = BLUE,
		.alive = true,
	},
	{
		.x = WIDTH / 2.0f - 5,
		.y = HEIGHT / 2.0f + 5,
		.angle = 135.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = YELLOW,
		.alive = true,
	},

};

inline static int clamp(int x, int lo, int hi)
{
	if (x < lo)
		return lo;

	if (x > hi)
		return hi;

	return x;
}

static void reset_game(void)
{
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			grid[x][y] = -1;
		}
	}

	memcpy(worms, worms_init, sizeof worms);
}

float angle_diff(float a, float b)
{
	float diff = fmodf(a - b, 2 * M_PI);

	if (diff < -M_PI)
		diff += 2 * M_PI;
	else if (diff > M_PI)
		diff -= 2 * M_PI;

	return diff;
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(2);

	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			int8_t owner = grid[x][y];

			if (owner < 0)
				continue;

			int8_t age = owner & 0x0f;
			owner >>= 4;

			if (owner >= 0)
				tft_draw_pixel(x, y, worms[owner].color);

			if (age)
				grid[x][y] = (owner << 4) | (age - 1);
		}
	}

	if (sdk_inputs.x)
		worms[0].angle -= ANGLE_DELTA;

	if (sdk_inputs.a)
		worms[0].angle += ANGLE_DELTA;

	if (sdk_inputs.b)
		worms[1].angle -= ANGLE_DELTA;

	if (sdk_inputs.y)
		worms[1].angle += ANGLE_DELTA;

	float p2_mag = sqrtf(powf(sdk_inputs.joy_x, 2) + powf(sdk_inputs.joy_y, 2));

	if (p2_mag > 200) {
		float new_angle = atan2f(sdk_inputs.joy_y, sdk_inputs.joy_x);
		float diff = angle_diff(worms[2].angle, new_angle);
		worms[2].angle -= diff * p2_mag / 2048 / 20;
	}

	if (sdk_inputs.vol_up)
		worms[3].angle -= ANGLE_DELTA;

	if (sdk_inputs.vol_down)
		worms[3].angle += ANGLE_DELTA;

	bool anybody_alive = false;

	for (int i = 0; i < NUM_WORMS; i++) {
		anybody_alive |= worms[i].alive;

		if (!worms[i].alive)
			continue;

		float hspd = worms[i].speed * cosf(worms[i].angle);
		float vspd = worms[i].speed * sinf(worms[i].angle);

		float x = worms[i].x += hspd;
		float y = worms[i].y += vspd;

		int nx[4] = {
			clamp(x + 0.5f, 0, WIDTH - 1),
			clamp(x + 0.5f, 0, WIDTH - 1),
			clamp(x - 0.5f, 0, WIDTH - 1),
			clamp(x - 0.5f, 0, WIDTH - 1),
		};

		int ny[4] = {
			clamp(y + 0.5f, 0, HEIGHT - 1),
			clamp(y - 0.5f, 0, HEIGHT - 1),
			clamp(y + 0.5f, 0, HEIGHT - 1),
			clamp(y - 0.5f, 0, HEIGHT - 1),
		};

		for (int j = 0; j < 4; j++) {
			int8_t owner = grid[nx[j]][ny[j]];
			int8_t age = owner & 0x0f;
			owner >>= 4;

			if ((owner >= 0) && (owner == i) && !age) {
				worms[i].alive = false;
				continue;
			}

			if ((owner >= 0) && (owner != i)) {
				worms[i].alive = false;
				continue;
			}

			grid[nx[j]][ny[j]] = (i << 4) | 0x0f;
		}

		if (worms[i].x < 0 || worms[i].x > WIDTH)
			worms[i].alive = false;

		if (worms[i].y < 0 || worms[i].y > HEIGHT)
			worms[i].alive = false;
	}

	if (!anybody_alive)
		reset_game();
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
	};

	sdk_main(&config);
}
