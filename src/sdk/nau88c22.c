#include <nau88c22.h>

#include "nau88c22_regs.h"

#include <stdio.h>

#define TIMEOUT 10000
#define TAG "nau88c22: "

#define return_on_error(expr)         \
	({                            \
		int __res = (expr);   \
		if (__res < 0)        \
			return __res; \
		__res;                \
	})

static int read_reg(nau88c22_driver_t drv, uint8_t addr, void *buf)
{
	uint16_t packet;

	addr <<= 1;

	if (1 != i2c_write_timeout_us(drv->i2c, drv->addr, &addr, 1, true, TIMEOUT))
		goto fail;

	if (2 != i2c_read_timeout_us(drv->i2c, drv->addr, (uint8_t *)&packet, 2, false, TIMEOUT))
		goto fail;

	packet = __builtin_bswap16(packet);
	packet |= addr << 8;
	*(uint16_t *)buf = packet;

	return 0;

fail:
	printf(TAG "read register 0x%hhx failed\n", addr >> 1);
	return -1;
}

static int write_reg(nau88c22_driver_t drv, const void *buf)
{
	uint16_t packet = __builtin_bswap16(*(uint16_t *)buf);

	if (2 != i2c_write_timeout_us(drv->i2c, drv->addr, (uint8_t *)&packet, 2, false, TIMEOUT))
		goto fail;

	return 0;

fail:
	printf(TAG "write register 0x%hhx failed\n", packet >> 9);
	return -1;
}

int nau88c22_identify(nau88c22_driver_t drv)
{
	struct DeviceId deviceId;
	struct DeviceRevision deviceRevision;

	return_on_error(read_reg(drv, DEVICE_REVISION_ADDR, &deviceRevision));
	return_on_error(read_reg(drv, DEVICE_ID_ADDR, &deviceId));

	return (deviceId.ID << 16) | deviceRevision.REV;
}

int nau88c22_reset(nau88c22_driver_t drv)
{
	struct Reset reset = {
		.addr = RESET_ADDR,
		.RESET = 0x1ff,
	};
	return write_reg(drv, &reset);
}

int nau88c22_start(nau88c22_driver_t drv)
{
	struct PowerManagement1 pm1 = {
		.addr = POWER_MANAGEMENT_1_ADDR,
		.DCBUFEN = 1,
		.ABIASEN = 1,
		.IOBUFEN = 1,
		.REFIMP = 1, // 80kΩ
	};
	return_on_error(write_reg(drv, &pm1));

	struct PowerManagement2 pm2 = {
		.addr = POWER_MANAGEMENT_2_ADDR,
		.RHPEN = 1,
		.LHPEN = 1,
	};
	return_on_error(write_reg(drv, &pm2));

	struct PowerManagement3 pm3 = {
		.addr = POWER_MANAGEMENT_3_ADDR,
		.AUXOUT1EN = 0,
		.AUXOUT2EN = 0,
		.LSPKEN = 1,
		.RSPKEN = 1,
		.RMIXEN = 1,
		.LMIXEN = 1,
		.RDACEN = 1,
		.LDACEN = 1,
	};
	return_on_error(write_reg(drv, &pm3));

	struct AudioInterface aif = {
		.addr = AUDIO_INTERFACE_ADDR,
		.WLEN = 0,  // 16b
		.AIFMT = 2, // I²S
	};
	return_on_error(write_reg(drv, &aif));

	struct ClockControl1 clock1 = {
		.addr = CLOCK_CONTROL_1_ADDR,
		.CLKM = 0,    // Use SCLK as MCLK directly
		.MCLKSEL = 0, // Divide by 1
	};
	return_on_error(write_reg(drv, &clock1));

	struct ClockControl2 clock2 = {
		.addr = CLOCK_CONTROL_2_ADDR,
		.SMPLR = 0,  // 48 kHz, maximum
		.SCLKEN = 1, // Slow clock enable
	};
	return_on_error(write_reg(drv, &clock2));

	struct _192kHzSampling hss = {
		.addr = _192KHZ_SAMPLING_ADDR,
		.ADC_OSR32x = 1,
		.DAC_OSR32x = 1,
		.UNKNOWN = 1,
	};
	return_on_error(write_reg(drv, &hss));

	struct LeftDACVolume ldacvol = {
		.addr = LEFT_DAC_VOLUME_ADDR,
		.LDACGAIN = 255,
	};
	return_on_error(write_reg(drv, &ldacvol));

	struct RightDACVolume rdacvol = {
		.addr = RIGHT_DAC_VOLUME_ADDR,
		.RDACGAIN = 255,
		.RDACVU = 1,
	};
	return_on_error(write_reg(drv, &rdacvol));

	struct LeftMixer lmix = {
		.addr = LEFT_MIXER_ADDR,
		.LDACLMX = 1,
	};
	return_on_error(write_reg(drv, &lmix));

	struct RightMixer rmix = {
		.addr = RIGHT_MIXER_ADDR,
		.RDACRMX = 1,
	};
	return_on_error(write_reg(drv, &rmix));

	struct RightSpeakerSubmix rsubmix = {
		.addr = RIGHT_SPEAKER_SUBMIX_ADDR,
		.RSUBBYP = 1, // Invert signal to drive speaker as bridge
	};
	return_on_error(write_reg(drv, &rsubmix));

	struct OutputControl outctrl = {
		.addr = OUTPUT_CONTROL_ADDR,
		.SPKBST = 1,
		.TSEN = 1, // Thermal shutdown
	};
	return_on_error(write_reg(drv, &outctrl));

	return 0;
}

int nau88c22_set_output_gain(nau88c22_driver_t drv, float gain)
{
	(void)drv;
	(void)gain;
	return -1;
}
