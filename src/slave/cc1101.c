#include "cc1101.h"
#include "cc1101_regs.h"

#include <math.h>

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

static spi_inst_t *spi = NULL;

static struct STATUS read_register(uint8_t addr, void *buf, int len)
{
	addr |= FLAG_READ;

	if (len > 1)
		addr |= FLAG_BURST;

	gpio_put(SLAVE_RF_CS_PIN, 0);

	while (gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	struct STATUS status;
	spi_write_read_blocking(spi, &addr, (uint8_t *)&status, 1);
	spi_read_blocking(spi, 0x00, buf, len);

	gpio_put(SLAVE_RF_CS_PIN, 1);

	return status;
}

static struct STATUS write_register(uint8_t addr, const void *data, int len)
{
	if (len > 1)
		addr |= FLAG_BURST;

	gpio_put(SLAVE_RF_CS_PIN, 0);

	while (gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	struct STATUS status;
	spi_write_read_blocking(spi, &addr, (uint8_t *)&status, 1);
	spi_write_blocking(spi, data, len);

	gpio_put(SLAVE_RF_CS_PIN, 1);

	return status;
}

static struct STATUS command(uint8_t addr)
{
	gpio_put(SLAVE_RF_CS_PIN, 0);

	while (gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	struct STATUS status;
	spi_write_read_blocking(spi, &addr, (uint8_t *)&status, 1);

	gpio_put(SLAVE_RF_CS_PIN, 1);

	return status;
}

static void find_best_rate(float goal, uint8_t *m, uint8_t *e)
{
	float step = 26000000.0f / (1 << 28);
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

void cc1101_init(spi_inst_t *_spi)
{
	spi = _spi;

	command(SRES);

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
		.CRC_AUTOFLUSH = 1,
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

	float freq_target = 434100000.0f;
	float freq_step = 26000000.0f / (1 << 16);
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
	find_best_rate(16000.0f, &drate_m, &drate_e);

	struct MDMCFG4 mdmcfg4 = {
		.CHANBW_E = 3,
		.CHANBW_M = 3,
		.DRATE_E = drate_e,
	};
	struct MDMCFG3 mdmcfg3 = { .DRATE_M = drate_m };

	write_register(REG_MDMCFG4, &mdmcfg4, 1);
	write_register(REG_MDMCFG3, &mdmcfg3, 1);

	struct MDMCFG2 mdmcfg2 = {
		.SYNC_MODE = 0b001, /* 15/16 bits, no carrier sense */
		.MANCHESTER_EN = 0,
		.MOD_FORMAT = 0b001, /* GFSK */
	};
	write_register(REG_MDMCFG2, &mdmcfg2, 1);

	struct MDMCFG1 mdmcfg1 = {
		.CHANSPC_E = 0,	       // TODO
		.NUM_PREAMBLE = 0b000, // 2 bytes
		.FEC_EN = 0,
	};
	write_register(REG_MDMCFG1, &mdmcfg1, 1);

	struct MDMCFG0 mdmcfg0 = {
		.CHANSPC_M = 0, // TODO
	};
	write_register(REG_MDMCFG0, &mdmcfg0, 1);

	struct DEVIATN deviatn = {
		.DEVIATION_E = 1, /* 1587, 3174, 6348, 12695, ... */
		.DEVIATION_M = 0,
	};
	write_register(REG_DEVIATN, &deviatn, 1);

	struct FREND0 frend0 = {
		.LODIV_BUF_CURRENT_TX = 1,
		.PA_POWER = 7,
	};
	write_register(REG_FREND0, &frend0, 1);

	uint8_t patable[8] = { 0x00, 0x88, 0xca, 0xc5, 0xc3, 0xc2, 0xc1, 0xc0 };
	write_register(REG_PATABLE, patable, 8);

	command(SCAL);
}

bool cc1101_receive(void)
{
	struct STATUS status = command(SNOP);

	struct TXBYTES txbytes;
	status = read_register(REG_TXBYTES, &txbytes, 1);

	if (txbytes.NUM_TXBYTES)
		return false; /* Still has bytes in the TX FIFO. */

	if (status.STATE == STATUS_STATE_RX)
		return true; /* Already there. */

	if (status.STATE == STATUS_STATE_IDLE) {
		command(SRX);
		return true;
	}

	return false; /* Busy in other state. */
}

bool cc1101_transmit(const void *buf, size_t len)
{
	struct STATUS status;
	struct RXBYTES rxbytes;

	status = read_register(REG_RXBYTES, &rxbytes, 1);

	if (status.STATE == STATUS_STATE_TX)
		return false; /* Busy transmitting. */

	if (rxbytes.NUM_RXBYTES)
		return false; /* Busy receiving a packet. */

	/* Go back to IDLE and flush the TX FIFO. */
	command(SIDLE);
	command(SFTX);

	/* Write the packet and begin transmit. */
	write_register(REG_FIFO, buf, len);
	command(STX);

	return true;
}

void cc1101_idle(void)
{
	command(SIDLE);
	command(SFRX);
	command(SFTX);
}

float cc1101_get_rssi(void)
{
	struct RSSI rssi;
	read_register(REG_RSSI, &rssi, 1);
	return rssi.RSSI * 0.5f - 74.0f;
}

int cc1101_poll(void *buf)
{
	struct STATUS status;

	struct RXBYTES rxbytes;
	status = read_register(REG_RXBYTES, &rxbytes, 1);

	if (!rxbytes.NUM_RXBYTES)
		return -1;

	if (status.STATE != STATUS_STATE_IDLE)
		return -1;

	read_register(REG_FIFO, buf, rxbytes.NUM_RXBYTES);
	return rxbytes.NUM_RXBYTES;
}
