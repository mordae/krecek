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
#include <pico/stdio_usb.h>

#include <stdio.h>

int main()
{
	stdio_usb_init();

	for (int i = 0; i < 30; i++) {
		if (stdio_usb_connected())
			break;

		sleep_ms(100);
	}

	printf("Hello, have a nice and productive day!\n");

	gpio_init(16);
	gpio_init(17);

	gpio_pull_up(16);
	gpio_pull_up(17);

#if 1
	while (true) {
		printf("btn: %i %i\n", gpio_get(16), gpio_get(17));
		sleep_ms(33);
	}
#else
	gpio_set_drive_strength(16, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_dir(16, GPIO_IN);
	gpio_put(16, 1);

	unsigned avg = 1000000;

	while (true) {
		unsigned total = 0;

		for (int i = 0; i < 1024; i++) {
			int x = gpio_get(16);
			gpio_set_dir(16, !x);
			total += x;
		}

		gpio_set_dir(16, GPIO_IN);

		avg = (avg + total * 1024) / 2;

		printf("Button Pressure: %5u\n", (avg - 768 * 1024) / 1024);

		sleep_ms(33);
	}
#endif
}
