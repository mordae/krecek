#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/adc.h>
#include <hardware/pll.h>
#include <hardware/spi.h>

#include <hardware/structs/pads_qspi.h>

#include <task.h>

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "cc1101.h"

struct mailbin mailbin __section(".mailbin");

static uint8_t rxbuf[MAILBIN_RF_SLOTS][CC1101_MAXLEN] __aligned(MAILBIN_RF_SLOTS *CC1101_MAXLEN);
static uint8_t txbuf[MAILBIN_RF_SLOTS][CC1101_MAXLEN] __aligned(MAILBIN_RF_SLOTS *CC1101_MAXLEN);

#define BASE_FREQ 433050000.0f
#define CHANNEL 25000.0f
static uint8_t rf_channel = 42;

#define REG_SET_FIELD(name, value) (((value) << (name##_LSB)) & (name##_BITS))

static void qspi_pad_set(bool od, bool ie, int drive, bool pue, bool pde, bool schmitt,
			 bool slewfast, int pad)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_OD, od);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_IE, ie);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_DRIVE, drive);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_PUE, pue);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_PDE, pde);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT, schmitt);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST, slewfast);

	// Pads have different order than controls. No idea why.
	uint8_t remap[6] = { 0, 5, 1, 2, 3, 4 };

	pads_qspi_hw->io[remap[pad]] = reg;
}

static int touch_read(uint8_t word)
{
	uint16_t response;

	gpio_put(SLAVE_TOUCH_CS_PIN, 0);
	spi_write_blocking(SLAVE_TOUCH_SPI, &word, 1);
	spi_read_blocking(SLAVE_TOUCH_SPI, 0x00, (uint8_t *)&response, 2);
	gpio_put(SLAVE_TOUCH_CS_PIN, 1);

	response = __builtin_bswap16(response);
	return (response & 0b0111111111111000) >> 3;
}

int main()
{
	/* Latch power on. */
	gpio_init(SLAVE_OFF_PIN);
	gpio_put(SLAVE_OFF_PIN, 1);
	gpio_set_dir(SLAVE_OFF_PIN, GPIO_OUT);

	/* Avoid triggering CC1101. */
	gpio_init(SLAVE_RF_CS_PIN);
	gpio_put(SLAVE_RF_CS_PIN, 1);
	gpio_set_dir(SLAVE_RF_CS_PIN, GPIO_OUT);

	/* Avoid triggering TSC2046. */
	gpio_init(SLAVE_TOUCH_CS_PIN);
	gpio_put(SLAVE_TOUCH_CS_PIN, 1);
	gpio_set_dir(SLAVE_TOUCH_CS_PIN, GPIO_OUT);

	/* Ready mailbin. */
	memset((void *)&mailbin, 0, sizeof(mailbin));

	for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
		mailbin.rf_rx_addr[i] = (uint32_t)&rxbuf[i];
		mailbin.rf_tx_addr[i] = (uint32_t)&txbuf[i];
	}

	mailbin.rf_channel = rf_channel;
	mailbin.gpio_input = ~0u;
	mailbin.qspi_input = ~0u;
	mailbin.magic = MAILBIN_MAGIC;

	/* Initialize stdio via mailbin. */
	mailbin_stdio_init();

	puts("slave: started");

	/* Disable USB as it's not connected anyway. */
	resets_hw->reset |= RESETS_RESET_USBCTRL_BITS;

	/*
	 * Reconfigure pll_usb we don't need for CC1101 we do need.
	 * 52 MHz is close enough for ADC 48 MHz and exactly 2x for CC1101.
	 */
	pll_init(pll_usb, 1, 1560000000, 6, 5);

	/* Start clocking CC1101. */
	gpio_disable_pulls(SLAVE_RF_XOSC_PIN);
	clock_gpio_init_int_frac16(SLAVE_RF_XOSC_PIN,
				   CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 2, 0);

	/* Prepare the shared SPI link. */
	spi_init(SLAVE_TOUCH_SPI, MIN(SLAVE_TOUCH_SPI_BAUDRATE, SLAVE_RF_SPI_BAUDRATE));

	/* Ready the ADC. */
	adc_init();

	/* A, B, X, Y, START - pullup */
	gpio_init(SLAVE_A_PIN);
	gpio_set_pulls(SLAVE_A_PIN, true, false);
	gpio_init(SLAVE_B_PIN);
	gpio_set_pulls(SLAVE_B_PIN, true, false);
	gpio_init(SLAVE_X_PIN);
	gpio_set_pulls(SLAVE_X_PIN, true, false);
	gpio_init(SLAVE_Y_PIN);
	gpio_set_pulls(SLAVE_Y_PIN, true, false);
	gpio_init(SLAVE_START_PIN);
	gpio_set_pulls(SLAVE_START_PIN, true, false);

	/* AUX[0-7] - pullup */
	gpio_init(SLAVE_AUX0_PIN);
	gpio_set_pulls(SLAVE_AUX0_PIN, true, false);
	gpio_init(SLAVE_AUX1_PIN);
	gpio_set_pulls(SLAVE_AUX1_PIN, true, false);
	gpio_init(SLAVE_AUX2_PIN);
	gpio_set_pulls(SLAVE_AUX2_PIN, true, false);
	gpio_init(SLAVE_AUX3_PIN);
	gpio_set_pulls(SLAVE_AUX3_PIN, true, false);
	gpio_init(SLAVE_AUX4_PIN);
	gpio_set_pulls(SLAVE_AUX4_PIN, true, false);
	gpio_init(SLAVE_AUX5_PIN);
	gpio_set_pulls(SLAVE_AUX5_PIN, true, false);
	gpio_init(SLAVE_AUX6_PIN);
	gpio_set_pulls(SLAVE_AUX6_PIN, true, false);
	gpio_init(SLAVE_AUX7_PIN);
	gpio_set_pulls(SLAVE_AUX7_PIN, true, false);

	/* BRACK_R, BRACK_L, JOY_X, JOY_Y - no pulls */
	gpio_disable_pulls(SLAVE_BRACK_L_PIN);
	gpio_disable_pulls(SLAVE_BRACK_R_PIN);
	gpio_disable_pulls(SLAVE_JOY_X_PIN);
	gpio_disable_pulls(SLAVE_JOY_Y_PIN);

	/* VOL_UP, VOL_DOWN, VOL_SW, SELECT - pullup */
	qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_UP_QSPI_PIN);
	qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_DOWN_QSPI_PIN);
	qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_SW_QSPI_PIN);
	qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_SELECT_QSPI_PIN);

	/* Shared SPI lanes */
	gpio_set_function(SLAVE_TOUCH_MISO_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SLAVE_TOUCH_MOSI_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SLAVE_TOUCH_SCK_PIN, GPIO_FUNC_SPI);

	/* Initialize CC1101 for RF communication. */
	cc1101_init(SLAVE_RF_SPI);

	/* Set default frequency. */
	cc1101_set_freq(BASE_FREQ + CHANNEL * rf_channel);

	/* Perform calibration */
	cc1101_calibrate();

	/* Begin receiving. */
	cc1101_begin_receive();

	puts("slave: configured");

	static int adc_hist[4][16];
	static int adc_total[4];
	static int adc_idx;

	static int touch_hist[4][16];
	static int touch_total[4];
	static int touch_idx;

	uint8_t touch_words[4] = {
		0b10010111,
		0b11010111,
		0b11000111,
		0b10110111,
	};

	uint64_t now = time_us_64();

	int next_tx_buf = 0;
	int next_rx_buf = 0;

	while (true) {
		mailbin.gpio_input = sio_hw->gpio_in;
		mailbin.qspi_input = sio_hw->gpio_hi_in;

		for (int a = 0; a < 4; a++) {
			int total = 0;
			adc_select_input(a);

			for (int i = 0; i < 16; i++)
				total += adc_read();

			adc_total[a] += total - adc_hist[a][adc_idx];
			adc_hist[a][adc_idx] = total;

			mailbin.adc[a] = adc_total[a] >> 4;
		}

		adc_idx = (adc_idx + 1) % 16;

		for (int t = 0; t < 4; t++) {
			int total = 0;

			for (int i = 0; i < 16; i++)
				total += touch_read(touch_words[t]);

			touch_total[t] += total - touch_hist[t][touch_idx];
			touch_hist[t][touch_idx] = total;

			mailbin.touch[t] = touch_total[t] >> 4;
		}

		touch_idx = (touch_idx + 1) % 16;

		if (rf_channel != (mailbin.rf_channel & 0xff)) {
			rf_channel = mailbin.rf_channel;
			float freq = 433050000.0f + 25000.0f * rf_channel;
			printf("CH%hhu %.0f\n", rf_channel, freq);
			cc1101_set_freq(freq);
		}

		for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
			int s = (next_rx_buf + i) % MAILBIN_RF_SLOTS;

			if (!mailbin.rf_rx_size[s]) {
				int len = cc1101_receive(rxbuf[s]);

				if (len >= 0) {
					mailbin.rf_rx_size[s] = len;
				} else {
					next_rx_buf = s;
					break;
				}
			}
		}

		for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
			int s = (next_tx_buf + i) % MAILBIN_RF_SLOTS;

			if (mailbin.rf_tx_size[s]) {
				if (cc1101_transmit(txbuf[s], mailbin.rf_tx_size[s])) {
					mailbin.rf_tx_size[s] = 0;
					next_tx_buf = (s + 1) % MAILBIN_RF_SLOTS;
					break;
				}

				next_tx_buf = s;
				break;
			}
		}

		now += 1000;
		sleep_until(now);
	}
}
