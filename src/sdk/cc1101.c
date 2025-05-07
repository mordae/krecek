#include "cc1101.h"
#include "cc1101_regs.h"

#include <math.h>
#include <sdk/remote.h>

#include <pico/stdlib.h>
#include <hardware/regs/clocks.h>
#include <string.h>

static void exchange_bits(const uint8_t *txbuf, uint8_t *rx)
{
	uint8_t tx = *txbuf;

	for (int i = 0; i < 8; i++) {
		*rx <<= 1;

		remote_gpio_set(SLAVE_TOUCH_MOSI_PIN, tx >> 7);
		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 1);
		*rx |= remote_gpio_get(SLAVE_TOUCH_MISO_PIN);
		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 0);

		tx <<= 1;
	}
}

static void read_register(uint8_t addr, void *buf, int len)
{
	remote_gpio_set(SLAVE_RF_CS_PIN, 0);

	while (remote_gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	for (int i = 0; i < len; i++) {
		uint8_t status;
		exchange_bits(&addr, &status);

		uint8_t null = 0;
		exchange_bits(&null, buf + i);
	}

	remote_gpio_set(SLAVE_RF_CS_PIN, 1);
}

static void write_register(uint8_t addr, const void *data, int len)
{
	remote_gpio_set(SLAVE_RF_CS_PIN, 0);

	while (remote_gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	for (int i = 0; i < len; i++) {
		uint8_t status;
		exchange_bits(&addr, &status);

		uint8_t null;
		exchange_bits(data, &null);
	}

	remote_gpio_set(SLAVE_RF_CS_PIN, 1);
}

static void write_command(uint8_t addr)
{
	remote_gpio_set(SLAVE_RF_CS_PIN, 0);

	while (remote_gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	uint8_t status;
	exchange_bits(&addr, &status);

	remote_gpio_set(SLAVE_RF_CS_PIN, 1);
}

static void find_best_rate(float goal, uint8_t *m, uint8_t *e)
{
	float step = (CLK_SYS_HZ / 5.0f) / (1 << 28);
	float best = 1e20;

	*m = 0;
	*e = 0;

	for (int ee = 0; ee < 16; ee++) {
		for (int mm = 0; mm < 256; mm++) {
			float rate = step * (256 + mm) * (1 << ee);
			float diff = fabsf(rate - goal);

			if (diff < best) {
				best = diff;
				*m = mm;
				*e = ee;
			}
		}
	}
}

void cc1101_init(void)
{
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_RF_CS_PIN);
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_MISO_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_MOSI_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_SCK_PIN);

	remote_gpio_pad_set(0, 1, 0, 0, 0, 1, 1, SLAVE_RF_XOSC_PIN);
	remote_poke(CLOCKS_BASE + CLOCKS_CLK_GPOUT0_DIV_OFFSET, 5 << 8);
	remote_poke(CLOCKS_BASE + CLOCKS_CLK_GPOUT0_CTRL_OFFSET, (1 << 12) | (1 << 11));
	remote_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * SLAVE_RF_XOSC_PIN,
		    IO_BANK0_GPIO21_CTRL_FUNCSEL_VALUE_CLOCKS_GPOUT_0);

	write_command(SRES);

	struct IOCFG0 iocfg0 = { .TEMP_SENSOR_ENABLE = 1 };
	write_register(REG_IOCFG0, &iocfg0, 1);

	struct SYNC1 sync1 = { .SYNC = 0x7a };
	struct SYNC0 sync0 = { .SYNC = 0xb5 };
	write_register(REG_SYNC1, &sync1, 1);
	write_register(REG_SYNC0, &sync0, 1);

	struct PKTLEN pktlen = { .PACKET_LENGTH = 60 };
	write_register(REG_PKTLEN, &pktlen, 1);

	struct PKTCTRL1 pktctrl1 = {
		.ADR_CHK = 0b11,
		.APPEND_STATUS = 1,
		.CRC_AUTOFLUSH = 0,
		.PQT = 0,
	};
	write_register(REG_PKTCTRL1, &pktctrl1, 1);

	struct PKTCTRL0 pktctrl0 = {
		.LENGTH_CONFIG = 0b01, // Variable length
		.CRC_EN = 1,
		.WHITE_DATA = 0,
	};
	write_register(REG_PKTCTRL0, &pktctrl0, 1);

	struct ADDR addr = { .DEVICE_ADDR = 42 }; // TODO
	write_register(REG_ADDR, &addr, 1);

	struct CHANNR channr = { .CHAN = 0 };
	write_register(REG_CHANNR, &channr, 1);

	struct FSCTRL1 fsctrl1 = { .FREQ_IF = 16 }; // 412_500 kHz
	write_register(REG_FSCTRL1, &fsctrl1, 1);

	struct FSCTRL0 fsctrl0 = { .FREQOFF = 0 };
	write_register(REG_FSCTRL0, &fsctrl0, 1);

	float freq_target = 433925000.0f;
	float freq_step = (CLK_SYS_HZ / 5.0f) / (1 << 16);
	unsigned freq = roundf(freq_target / freq_step);

	struct FREQ2 freq2 = { .FREQ = freq >> 16 };
	struct FREQ1 freq1 = { .FREQ = freq >> 8 };
	struct FREQ0 freq0 = { .FREQ = freq };

	write_register(REG_FREQ2, &freq2, 1);
	write_register(REG_FREQ1, &freq1, 1);
	write_register(REG_FREQ0, &freq0, 1);

	// Channel Bandwidth
	//
	// E M Bandwidth
	// 0 0 825_000
	// 0 1 660_000
	// 0 2 550_000
	// 0 3 471_429
	// 1 0 412_500
	// 1 1 330_000
	// 1 2 275_000
	// 1 3 235_714
	// 2 0 206_250
	// 2 1 165_000
	// 2 2 137_500
	// 2 3 117_857
	// 3 0 103_125
	// 3 1  82_500
	// 3 2  68_750
	// 3 3  58_929

	uint8_t drate_m, drate_e;
	find_best_rate(6000.0f, &drate_m, &drate_e);

	struct MDMCFG4 mdmcfg4 = {
		.CHANBW_E = 3,
		.CHANBW_M = 3,
		.DRATE_E = drate_e,
	};
	struct MDMCFG3 mdmcfg3 = { .DRATE_M = drate_m };

	write_register(REG_MDMCFG4, &mdmcfg4, 1);
	write_register(REG_MDMCFG3, &mdmcfg3, 1);

	struct MDMCFG2 mdmcfg2 = {
		.CARRIER_SENSE = 0,
		.SYNC_MODE = 2,
		.MANCHESTER_EN = 0,
		.MOD_FORMAT = 1, // GFSK
	};
	write_register(REG_MDMCFG2, &mdmcfg2, 1);

	struct MDMCFG1 mdmcfg1 = {
		.CHANSPC_E = 0,	   // TODO
		.NUM_PREAMBLE = 0, // 2 bytes
		.FEC_EN = 0,
	};
	write_register(REG_MDMCFG1, &mdmcfg1, 1);

	struct MDMCFG0 mdmcfg0 = {
		.CHANSPC_M = 0, // TODO
	};
	write_register(REG_MDMCFG0, &mdmcfg0, 1);

	struct DEVIATN deviatn = {
		.DEVIATION_E = 1,
		.DEVIATION_M = 7,
	};
	write_register(REG_DEVIATN, &deviatn, 1);

	struct FREND0 frend0 = {
		.LODIV_BUF_CURRENT_TX = 1,
		.PA_POWER = 1,
	};
	write_register(REG_FREND0, &frend0, 1);

	uint8_t patable[8] = { 0x00, 0x88, 0xca, 0xc5, 0xc3, 0xc2, 0xc1, 0xc0 };
	write_register(REG_PATABLE | FLAG_BURST, patable, 8);

	write_command(SCAL);
}

void cc1101_receive(void)
{
	write_command(SRX);
}

void cc1101_transmit(const void *buf, size_t len)
{
	write_register(REG_FIFO, buf, len);
	write_command(STX);
}

void cc1101_idle(void)
{
	write_command(SIDLE);
}

float cc1101_get_rssi(void)
{
	struct RSSI rssi;
	read_register(REG_RSSI, &rssi, 1);
	return rssi.RSSI * 0.5f - 74.0f;
}

bool cc1101_poll(void *buf, size_t *len)
{
	struct RXBYTES rxbytes;
	read_register(REG_RXBYTES, &rxbytes, 1);

	*len = rxbytes.NUM_RXBYTES;
	memset(buf, 0, *len);

	return (*len > 0);
}
