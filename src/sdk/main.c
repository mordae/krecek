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
#include <es8312.h>
#include <i2s.pio.h>

#include <pico/multicore.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>

#include <hardware/regs/adc.h>
#include <hardware/regs/clocks.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/pads_qspi.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/pwm.h>
#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

struct sdk_config sdk_config = {};
struct sdk_inputs sdk_inputs = {};
struct sdk_inputs sdk_inputs_delta = {};

static struct sdk_inputs prev_inputs = {};

static int sm_i2s = -1;
static int dma_i2s_rx = -1;
static int dma_i2s_tx = -1;

static int num_wakeups = 0;
static int min_samples = INT_MAX;
static int max_samples = 0;
static int all_samples = 0;

#define I2S_BUF_BITS 11
#define I2S_BUF_LEN (1 << (I2S_BUF_BITS - 1))

static int16_t i2s_rx_buf[I2S_BUF_LEN] __attribute__((__aligned__(I2S_BUF_LEN * 2)));
static int16_t i2s_tx_buf[I2S_BUF_LEN] __attribute__((__aligned__(I2S_BUF_LEN * 2)));

static semaphore_t paint_sema;
static semaphore_t sync_sema;

static struct es8312_driver dsp = {
	.i2c = DSP_I2C,
	.addr = ES8312_ADDR,
};

static void stats_task(void);
static void tft_task(void);
static void audio_task(void);
static void input_task(void);
static void paint_task(void);

static void set_amp_enabled(bool en);

static void __attribute__((__noreturn__)) __attribute__((__format__(printf, 1, 2)))
hang(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("sdk: giving up on core %u\n", get_core_num());

	while (true)
		sleep_ms(1000);
}

static uint32_t peek(uint32_t addr)
{
	uint32_t out;

	if (!dap_peek(addr, &out))
		hang("sdk: dap_peek(0x%08x) failed\n", (unsigned)addr);

	return out;
}

static void poke(uint32_t addr, uint32_t value)
{
	if (!dap_poke(addr, value))
		hang("sdk: dap_poke(0x%08x, 0x%08x) failed\n", (unsigned)addr, (unsigned)value);
}

task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		MAKE_TASK(4, "stats", stats_task),
		MAKE_TASK(2, "input", input_task),
		MAKE_TASK(1, "paint", paint_task),
	},
	{
		MAKE_TASK(4, "tft", tft_task),
		MAKE_TASK(1, "audio", audio_task),
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

	/* Output inverse of the crystal clock */
	gpio_set_outover(CLK_OUT_PIN, GPIO_OVERRIDE_INVERT);
	clock_gpio_init_int_frac(CLK_OUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1,
				 0);

	/* Start XOSC */
	poke(XOSC_BASE + XOSC_STARTUP_OFFSET, 0);
	poke(XOSC_BASE + XOSC_CTRL_OFFSET, 0xfabaa0);

	/* Un-reset subsystems we wish to use */
	poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	/* Prevent power-off */
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_SELECT_PIN, (1 << 3) | (1 << 6));
	poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET + 8 * SLAVE_OFF_QSPI_PIN,
	     IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS);
}

static void slave_init()
{
	uint32_t status = 0;

	while (true) {
		status = peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
		if ((status & unreset) == unreset)
			break;
	}

	puts("sdk: slave un-reset");

	poke(PLL_SYS_BASE + PLL_FBDIV_INT_OFFSET, 132);
	poke(PLL_SYS_BASE + PLL_PRIM_OFFSET, 0x62000);

	status = peek(PLL_SYS_BASE + PLL_PWR_OFFSET);
	poke(PLL_SYS_BASE + PLL_PWR_OFFSET, status & ~(PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS));

	while (true) {
		status = peek(PLL_SYS_BASE + PLL_CS_OFFSET);
		if (status & PLL_CS_LOCK_BITS)
			break;
	}

	poke(PLL_SYS_BASE + PLL_PWR_OFFSET, 0);

	puts("sdk: slave PLL_SYS locked");

	poke(CLOCKS_BASE + CLOCKS_CLK_SYS_CTRL_OFFSET,
	     CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX);

	poke(CLOCKS_BASE + CLOCKS_CLK_PERI_CTRL_OFFSET, CLOCKS_CLK_PERI_CTRL_ENABLE_BITS);

	poke(CLOCKS_BASE + CLOCKS_CLK_REF_CTRL_OFFSET,
	     CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC << CLOCKS_CLK_REF_CTRL_SRC_LSB);

	poke(CLOCKS_BASE + CLOCKS_CLK_ADC_DIV_OFFSET, 3 << CLOCKS_CLK_ADC_DIV_INT_LSB);
	poke(CLOCKS_BASE + CLOCKS_CLK_ADC_CTRL_OFFSET,
	     CLOCKS_CLK_ADC_CTRL_ENABLE_BITS | (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS
						<< CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB));

	unreset |= RESETS_RESET_ADC_BITS;
	poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	while (true) {
		status = peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
		if ((status & unreset) == unreset)
			break;
	}

	puts("sdk: slave ADC un-reset");

	/* We can now increase the access speed. */
	dap_set_delay_cycles(0);

	poke(ADC_BASE + ADC_CS_OFFSET, ADC_CS_EN_BITS);

	/* Enable display backlight. */
	static_assert(SLAVE_TFT_LED_PIN == 13, "Code assumes that SLAVE_TFT_LED_PIN == 13");
	poke(IO_BANK0_BASE + IO_BANK0_GPIO13_CTRL_OFFSET,
	     IO_BANK0_GPIO13_CTRL_FUNCSEL_VALUE_PWM_B_6 << IO_BANK0_GPIO13_CTRL_FUNCSEL_LSB);
	poke(PWM_BASE + PWM_CH6_CC_OFFSET, (uint32_t)sdk_config.brightness << PWM_CH6_CC_B_LSB);
	poke(PWM_BASE + PWM_CH6_TOP_OFFSET, 256);
	poke(PWM_BASE + PWM_EN_OFFSET, PWM_EN_CH6_BITS);

	/* Enable button input + pull-ups. */
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_A_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_B_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_X_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_Y_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);

	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_UP_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_DOWN_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_SW_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);

	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_SELECT_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);

	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX0_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX1_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX2_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX3_PIN,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);

	poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX4_QSPI_PIN,
	     PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
		     PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX5_QSPI_PIN,
	     PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
		     PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX6_QSPI_PIN,
	     PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
		     PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX7_QSPI_PIN,
	     PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
		     PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);

	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * 26,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * 27,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * 28,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * 29,
	     PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);

	set_amp_enabled(false);

	puts("sdk: slave configuration complete");
}

void sdk_set_screen_brightness(uint8_t level)
{
	sdk_config.brightness = level;
	poke(PWM_BASE + PWM_CH6_CC_OFFSET, (uint32_t)level << PWM_CH6_CC_B_LSB);
}

static void set_amp_enabled(bool en)
{
	poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * SLAVE_AMP_EN_PIN,
	     (IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER_BITS |
	      (en ? IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER_BITS : 0) |
	      IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS));
}

static void dsp_init(void)
{
	unsigned rate = i2c_init(DSP_I2C, 100000);

	printf("sdk: i2c rate=%u\n", rate);

	gpio_set_function(DSP_CDATA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(DSP_CCLK_PIN, GPIO_FUNC_I2C);

	gpio_set_pulls(DSP_CDATA_PIN, true, false);
	gpio_set_pulls(DSP_CCLK_PIN, true, false);

	int id = es8312_identify(&dsp);

	printf("sdk: DSP identified as ES%x r%x\n", id >> 8, id & 0xff);

	es8312_reset(&dsp);

	sm_i2s = pio_claim_unused_sm(SDK_PIO, true);

	struct pio_i2s_config i2s_cfg = {
		.origin = -1,
		.pio = SDK_PIO,
		.sm = sm_i2s,
		.in_pin = DSP_DOUT_PIN,
		.out_pin = DSP_DIN_PIN,
		.clock_pin_base = DSP_LRCK_PIN,
		.sample_rate = 48000,
	};

	pio_i2s_init(&i2s_cfg);
	pio_sm_set_enabled(SDK_PIO, sm_i2s, true);

	dma_i2s_rx = dma_claim_unused_channel(true);
	dma_i2s_tx = dma_claim_unused_channel(true);

	dma_channel_config dma_cfg;

	dma_cfg = dma_channel_get_default_config(dma_i2s_rx);
	channel_config_set_dreq(&dma_cfg, pio_get_dreq(SDK_PIO, sm_i2s, false));
	channel_config_set_read_increment(&dma_cfg, false);
	channel_config_set_write_increment(&dma_cfg, true);
	channel_config_set_ring(&dma_cfg, true, I2S_BUF_BITS);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
	dma_channel_configure(dma_i2s_rx, &dma_cfg, i2s_rx_buf, &SDK_PIO->rxf[sm_i2s], UINT_MAX,
			      false);

	dma_cfg = dma_channel_get_default_config(dma_i2s_tx);
	channel_config_set_dreq(&dma_cfg, pio_get_dreq(SDK_PIO, sm_i2s, true));
	channel_config_set_read_increment(&dma_cfg, true);
	channel_config_set_write_increment(&dma_cfg, false);
	channel_config_set_ring(&dma_cfg, false, I2S_BUF_BITS);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
	dma_channel_configure(dma_i2s_tx, &dma_cfg, &SDK_PIO->txf[sm_i2s], i2s_tx_buf, UINT_MAX,
			      false);

	puts("sdk: configured i2s");

	es8312_start(&dsp);
	es8312_set_output(&dsp, false, false);
	es8312_set_output_gain(&dsp, 0);

	puts("sdk: configured audio DSP");
}

void sdk_set_output_gain_db(float gain)
{
	int gain_raw = 191 + gain * 2;

	if (gain_raw < 0)
		gain_raw = 0;

	if (gain_raw > 255)
		gain_raw = 255;

	if (0 > es8312_set_output_gain(&dsp, gain_raw))
		puts("sdk: failed to set output gain");

	set_amp_enabled(!!gain_raw);
}

void sdk_main(struct sdk_config *conf)
{
	set_sys_clock_khz(CLK_SYS_HZ / KHZ, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, CLK_SYS_HZ,
			CLK_SYS_HZ);

	sdk_config = *conf;

	if (!sdk_config.brightness)
		sdk_config.brightness = 64;

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

	printf("sdk: Hello, welcome to Krecek!\n");

	tft_init();
	slave_init();
	dsp_init();

	sem_init(&paint_sema, 1, 1);
	sem_init(&sync_sema, 0, 1);

	game_start();

	dma_channel_start(dma_i2s_rx);
	dma_channel_start(dma_i2s_tx);

	multicore_launch_core1(task_run_loop);
	task_run_loop();
}

__weak void game_start(void)
{
}

__weak void game_reset(void)
{
}

__weak void game_audio(int nsamples)
{
	(void)nsamples;
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

		printf("sdk: audio samples: min=%i, avg=%i, max=%i / buf=%i\n", min_samples,
		       all_samples / num_wakeups, max_samples, I2S_BUF_LEN);

		num_wakeups = 0;
		min_samples = INT_MAX;
		max_samples = 0;
		all_samples = 0;
	}
}

inline static int slave_gpio_get(int pin)
{
	uint32_t tmp = peek(IO_BANK0_BASE + IO_BANK0_GPIO0_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_BANK0_GPIO0_STATUS_INFROMPAD_LSB) & 1;
}

inline static int slave_gpio_qspi_get(int pin)
{
	uint32_t tmp = peek(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_QSPI_GPIO_QSPI_SCLK_STATUS_INFROMPAD_LSB) & 1;
}

inline static __unused int slave_adc_read(int gpio)
{
	poke(ADC_BASE + ADC_CS_OFFSET,
	     ((gpio - 26) << ADC_CS_AINSEL_LSB) | ADC_CS_EN_BITS | ADC_CS_START_MANY_BITS);

	uint32_t sample = peek(ADC_BASE + ADC_RESULT_OFFSET);

	return sample;
}

static unsigned rx_offset = 0;
static unsigned tx_offset = 0;
static unsigned xx_limit = 0;

static void audio_task(void)
{
	while (true) {
		xx_limit = ~(unsigned)dma_hw->ch[dma_i2s_rx].transfer_count;
		int delta = xx_limit - rx_offset;

		if (delta < min_samples)
			min_samples = delta;

		if (delta > max_samples)
			max_samples = delta;

		all_samples += delta;
		num_wakeups++;

		if (delta > I2S_BUF_LEN) {
			puts("sdk: audio buffer underflow, try yielding more");
			xx_limit = rx_offset + I2S_BUF_LEN;
			game_audio(I2S_BUF_LEN);
		} else {
			game_audio(delta);
		}

		rx_offset = xx_limit;
		tx_offset = xx_limit;
		task_sleep_us(900);
	}
}

bool sdk_write_sample(int16_t sample)
{
	if (tx_offset >= xx_limit)
		return false;

	i2s_tx_buf[tx_offset++ & (I2S_BUF_LEN - 1)] = sample;
	return true;
}

bool sdk_read_sample(int16_t *sample)
{
	if (rx_offset >= xx_limit)
		return false;

	*sample = i2s_rx_buf[rx_offset++ & (I2S_BUF_LEN - 1)];
	return true;
}

int sdk_write_samples(const int16_t *buf, int len)
{
	for (int i = 0; i < len; i++)
		if (!sdk_write_sample(buf[i]))
			return i;

	return len;
}

int sdk_read_samples(int16_t *buf, int len)
{
	for (int i = 0; i < len; i++)
		if (!sdk_read_sample(buf + i))
			return i;

	return len;
}

static void input_task(void)
{
	task_sleep_ms(250);

	while (true) {
		sdk_inputs.a = !slave_gpio_get(SLAVE_A_PIN);
		sdk_inputs.b = !slave_gpio_get(SLAVE_B_PIN);
		sdk_inputs.x = !slave_gpio_get(SLAVE_X_PIN);
		sdk_inputs.y = !slave_gpio_get(SLAVE_Y_PIN);

		sdk_inputs.vol_up = !slave_gpio_get(SLAVE_VOL_UP_PIN);
		sdk_inputs.vol_down = !slave_gpio_get(SLAVE_VOL_DOWN_PIN);
		sdk_inputs.vol_sw = !slave_gpio_get(SLAVE_VOL_SW_PIN);

		sdk_inputs.aux[0] = !slave_gpio_get(SLAVE_AUX0_PIN);
		sdk_inputs.aux[1] = !slave_gpio_get(SLAVE_AUX1_PIN);
		sdk_inputs.aux[2] = !slave_gpio_get(SLAVE_AUX2_PIN);
		sdk_inputs.aux[3] = !slave_gpio_get(SLAVE_AUX3_PIN);

		sdk_inputs.aux[4] = !slave_gpio_qspi_get(SLAVE_AUX4_QSPI_PIN);
		sdk_inputs.aux[5] = !slave_gpio_qspi_get(SLAVE_AUX5_QSPI_PIN);
		sdk_inputs.aux[6] = !slave_gpio_qspi_get(SLAVE_AUX6_QSPI_PIN);
		sdk_inputs.aux[7] = !slave_gpio_qspi_get(SLAVE_AUX7_QSPI_PIN);

		sdk_inputs.start = slave_gpio_get(SLAVE_START_PIN);
		sdk_inputs.select = !slave_gpio_get(SLAVE_SELECT_PIN);

		sdk_inputs.joy_x = 2048 - slave_adc_read(SLAVE_JOY_X_PIN);
		sdk_inputs.joy_y = 2048 - slave_adc_read(SLAVE_JOY_Y_PIN);

		// TODO: joy_sw + brackets?

		adc_select_input(2);
		int bat = 0;

		for (int i = 0; i < 32; i++)
			bat += adc_read();

		float mv = (float)bat * 1.6f / 32.0f;

		if (!sdk_inputs.batt_mv)
			sdk_inputs.batt_mv = mv;
		else
			sdk_inputs.batt_mv = sdk_inputs.batt_mv * 0.9 + 0.1 * mv;

		/* Calculate input deltas */
		sdk_inputs_delta.a = sdk_inputs.a - prev_inputs.a;
		sdk_inputs_delta.b = sdk_inputs.b - prev_inputs.b;
		sdk_inputs_delta.x = sdk_inputs.x - prev_inputs.x;
		sdk_inputs_delta.y = sdk_inputs.y - prev_inputs.y;

		sdk_inputs_delta.vol_up = sdk_inputs.vol_up - prev_inputs.vol_up;
		sdk_inputs_delta.vol_down = sdk_inputs.vol_down - prev_inputs.vol_down;
		sdk_inputs_delta.vol_sw = sdk_inputs.vol_sw - prev_inputs.vol_sw;

		for (int i = 0; i < 8; i++)
			sdk_inputs_delta.aux[i] = sdk_inputs.aux[i] - prev_inputs.aux[i];

		sdk_inputs_delta.start = sdk_inputs.start - prev_inputs.start;
		sdk_inputs_delta.select = sdk_inputs.select - prev_inputs.select;

		sdk_inputs_delta.batt_mv = sdk_inputs.batt_mv - prev_inputs.batt_mv;

		prev_inputs = sdk_inputs;

		if (sdk_config.off_on_select) {
			if (sdk_inputs_delta.select > 0) {
				puts("sdk: SELECT pressed, turning off...");
				poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET +
					     8 * SLAVE_OFF_QSPI_PIN,
				     (IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER_BITS |
				      IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER_BITS |
				      IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS));
			}
		}

		/* Let the game process inputs as soon as possible. */
		game_input();

		task_sleep_ms(15);
	}
}

static void paint_task(void)
{
	uint32_t last_sync = time_us_32();
	int delta = 1000 * 1000 / 30;
	int budget = 0;

	uint32_t last_updated = 0;
	float active_fps = 30;
	float fps = active_fps;

	game_reset();

	while (true) {
		sem_acquire_blocking(&paint_sema);

		if (sdk_config.target_fps > 0)
			budget += 1000000.0f / sdk_config.target_fps;

		game_paint(delta ? delta : 1);

		if (sdk_config.show_fps) {
			static char buf[64];
			snprintf(buf, sizeof buf, "%.0f", floorf(active_fps));
			tft_draw_string_right(tft_width - 1, 0, 8, buf);
		}

		sem_release(&sync_sema);

		uint32_t this_sync = time_us_32();
		delta = this_sync - last_sync;
		fps = 0.95 * fps + 0.05 * (1000000.0f / delta);
		last_sync = this_sync;

		if ((this_sync - last_updated) >= 1000000) {
			last_updated = this_sync;
			active_fps = fps;
		}

		if (sdk_config.target_fps > 0) {
			budget -= delta;

			if (budget > 0) {
				task_sleep_us(budget / 2);
				continue;
			} else if (budget < -1000000) {
				puts("sdk: cannot reach target fps, giving up");
				budget = 0;
			}
		}

		task_yield();
	}
}

static void tft_task(void)
{
	while (true) {
		sem_acquire_blocking(&sync_sema);
		tft_swap_buffers();
		sem_release(&paint_sema);

		tft_sync();
		task_yield();
	}
}

void tft_dma_channel_wait_for_finish_blocking(int dma_ch)
{
	task_wait_for_dma(dma_ch);
}
