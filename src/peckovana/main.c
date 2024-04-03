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

#include <pico/multicore.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/pwm.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <task.h>
#include <tft.h>
#include <dap.h>

#define DAP_SWDIO_PIN 25
#define DAP_SWCLK_PIN 24

#define DAP_CORE0 0x01002927u
#define DAP_CORE1 0x11002927u
#define DAP_RESCUE 0xf1002927u

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

#define SLAVE_A_PIN 22
#define SLAVE_B_PIN 23
#define SLAVE_Y_PIN 24
#define SLAVE_X_PIN 25

#define SLAVE_START_PIN 19
#define SLAVE_SELECT_PIN 20

static bool p1_up_btn = 0;
static bool p1_gun_btn = 0;

static bool p2_up_btn = 0;
static bool p2_gun_btn = 0;

static void stats_task(void);
static void tft_task(void);
static void input_task(void);

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

#define WIDTH 160
#define HEIGHT 120

/*
 * Tasks to run concurrently:
 */
task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		/* On the first core: */
		MAKE_TASK(4, "stats", stats_task),
		MAKE_TASK(1, "input", input_task),
		NULL,
	},
	{
		/* On the second core: */
		MAKE_TASK(1, "tft", tft_task),
		NULL,
	},
};

/*
 * Reports on all running tasks every 10 seconds.
 */
static void stats_task(void)
{
	while (true) {
		task_sleep_ms(10 * 1000);

		for (unsigned i = 0; i < NUM_CORES; i++)
			task_stats_report_reset(i);
	}
}

inline static int slave_gpio_get(int pin)
{
	uint32_t tmp;

	if (!dap_peek(0x40014000 + 8 * pin, &tmp)) {
		puts("slave_gpio_get: dap_peek failed");
		return 0;
	}

	return (tmp >> 17) & 1;
}

/*
 * Processes joystick and button inputs.
 */
static void input_task(void)
{
	task_sleep_ms(300);

	while (true) {
		p1_up_btn = !slave_gpio_get(SLAVE_A_PIN);
		p1_gun_btn = !slave_gpio_get(SLAVE_B_PIN);

		p2_up_btn = !slave_gpio_get(SLAVE_X_PIN);
		p2_gun_btn = !slave_gpio_get(SLAVE_Y_PIN);

		if (!slave_gpio_get(SLAVE_SELECT_PIN)) {
			puts("SELECT");
			dap_poke(0x40018004, 0x331f);
		}

		task_sleep_ms(10);
	}
}

inline __unused static int clamp(int x, int lo, int hi)
{
	if (x < lo)
		return lo;

	if (x > hi)
		return hi;

	return x;
}

static void reset_game(void)
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

/*
 * Outputs stuff to the screen as fast as possible.
 */
static void tft_task(void)
{
	uint32_t last_sync = time_us_32();
	int fps = 30;

	reset_game();

	while (true) {
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

		if ((p1.y >= tft_height - 31) && p1_up_btn)
			p1.dy = -tft_height * 1.15;

		if ((p1.px < 0) && p1_gun_btn) {
			p1.px = 24;
			p1.py = p1.y + 16;
		}

		if ((p2.y >= tft_height - 31) && p2_up_btn)
			p2.dy = -tft_height * 1.15;

		if ((p2.px < 0) && p2_gun_btn) {
			p2.px = tft_width - 25;
			p2.py = p2.y + 16;
		}

		/*
		 * Vertical movement
		 */

		p1.y += p1.dy / fps;
		p2.y += p2.dy / fps;

		/*
		 * Gravitation
		 */

		p1.dy += (float)tft_height / fps;
		p2.dy += (float)tft_height / fps;

		/*
		 * Fall boosting
		 */

		if (p1.dy > 0 && p1_up_btn) {
			p1.dy += (float)tft_height / fps;
		}

		if (p2.dy > 0 && p2_up_btn) {
			p2.dy += (float)tft_height / fps;
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

		float pdistance = 0.5 * (float)tft_width / fps;

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
						reset_game();
				}
			}
		}

		if (p2.px >= 0) {
			if (p2.py >= p1.y && p2.py < (p1.y + 32)) {
				if (p2.px < 24) {
					p2.px = -1;
					p1.hp -= 1;

					if (p1.hp < 1)
						reset_game();
				}
			}
		}

		/*
		 * FPS and others
		 */

		char buf[64];

		snprintf(buf, sizeof buf, "%i", fps);
		tft_draw_string_right(tft_width - 1, 0, GRAY, buf);

		tft_swap_buffers();
		task_sleep_ms(3);
		tft_sync();

		uint32_t this_sync = time_us_32();
		uint32_t delta = this_sync - last_sync;
		fps = 1 * 1000 * 1000 / delta;
		last_sync = this_sync;
	}
}

int main()
{
	stdio_usb_init();
	task_init();

	for (int i = 0; i < 30; i++) {
		if (stdio_usb_connected())
			break;

		sleep_ms(100);
	}

	adc_init();

	for (int i = 0; i < 16; i++)
		srand(adc_read() + random());

	tft_init();

	printf("Hello, have a nice and productive day!\n");

	dap_init(DAP_SWDIO_PIN, DAP_SWCLK_PIN);
	dap_reset();

	dap_select_target(DAP_CORE0);
	unsigned idcode = dap_read_idcode();
	printf("idcode = %#010x\n", idcode);
	dap_setup_mem((uint32_t *)&idcode);
	printf("idr = %#010x\n", idcode);
	dap_noop();

	/* Un-reset stuff that's ok with clk_sys and clk_ref */
	dap_poke(0x4000c000, 0x1e3bc9d);

	/* Enable display backlight. */
	dap_poke(0x4001406c, 0x331f);

	/* Enable button input + pull-ups. */
	dap_poke(0x4001c000 + 4 + 4 * SLAVE_A_PIN, (1 << 3) | (1 << 6));
	dap_poke(0x4001c000 + 4 + 4 * SLAVE_B_PIN, (1 << 3) | (1 << 6));
	dap_poke(0x4001c000 + 4 + 4 * SLAVE_X_PIN, (1 << 3) | (1 << 6));
	dap_poke(0x4001c000 + 4 + 4 * SLAVE_Y_PIN, (1 << 3) | (1 << 6));

	dap_poke(0x4001c000 + 4 + 4 * SLAVE_SELECT_PIN, (1 << 3) | (1 << 6));

	/* Make sure we do not turn outselves off. */
	dap_poke(0x40018004, 0x001f);

	multicore_launch_core1(task_run_loop);
	task_run_loop();
}
