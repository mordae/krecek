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
#include <pico/multicore.h>
#include <pico/stdio_usb.h>

#include <tft.h>
#include <stdio.h>
#include <task.h>

#define RED 240
#define GREEN 244
#define BLUE 250
#define GRAY 4

static void stats_task(void);
static void tft_task(void);

/*
 * Tasks to run concurrently:
 */
task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		/* On the first core: */
		MAKE_TASK(4, "stats", stats_task),
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
 * Outputs stuff to the screen as fast as possible.
 */
static void tft_task(void)
{
	uint32_t last_sync = time_us_32();
	int fps = 0;

	int len = tft_width > tft_height ? tft_height : tft_width;

	while (true) {
		for (int i = -16; i < len; i++) {
			tft_fill(0);

			char buf[32];
			snprintf(buf, sizeof buf, "%i", fps);
			tft_draw_string_right(tft_width - 1, 0, GRAY, buf);

			for (int j = 0; j < len; j++) {
				tft_draw_pixel(j, j, GREEN);
				tft_draw_pixel(len - 1 - j, j, BLUE);
			}

			tft_draw_string(i + 32, i, RED, "Hello!");

			tft_swap_buffers();
			task_sleep_ms(9);
			tft_sync();

			uint32_t this_sync = time_us_32();
			uint32_t delta = this_sync - last_sync;
			fps = 1 * 1000 * 1000 / delta;
			last_sync = this_sync;
		}
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

	gpio_init(6);
	gpio_set_dir(6, GPIO_OUT);
	gpio_put(6, 1);
	tft_init();

	printf("Hello, have a nice and productive day!\n");

	multicore_launch_core1(task_run_loop);
	task_run_loop();
}
