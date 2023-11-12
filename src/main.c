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

#include <stdlib.h>
#include <stdio.h>
#include <task.h>
#include <tft.h>

#define JOY_BTN_PIN 22
#define JOY_X_PIN 27
#define JOY_Y_PIN 28

#define RED 240
#define GREEN 244
#define BLUE 250
#define GRAY 8

static int input_joy_x = 0;
static int input_joy_y = 0;
static int input_joy_btn = 0;

static void stats_task(void);
static void tft_task(void);
static void input_task(void);

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

/*
 * Processes joystick and button inputs.
 */
static void input_task(void)
{
	gpio_init(JOY_BTN_PIN);
	gpio_set_dir(JOY_BTN_PIN, GPIO_IN);
	gpio_pull_up(JOY_BTN_PIN);

	while (true) {
		int joy_btn = !gpio_get(JOY_BTN_PIN);

		adc_select_input(JOY_X_PIN - 26);

		int joy_x = 0;
		int joy_y = 0;

		for (int i = 0; i < 256; i++)
			joy_x += -(adc_read() - 2048);

		adc_select_input(JOY_Y_PIN - 26);

		for (int i = 0; i < 256; i++)
			joy_y += -(adc_read() - 2048);

		srand(joy_x + joy_y + time_us_32());

		joy_x /= 256;
		joy_y /= 256;

		input_joy_x = joy_x;
		input_joy_y = joy_y;
		input_joy_btn = joy_btn;

		//printf("joy: btn=%i, x=%5i, y=%5i\n", joy_btn, joy_x, joy_y);
		task_sleep_ms(50);
	}
}

inline static int clamp(int x, int lo, int hi)
{
	if (x < lo)
		return lo;

	if (x > hi)
		return hi;

	return x;
}

/*
 * Outputs stuff to the screen as fast as possible.
 */
static void tft_task(void)
{
	uint32_t last_sync = time_us_32();
	int fps = 0;

	int ew = 30;
	int eh = 30;
	int ex = random() % (tft_width - ew);
	int ey = random() % (tft_height - eh);
	int ehp_max = 59;
	int ehp = ehp_max;
	int exa = 2;
	int eya = 2;
	int score = 0;

	while (true) {
		tft_fill(0);
		char buf[64];

		snprintf(buf, sizeof buf, "%i", score);
		tft_draw_string(0, 0, RED, buf);

		/* Calculate crosshair position */
		int tx = tft_width / 2 * input_joy_x / 2047;
		int ty = tft_height / 2 * -input_joy_y / 2047;
		tx += tft_width / 2;
		ty += tft_height / 2;

		/* Check for collission */
		if ((tx >= ex) && (tx <= (ex + ew)) && (ty >= ey) && (ty <= (ey + eh))) {
			ehp = MAX(0, ehp - 1);
		} else {
			ehp = MIN(ehp_max, ehp + 1);
		}

		if (!ehp) {
			ex = random() % (tft_width - ew);
			ey = random() % (tft_height - eh);
			ehp = ehp_max;
			ew = clamp(ew - 2, 10, 30);
			eh = clamp(eh - 2, 10, 30);
			score++;
		} else {
			ex += exa * (random() % 3 - 1);
			ey += eya * (random() % 3 - 1);

			ex = clamp(ex, 0, tft_width - ew);
			ey = clamp(ey, 0, tft_height - eh);
		}

		int ecolor = 249 + 6 - ehp / 10;

		/* draw THE enemy */
		tft_draw_rect(ex, ey, ex + ew, ey + eh, ecolor);

		/* Draw the crosshair */
		tft_draw_rect(tx - 2, ty, tx + 2, ty, GREEN);
		tft_draw_rect(tx, ty - 2, tx, ty + 2, GREEN);

		snprintf(buf, sizeof buf, "%i", fps);
		tft_draw_string_right(tft_width - 1, 0, GRAY, buf);

		tft_swap_buffers();
		task_sleep_ms(9);
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
	adc_gpio_init(JOY_X_PIN);
	adc_gpio_init(JOY_Y_PIN);
	adc_select_input(JOY_X_PIN);

	for (int i = 0; i < 16; i++)
		srand(adc_read() + random());

	gpio_init(6);
	gpio_set_dir(6, GPIO_OUT);
	gpio_put(6, 1);
	tft_init();

	printf("Hello, have a nice and productive day!\n");

	multicore_launch_core1(task_run_loop);
	task_run_loop();
}
