#include <pico/stdlib.h>

#include <hardware/i2c.h>
#include <hardware/dma.h>

#include <hardware/regs/io_qspi.h>

#include <limits.h>
#include <stdio.h>

#include <task.h>

#include <sdk.h>
#include <sdk/remote.h>

#include "nau88c22.h"
#include "i2s.pio.h"

static struct nau88c22_driver dsp = {
	.i2c = DSP_I2C,
	.addr = NAU88C22_ADDR,
};

static int sm_i2s_rx = -1;
static int sm_i2s_tx = -1;
static int dma_i2s_rx = -1;
static int dma_i2s_tx = -1;

static unsigned rx_offset = 0;
static unsigned tx_offset = 0;
static unsigned xx_limit = 0;

static int num_wakeups = 0;
static int min_samples = INT_MAX;
static int max_samples = 0;
static int all_samples = 0;

#define I2S_BUF_BITS 12
#define I2S_BUF_LEN (1 << (I2S_BUF_BITS - 2))

static int16_t i2s_rx_buf[I2S_BUF_LEN][2] __attribute__((__aligned__(I2S_BUF_LEN * 4)));
static int16_t i2s_tx_buf[I2S_BUF_LEN][2] __attribute__((__aligned__(I2S_BUF_LEN * 4)));

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

		if (delta)
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

void sdk_write_sample(int16_t left, int16_t right)
{
	size_t idx = tx_offset++ % I2S_BUF_LEN;
	i2s_tx_buf[idx][0] = left;
	i2s_tx_buf[idx][1] = right;
}

void sdk_read_sample(int16_t *left, int16_t *right)
{
	size_t idx = rx_offset++ % I2S_BUF_LEN;
	*left = i2s_rx_buf[idx][0];
	*right = i2s_rx_buf[idx][1];
}

void sdk_audio_init(void)
{
	unsigned rate = i2c_init(DSP_I2C, 100000);

	printf("sdk: i2c rate=%u\n", rate);

	gpio_set_function(DSP_CDATA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(DSP_CCLK_PIN, GPIO_FUNC_I2C);

	gpio_set_pulls(DSP_CDATA_PIN, true, false);
	gpio_set_pulls(DSP_CCLK_PIN, true, false);

	if (0 > nau88c22_reset(&dsp)) {
		printf("sdk: failed to reset SDP\n");
	} else {
		printf("sdk: DSP reset\n");
	}

	int device_id = nau88c22_identify(&dsp);
	int id = device_id >> 16;
	int rev = device_id & 0xffff;

	if (id == dsp.addr) {
		printf("sdk: DSP identified as NAU88C22 rev. %x\n", rev);
	} else {
		printf("sdk: DSP identified as %x rev %x\n", id, rev);
	}

	sm_i2s_rx = pio_claim_unused_sm(SDK_PIO, true);
	sm_i2s_tx = pio_claim_unused_sm(SDK_PIO, true);

	printf("sdk: pio_i2s_rx: claimed sm%i\n", sm_i2s_rx);
	printf("sdk: pio_i2s_tx: claimed sm%i\n", sm_i2s_tx);

	struct pio_i2s_config i2s_cfg = {
		.origin = -1,
		.pio = SDK_PIO,
		.rx_sm = sm_i2s_rx,
		.tx_sm = sm_i2s_tx,
		.in_pin = DSP_DOUT_PIN,
		.out_pin = DSP_DIN_PIN,
		.clock_pin_base = DSP_LRCK_PIN,
		.sample_rate = SDK_AUDIO_RATE,
	};

	int rx_offset = pio_i2s_rx_init(&i2s_cfg);

	if (0 > rx_offset)
		sdk_panic("sdk: pio_i2s_rx_init failed\n");

	printf("sdk: pio_i2s_rx loaded at %i-%i\n", rx_offset,
	       rx_offset + pio_i2s_rx_program.length - 1);

	int tx_offset = pio_i2s_tx_init(&i2s_cfg);

	if (0 > tx_offset)
		sdk_panic("sdk: pio_i2s_tx_init failed\n");

	printf("sdk: pio_i2s_tx loaded at %i-%i\n", tx_offset,
	       tx_offset + pio_i2s_tx_program.length - 1);

	dma_i2s_rx = dma_claim_unused_channel(true);
	dma_i2s_tx = dma_claim_unused_channel(true);

	printf("sdk: pio_i2s_rx: claimed dma%i\n", dma_i2s_rx);
	printf("sdk: pio_i2s_tx: claimed dma%i\n", dma_i2s_tx);

	dma_channel_config dma_cfg;

	dma_cfg = dma_channel_get_default_config(dma_i2s_rx);
	channel_config_set_dreq(&dma_cfg, pio_get_dreq(SDK_PIO, sm_i2s_rx, false));
	channel_config_set_read_increment(&dma_cfg, false);
	channel_config_set_write_increment(&dma_cfg, true);
	channel_config_set_ring(&dma_cfg, true, I2S_BUF_BITS);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
	dma_channel_configure(dma_i2s_rx, &dma_cfg, i2s_rx_buf, &SDK_PIO->rxf[sm_i2s_rx], UINT_MAX,
			      false);

	dma_cfg = dma_channel_get_default_config(dma_i2s_tx);
	channel_config_set_dreq(&dma_cfg, pio_get_dreq(SDK_PIO, sm_i2s_tx, true));
	channel_config_set_read_increment(&dma_cfg, true);
	channel_config_set_write_increment(&dma_cfg, false);
	channel_config_set_ring(&dma_cfg, false, I2S_BUF_BITS);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
	dma_channel_configure(dma_i2s_tx, &dma_cfg, &SDK_PIO->txf[sm_i2s_tx], i2s_tx_buf, UINT_MAX,
			      false);

	puts("sdk: configured i2s");

	nau88c22_start(&dsp);
	nau88c22_set_output_gain(&dsp, 0);

	puts("sdk: configured audio DSP");
}

void sdk_audio_start(void)
{
	dma_hw->multi_channel_trigger = (1u << dma_i2s_tx) | (1u << dma_i2s_rx);
	pio_enable_sm_mask_in_sync(SDK_PIO, (1 << sm_i2s_rx) | (1 << sm_i2s_tx));
}

void sdk_audio_report(void)
{
	static uint32_t last_wakeup = 0;

	uint32_t now = time_us_32();
	uint32_t diff = now - last_wakeup;
	last_wakeup = now;

	float rate = (float)all_samples / (1e-6f * diff);

	printf("sdk: audio samples: min=%i avg=%i max=%i / buf=%i, rate=%.0f\n", min_samples,
	       all_samples / num_wakeups, max_samples, I2S_BUF_LEN, rate);

	num_wakeups = 0;
	min_samples = INT_MAX;
	max_samples = 0;
	all_samples = 0;
}

void sdk_set_output_gain_db(float gain)
{
	nau88c22_set_output_gain(&dsp, gain);
}

void sdk_enable_headphones(bool en)
{
	nau88c22_enable_headphones(&dsp, en);
	nau88c22_enable_speaker(&dsp, !en);
}
