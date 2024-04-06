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

#include <sdk.h>

#include <dap.h>
#include <task.h>
#include <tft.h>

#include <pico/multicore.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/clocks.h>

#include <hardware/regs/adc.h>
#include <hardware/regs/clocks.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>

#include <stdio.h>
#include <stdlib.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

struct sdk_config sdk_config = {};
struct sdk_inputs sdk_inputs = {};

static void stats_task(void);
static void tft_task(void);
static void input_task(void);

task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		MAKE_TASK(4, "stats", stats_task),
		MAKE_TASK(1, "input", input_task),
		NULL,
	},
	{
		MAKE_TASK(1, "tft", tft_task),
		NULL,
	},
};

static uint32_t unreset = RESETS_RESET_SYSINFO_BITS | RESETS_RESET_SYSCFG_BITS |
			  RESETS_RESET_PWM_BITS | RESETS_RESET_PLL_SYS_BITS |
			  RESETS_RESET_PADS_QSPI_BITS | RESETS_RESET_PADS_BANK0_BITS |
			  RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_IO_BANK0_BITS;

static void slave_park()
{
	dap_init(DAP_SWDIO_PIN, DAP_SWCLK_PIN);
	dap_reset();

	dap_select_target(DAP_RESCUE);
	unsigned idcode = dap_read_idcode();
#define CDBGPWRUPREQ (1 << 28)
	dap_set_reg(DAP_DP4, CDBGPWRUPREQ);
	dap_set_reg(DAP_DP4, 0);

	dap_reset();
	dap_select_target(DAP_CORE0);
	idcode = dap_read_idcode();
	//printf("core0 idcode = 0x%08x\n", idcode);
	dap_setup_mem((uint32_t *)&idcode);
	//printf("idr = %#010x\n", idcode);
	dap_noop();

	/* Start XOSC */
	clock_gpio_init(CLK_OUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1.0);
	dap_poke(XOSC_BASE + XOSC_STARTUP_OFFSET, 0);
	dap_poke(XOSC_BASE + XOSC_CTRL_OFFSET, 0xfabaa0);

	/* Un-reset subsystems we wish to use */
	dap_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	/* Prevent power-off */
	dap_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_SELECT_PIN,
		 (1 << 3) | (1 << 6));
	dap_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET + 8 * SLAVE_OFF_QSPI_PIN,
		 IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS);
}

static void slave_init()
{
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
}

void sdk_main(struct sdk_config *conf)
{
	sdk_config = *conf;

	slave_park();
	stdio_usb_init();
	task_init();

	if (sdk_config.wait_for_usb) {
		for (int i = 0; i < 30; i++) {
			if (stdio_usb_connected())
				break;

			sleep_ms(100);
		}
	}

	adc_init();

	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);
	adc_gpio_init(29);

	for (int i = 0; i < 16; i++)
		srand(adc_read() + random());

	printf("Hello, welcome to Krecek!\n");

	tft_init();
	slave_init();

	multicore_launch_core1(task_run_loop);
	task_run_loop();
}

__weak void game_reset(void)
{
}

__weak void game_input(void)
{
}

__weak void game_paint(unsigned __unused dt)
{
}

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

inline static __unused int slave_adc_read(int gpio)
{
	if (!dap_poke(ADC_BASE + ADC_CS_OFFSET,
		      ((gpio - 26) << ADC_CS_AINSEL_LSB) | ADC_CS_EN_BITS | ADC_CS_START_ONCE_BITS))
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

static void input_task(void)
{
	task_sleep_ms(250);

	while (true) {
		sdk_inputs.a = !slave_gpio_get(SLAVE_A_PIN);
		sdk_inputs.b = !slave_gpio_get(SLAVE_B_PIN);
		sdk_inputs.x = !slave_gpio_get(SLAVE_X_PIN);
		sdk_inputs.y = !slave_gpio_get(SLAVE_Y_PIN);

		sdk_inputs.start = slave_gpio_get(SLAVE_START_PIN);
		sdk_inputs.select = !slave_gpio_get(SLAVE_SELECT_PIN);

		if (sdk_config.off_on_select) {
			if (sdk_inputs.select) {
				puts("SELECT pressed, turning off...");
				dap_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET +
						 8 * SLAVE_OFF_QSPI_PIN,
					 IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER_BITS |
						 IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER_BITS |
						 IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS);
			}
		}

		sdk_inputs.joy_x = 2048 - slave_adc_read(SLAVE_JOY_X_PIN) - 197;
		sdk_inputs.joy_y = 2048 - slave_adc_read(SLAVE_JOY_Y_PIN) - 172;

		// TODO: aux[0-7] + joy_sw + brackets?

		adc_select_input(2);
		int bat = 0;

		for (int i = 0; i < 32; i++)
			bat += adc_read();

		float mv = (float)bat * 1.6f / 32.0f;

		if (!sdk_inputs.batt_mv)
			sdk_inputs.batt_mv = mv;
		else
			sdk_inputs.batt_mv = sdk_inputs.batt_mv * 0.9 + 0.1 * mv;

		game_input();

		task_sleep_ms(10);
	}
}

static void tft_task(void)
{
	uint32_t last_sync = time_us_32();
	uint32_t delta = 1000 * 1000 / 30;
	int fps = 30;

	game_reset();

	while (true) {
		if (sdk_config.show_fps) {
			game_paint(delta ? delta : 1);

			static char buf[64];
			snprintf(buf, sizeof buf, "%i", fps);
			tft_draw_string_right(tft_width - 1, 0, 8, buf);

			tft_swap_buffers();
			tft_sync();

			uint32_t this_sync = time_us_32();
			delta = this_sync - last_sync;
			fps = 1 * 1000 * 1000 / delta;
			last_sync = this_sync;
		} else {
			game_paint(delta ? delta : 1);

			tft_swap_buffers();
			tft_sync();

			uint32_t this_sync = time_us_32();
			delta = this_sync - last_sync;
			last_sync = this_sync;
		}

		task_sleep_ms(1);
	}
}
