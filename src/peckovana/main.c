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

#include <sdk.h>
#include <tft.h>

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

struct hamster {
	float y;
	float dy;
	uint8_t color;
	float px, py;
	int hp;
};

static struct hamster p1, p2;

uint32_t heart_sprite[32] = {
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00011100011100000000000000000000, /* do not wrap please */
	0b00111110111110000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b00111111111110000000000000000000, /* do not wrap please */
	0b00011111111100000000000000000000, /* do not wrap please */
	0b00001111111000000000000000000000, /* do not wrap please */
	0b00000111110000000000000000000000, /* do not wrap please */
	0b00000011100000000000000000000000, /* do not wrap please */
	0b00000001000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
};

static void draw_sprite(int x0, int y0, uint32_t sprite[32], int color, bool transp)
{
	int x1 = x0 + 32;
	int y1 = y0 + 32;

	for (int y = y0; y < y1; y++) {
		for (int x = x0; x < x1; x++) {
			bool visible = (sprite[(y - y0)] << (x - x0)) >> 31;

			if (!visible && transp)
				continue;

			int c = color * visible;
			tft_draw_pixel(x, y, c);
		}
	}
}

void game_reset(void)
{
	p1.color = RED;
	p1.dy = 0;
	p1.y = tft_height - 31;
	p1.px = -1;
	p1.py = -1;
	p1.hp = 3;

	p2.color = GREEN;
	p2.dy = 0;
	p2.y = tft_height - 31;
	p2.px = -1;
	p2.py = -1;
	p2.hp = 3;
}

void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	tft_fill(0);

	float bottom = tft_height - 31;

	/*
	 * Draw hamsters
	 */

	tft_draw_rect(0, p1.y, 23, p1.y + 31, p1.color);
	tft_draw_rect(tft_width - 24, p2.y, tft_width - 1, p2.y + 31, p2.color);

	/*
	 * Draw hearts
	 */

	for (int i = 0; i < p1.hp; i++)
		draw_sprite(28 + 16 * i, 4, heart_sprite, RED, true);

	for (int i = 0; i < p2.hp; i++)
		draw_sprite(tft_width - 17 - (28 + 16 * i), 4, heart_sprite, GREEN, true);

	/*
	 * Jumping
	 */

	if ((p1.y >= tft_height - 31) && sdk_inputs.a)
		p1.dy = -tft_height * 1.15;

	if ((p1.px < 0) && sdk_inputs.x) {
		p1.px = 24;
		p1.py = p1.y + 16;
	}

	if ((p2.y >= tft_height - 31) && sdk_inputs.y)
		p2.dy = -tft_height * 1.15;

	if ((p2.px < 0) && sdk_inputs.b) {
		p2.px = tft_width - 25;
		p2.py = p2.y + 16;
	}

	/*
	 * Vertical movement
	 */

	p1.y += p1.dy * dt;
	p2.y += p2.dy * dt;

	/*
	 * Gravitation
	 */

	p1.dy += (float)tft_height * dt;
	p2.dy += (float)tft_height * dt;

	/*
	 * Fall boosting
	 */

	if (p1.dy > 0 && sdk_inputs.a) {
		p1.dy += (float)tft_height * dt;
	}

	if (p2.dy > 0 && sdk_inputs.y) {
		p2.dy += (float)tft_height * dt;
	}

	/*
	 * Cap acceleration and keep hamsters above floor
	 */

	if (p1.dy > tft_height)
		p1.dy = tft_height;

	if (p2.dy > tft_height)
		p2.dy = tft_height;

	if (p1.y >= bottom)
		p1.y = bottom;

	if (p2.y >= bottom)
		p2.y = bottom;

	/*
	 * Draw projectiles
	 */

	if (p1.px >= 0)
		tft_draw_rect(p1.px - 1, p1.py - 1, p1.px + 1, p1.py + 1, p1.color);

	if (p2.px >= 0)
		tft_draw_rect(p2.px - 1, p2.py - 1, p2.px + 1, p2.py + 1, p2.color);

	/*
	 * Mid-air projectile collissions
	 */

	if (p1.px >= 0 && p2.px >= 0) {
		if ((p1.py <= p2.py + 1) && (p1.py >= p2.py - 1)) {
			/* Projectiles are at about the same height. */

			if (p1.px >= p2.px) {
				/* They must have collided. */
				p1.px = -1;
				p2.px = -1;
			}
		}
	}

	/*
	 * Horizontal projectile movement
	 */

	float pdistance = 0.5 * (float)tft_width * dt;

	if (p1.px >= 0)
		p1.px += pdistance;

	if (p2.px >= 0)
		p2.px -= pdistance;

	if (p1.px >= tft_width)
		p1.px = -1;

	if (p2.px < 0)
		p2.px = -1;

	/*
	 * Projectile-hamster collissions
	 */

	if (p1.px >= 0) {
		if (p1.py >= p2.y && p1.py < (p2.y + 32)) {
			if (p1.px >= tft_width - 24) {
				p1.px = -1;
				p2.hp -= 1;

				if (p2.hp < 1)
					game_reset();
			}
		}
	}

	if (p2.px >= 0) {
		if (p2.py >= p1.y && p2.py < (p1.y + 32)) {
			if (p2.px < 24) {
				p2.px = -1;
				p1.hp -= 1;

				if (p1.hp < 1)
					game_reset();
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
		.target_fps = 40.5,
	};

	sdk_main(&config);
}
