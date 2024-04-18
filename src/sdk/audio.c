#include <pico/stdlib.h>

#include <hardware/dma.h>

#include <hardware/regs/io_qspi.h>

#include <limits.h>
#include <stdio.h>

#include <task.h>

#include <sdk.h>
#include <sdk/slave.h>

#include "es8312.h"
#include "i2s.pio.h"

static struct es8312_driver dsp = {
	.i2c = DSP_I2C,
	.addr = ES8312_ADDR,
};

static int sm_i2s = -1;
static int dma_i2s_rx = -1;
static int dma_i2s_tx = -1;

static unsigned rx_offset = 0;
static unsigned tx_offset = 0;
static unsigned xx_limit = 0;

static int num_wakeups = 0;
static int min_samples = INT_MAX;
static int max_samples = 0;
static int all_samples = 0;

#define I2S_BUF_BITS 11
#define I2S_BUF_LEN (1 << (I2S_BUF_BITS - 1))

static int16_t __scratch_x("i2s_rx_buf") i2s_rx_buf[I2S_BUF_LEN]
	__attribute__((__aligned__(I2S_BUF_LEN * 2)));
static int16_t __scratch_y("i2s_tx_buf") i2s_tx_buf[I2S_BUF_LEN]
	__attribute__((__aligned__(I2S_BUF_LEN * 2)));

void sdk_audio_task(void)
{
	unsigned sleep = 900;

	while (true) {
		if (!dma_channel_is_busy(dma_i2s_tx)) {
			dma_channel_abort(dma_i2s_rx); // Just in case
			dma_hw->multi_channel_trigger = (1u << dma_i2s_tx) | (1u << dma_i2s_rx);
		}

		xx_limit = ~(unsigned)dma_hw->ch[dma_i2s_rx].transfer_count;
		int delta = xx_limit - rx_offset;

		if (delta < min_samples)
			min_samples = delta;

		if (delta > max_samples)
			max_samples = delta;

		all_samples += delta;
		num_wakeups++;

		if (delta > I2S_BUF_LEN) {
			puts("sdk: audio buffer underflow");
			delta = I2S_BUF_LEN;
			rx_offset = xx_limit - delta;
			tx_offset = rx_offset;
		}

		game_audio(delta);

		rx_offset = xx_limit;
		tx_offset = xx_limit;

		if (delta > 64) {
			sleep--;
		} else if (delta < 64) {
			sleep++;
		}

		if (sleep < 250)
			sleep = 250;

		if (sleep > 2500)
			sleep = 2500;

		task_sleep_us(sleep);
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

static void set_amp_enabled(bool en)
{
	sdk_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * SLAVE_AMP_EN_PIN,
		 (IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER_BITS |
		  (en ? IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER_BITS : 0) |
		  IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS));
}

void sdk_audio_init(void)
{
	set_amp_enabled(false);

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
		.sample_rate = SDK_AUDIO_RATE,
	};

	int offset = pio_i2s_init(&i2s_cfg);

	if (0 > offset)
		sdk_panic("sdk: pio_i2s_init failed\n");

	printf("sdk: pio_i2s offset=%i\n", offset);

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

void sdk_audio_start(void)
{
	dma_hw->multi_channel_trigger = (1u << dma_i2s_tx) | (1u << dma_i2s_rx);
}

void sdk_audio_report(void)
{
	printf("sdk: audio samples: min=%i, avg=%i, max=%i / buf=%i\n", min_samples,
	       all_samples / num_wakeups, max_samples, I2S_BUF_LEN);

	num_wakeups = 0;
	min_samples = INT_MAX;
	max_samples = 0;
	all_samples = 0;
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
