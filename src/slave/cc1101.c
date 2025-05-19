#include "cc1101.h"
#include "cc1101_regs.h"

#include <math.h>

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

static spi_inst_t *spi = NULL;

static struct STATUS read_register_unsafe(uint8_t addr, void *buf, int len)
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

static struct STATUS read_register_safe(uint8_t addr, void *buf)
{
	uint8_t r0, r1;

	union {
		uint8_t u8;
		struct STATUS st;
	} s0, s1;

	s1.st = read_register_unsafe(addr, &r1, 1);

	do {
		r0 = r1, s0 = s1;
		s1.st = read_register_unsafe(addr, &r1, 1);
	} while (r0 != r1 || s0.u8 != s1.u8);

	*(uint8_t *)buf = r1;

	return s1.st;
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

static struct STATUS read_status_safe(void)
{
	union {
		uint8_t u8;
		struct STATUS st;
	} s0, s1;

	s1.st = command(SNOP);

	do {
		s0 = s1;
		s1.st = command(SNOP);
	} while (s0.u8 != s1.u8);

	return s1.st;
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
		.SYNC_MODE = 0b110, /* 16/16 bits with carrier sense */
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
		.DEVIATION_E = 2, /* 1587, 3174, 6348, 12695, ... */
		.DEVIATION_M = 0,
	};
	write_register(REG_DEVIATN, &deviatn, 1);

	struct MCSM0 mcsm0 = {
		.FS_AUTOCAL = 0b10, /* RX/TX to IDLE */
		.PO_TIMEOUT = 0b01, /* default */
	};
	write_register(REG_MCSM0, &mcsm0, 1);

	struct MCSM1 mcsm1 = {
		.CCA_MODE = 0b11,   /* RSSI low, not receiving */
		.RXOFF_MODE = 0b11, /* RX after RX */
		.TXOFF_MODE = 0b11, /* RX after TX */
	};
	write_register(REG_MCSM1, &mcsm1, 1);

	struct AGCCTRL1 agcctrl1 = {
		.AGC_LNA_PRIORITY = 1,
		.CARRIER_SENSE_REL_THR = 0b10, /* 10 dB increase = busy */
		.CARRIER_SENSE_ABS_THR = -8,   /* no absolute threshold */
	};
	write_register(REG_AGCCTRL1, &agcctrl1, 1);

	struct FREND0 frend0 = {
		.LODIV_BUF_CURRENT_TX = 1,
		.PA_POWER = 7,
	};
	write_register(REG_FREND0, &frend0, 1);

	uint8_t patable[] = { 0x00, 0x88, 0xca, 0xc5, 0xc3, 0xc2, 0xc1, 0xc0 };
	write_register(REG_PATABLE, patable, sizeof(patable));
}

static void wait_for(enum STATUS_STATE state)
{
	struct STATUS status;
	do {
		status = read_status_safe();
	} while (status.STATE != state);
}

void cc1101_idle(void)
{
	command(SIDLE);
	command(SFRX);
	command(SFTX);
	wait_for(STATUS_STATE_IDLE);
}

void cc1101_calibrate(void)
{
	command(SCAL);
	wait_for(STATUS_STATE_IDLE);
}

void cc1101_begin_receive(void)
{
	command(SRX);
	wait_for(STATUS_STATE_RX);
}

float cc1101_get_rssi(void)
{
	struct RSSI rssi;
	read_register_safe(REG_RSSI, &rssi);
	return rssi.RSSI * 0.5f - 74.0f;
}

bool cc1101_transmit(const void *buf, int len)
{
	const uint8_t *data = buf;

	if (len < 1) {
		puts("cc1101: TX empty ?!");
		return true;
	}

	if (!data[0]) {
		puts("cc1101: TX len=0 ?!");
		return true;
	}

	struct STATUS status = read_status_safe();

	if (STATUS_STATE_TX == status.STATE)
		return false; /* Busy transmitting. */

	/* Prepare for TX. */
	command(SFSTXON);

	while (true) {
		struct STATUS status = read_status_safe();

		switch (status.STATE) {
		case STATUS_STATE_CALIBRATE:
		case STATUS_STATE_SETTLING:
			continue;

		case STATUS_STATE_TXFIFO_UNDERFLOW:
			puts("cc1101: TXFIFO_UNDERFLOW");
			command(SFTX);
			continue;

		case STATUS_STATE_FSTXON:
			write_register(REG_FIFO, data, len);
			command(STX);
			continue;

		case STATUS_STATE_TX:
			return true;

		case STATUS_STATE_RXFIFO_OVERFLOW:
			puts("cc1101: RXFIFO_OVERFLOW?!");
			command(SFRX);
			continue;

		case STATUS_STATE_IDLE:
		case STATUS_STATE_RX:
			// puts("!CCA");
			return false;
		}
	}
}

int cc1101_receive(void *buf)
{
	struct RXBYTES rxbytes;
	read_register_safe(REG_RXBYTES, &rxbytes);

	struct STATUS status;
	struct PKTSTATUS pktstatus;
	status = read_register_safe(REG_PKTSTATUS, &pktstatus);

	if (!rxbytes.NUM_RXBYTES)
		return -1;

	if (pktstatus.SFD)
		return -2;

	while (true) {
		switch (status.STATE) {
		case STATUS_STATE_CALIBRATE:
		case STATUS_STATE_SETTLING:
			continue;

		case STATUS_STATE_RXFIFO_OVERFLOW:
			puts("cc1101: RX FIFO overflow");
			command(SFRX);
			return -4;

		case STATUS_STATE_TXFIFO_UNDERFLOW:
			puts("cc1101: TX FIFO underflow");
			command(SFTX);
			return -4;

		case STATUS_STATE_IDLE:
			command(SRX);
			return -2;

		case STATUS_STATE_TX:
		case STATUS_STATE_FSTXON:
			return -2;

		case STATUS_STATE_RX:
			goto read;
		}
	}

read:
	uint8_t *data = buf;
	read_register_unsafe(REG_FIFO, data, 1);

	if (data[0] > 61) {
		printf("cc1101: RX len=%hhu ?!\n", data[0]);
		command(SFRX);
		return -3;
	}

	read_register_unsafe(REG_FIFO, data + 1, data[0] + 2);
	return data[0] + 3;
}
