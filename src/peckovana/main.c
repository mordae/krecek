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
#include <hardware/clocks.h>

#include <hardware/regs/adc.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>

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

static float bat_mv = 0.0;

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

	if (!dap_peek(IO_BANK0_BASE + IO_BANK0_GPIO0_STATUS_OFFSET + 8 * pin, &tmp))
		return 0;

	return (tmp >> IO_BANK0_GPIO0_STATUS_INFROMPAD_LSB) & 1;
}

inline static __unused int slave_adc_read(int ain)
{
	if (!dap_poke(ADC_BASE + ADC_CS_OFFSET,
		      (ain << ADC_CS_AINSEL_LSB) | ADC_CS_EN_BITS | ADC_CS_START_ONCE_BITS))
		return 0;

	for (int i = 0; i < 10; i++) {
		uint32_t status = 0;

		if (!dap_peek(ADC_BASE + ADC_CS_OFFSET, &status))
			return 0;

		if (status & ADC_CS_READY_BITS)
			break;
	}

	uint32_t result = 0;

	if (!dap_peek(ADC_BASE + ADC_RESULT_OFFSET, &result))
		return 0;

	return result;
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

#if 0
		int joy_x = 4095 - slave_adc_read(2);
		int joy_y = 4095 - slave_adc_read(3);

		printf("joy: %4i  %4i\n", joy_x, joy_y);
#endif

		adc_select_input(2);
		int bat = 0;

		for (int i = 0; i < 32; i++)
			bat += adc_read();

		float mv = (float)bat * 1.6f / 32.0f;

		if (!bat_mv)
			bat_mv = mv;
		else
			bat_mv = bat_mv * 0.9 + 0.1 * mv;

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

		snprintf(buf, sizeof buf, "%.1f mV", bat_mv);
		tft_draw_string_right(tft_width - 1, 16, GRAY, buf);

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

	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);
	adc_gpio_init(29);

	for (int i = 0; i < 16; i++)
		srand(adc_read() + random());

	tft_init();

	printf("Hello, have a nice and productive day!\n");

	dap_init(DAP_SWDIO_PIN, DAP_SWCLK_PIN);
	dap_reset();

	dap_select_target(DAP_RESCUE);
	unsigned idcode = dap_read_idcode();
	printf("rescue idcode = 0x%08x\n", idcode);
#define CDBGPWRUPREQ (1 << 28)
	dap_set_reg(DAP_DP4, CDBGPWRUPREQ);
	dap_set_reg(DAP_DP4, 0);

	dap_reset();
	dap_select_target(DAP_CORE0);
	idcode = dap_read_idcode();
	printf("core0 idcode = 0x%08x\n", idcode);
	dap_setup_mem((uint32_t *)&idcode);
	printf("idr = %#010x\n", idcode);
	dap_noop();

	/* Start XOSC */
	clock_gpio_init(CLK_OUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1.0);
	dap_poke(XOSC_BASE + XOSC_STARTUP_OFFSET, 0);
	dap_poke(XOSC_BASE + XOSC_CTRL_OFFSET, 0xfabaa0);

	uint32_t unreset = RESETS_RESET_SYSINFO_BITS | RESETS_RESET_SYSCFG_BITS |
			   RESETS_RESET_PWM_BITS | RESETS_RESET_PLL_SYS_BITS |
			   RESETS_RESET_PADS_QSPI_BITS | RESETS_RESET_PADS_BANK0_BITS |
			   RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_IO_BANK0_BITS;

	/* Un-reset subsystems we wish to use */
	dap_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	uint32_t status = 0;

	while (true) {
		dap_peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET, &status);
		if ((status & unreset) == unreset)
			break;
	}

	puts("#2 un-reset");

	dap_poke(PLL_SYS_BASE + PLL_FBDIV_INT_OFFSET, 125);
	dap_poke(PLL_SYS_BASE + PLL_PRIM_OFFSET, 0x62000);

	dap_peek(PLL_SYS_BASE + PLL_PWR_OFFSET, &status);
	dap_poke(PLL_SYS_BASE + PLL_PWR_OFFSET, status & ~(PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS));

	while (true) {
		dap_peek(PLL_SYS_BASE + PLL_CS_OFFSET, &status);
		if (status & PLL_CS_LOCK_BITS)
			break;
	}

	dap_poke(PLL_SYS_BASE + PLL_PWR_OFFSET, 0);

	puts("#2 PLL_SYS locked");

	dap_poke(CLOCKS_BASE + CLOCKS_CLK_SYS_CTRL_OFFSET,
		 CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX);

	dap_poke(CLOCKS_BASE + CLOCKS_CLK_PERI_CTRL_OFFSET, CLOCKS_CLK_PERI_CTRL_ENABLE_BITS);

	dap_poke(CLOCKS_BASE + CLOCKS_CLK_REF_CTRL_OFFSET,
		 CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC << CLOCKS_CLK_REF_CTRL_SRC_LSB);

	dap_poke(CLOCKS_BASE + CLOCKS_CLK_ADC_DIV_OFFSET, 5 << CLOCKS_CLK_ADC_DIV_INT_LSB);
	dap_poke(CLOCKS_BASE + CLOCKS_CLK_ADC_CTRL_OFFSET,
		 CLOCKS_CLK_ADC_CTRL_ENABLE_BITS | (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS
						    << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB));

	unreset |= RESETS_RESET_ADC_BITS;
	dap_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	while (true) {
		dap_peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET, &status);
		if ((status & unreset) == unreset)
			break;
	}

	puts("#2 ADC un-reset");

	dap_peek(CLOCKS_BASE + CLOCKS_CLK_ADC_SELECTED_OFFSET, &status);

	dap_poke(ADC_BASE + ADC_CS_OFFSET, ADC_CS_EN_BITS);

	/* Enable display backlight. */
	dap_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * SLAVE_TFT_LED_PIN,
		 IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS | IO_BANK0_GPIO0_CTRL_OEOVER_BITS |
			 IO_BANK0_GPIO0_CTRL_OUTOVER_BITS);

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
