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
		.MICBIASEN = 1,
		.DCBUFEN = 1,
		.PLLEN = 0, // No PLL until we set things up.
		.ABIASEN = 1,
		.IOBUFEN = 1,
		.REFIMP = 1, // 80kΩ
	};
	return_on_error(write_reg(drv, &pm1));

	struct PowerManagement2 pm2 = {
		.addr = POWER_MANAGEMENT_2_ADDR,
		.RHPEN = 0,
		.LHPEN = 0,
		.LADCEN = 1,
		.RADCEN = 1,
		.LBSTEN = 1,
		.RBSTEN = 1,
		.LPGAEN = 0,
		.RPGAEN = 0,
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

	struct PLLK1 pllk1 = {
		.addr = PLL_K1_ADDR,
		.PLLK = 0,
	};
	return_on_error(write_reg(drv, &pllk1));

	struct PLLK2 pllk2 = {
		.addr = PLL_K2_ADDR,
		.PLLK = 0,
	};
	return_on_error(write_reg(drv, &pllk2));

	struct PLLK3 pllk3 = {
		.addr = PLL_K3_ADDR,
		.PLLK = 0,
	};
	return_on_error(write_reg(drv, &pllk3));

	struct PLLN plln = {
		.addr = PLL_N_ADDR,
		.PLLN = 8,
		.PLLMCLK = 0, // Divide by 1
	};
	return_on_error(write_reg(drv, &plln));

	struct AudioInterface aif = {
		.addr = AUDIO_INTERFACE_ADDR,
		.WLEN = 0,     // 16b
		.AIFMT = 0b11, // PCM Data
		.LRP = 0,      // PCM A
		.DACPHS = 0,   // Invert Left/Right for DAC
		.ADCPHS = 0,   // Invert Left/Right for ADC
	};
	return_on_error(write_reg(drv, &aif));

	struct ClockControl1 clock1 = {
		.addr = CLOCK_CONTROL_1_ADDR,
		.CLKM = 0,    // Use MCLK directly
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
		.PLL49MOUT = 0,
		.ADCB_OVER = 0,
	};
	return_on_error(write_reg(drv, &hss));

	struct LeftDACVolume ldacvol = {
		.addr = LEFT_DAC_VOLUME_ADDR,
		.LDACVU = 1,
		.LDACGAIN = 255 - (2 * 6), // -6 dB
	};
	return_on_error(write_reg(drv, &ldacvol));

	struct RightDACVolume rdacvol = {
		.addr = RIGHT_DAC_VOLUME_ADDR,
		.RDACVU = 1,
		.RDACGAIN = 255 - (2 * 6), // -6 dB
	};
	return_on_error(write_reg(drv, &rdacvol));

	struct LHPVolume lhpvol = {
		.addr = LHP_VOLUME_ADDR,
		.LHPVU = 1,
		.LHPGAIN = 63 - 36, // 6 - 36 dB
	};
	return_on_error(write_reg(drv, &lhpvol));

	struct RHPVolume rhpvol = {
		.addr = RHP_VOLUME_ADDR,
		.RHPVU = 1,
		.RHPGAIN = 63 - 36, // 6 - 36 dB
	};
	return_on_error(write_reg(drv, &rhpvol));

	struct LSPKOutVolume lspkvol = {
		.addr = LSPKOUT_VOLUME_ADDR,
		.LSPKVU = 1,
		.LSPKGAIN = 63 - 14, // 6 - 14 dB
	};
	return_on_error(write_reg(drv, &lspkvol));

	struct RSPKOutVolume rspkvol = {
		.addr = RSPKOUT_VOLUME_ADDR,
		.RSPKVU = 1,
		.RSPKGAIN = 63 - 14, // 6 - 14 dB
	};
	return_on_error(write_reg(drv, &rspkvol));

	struct LeftMixer lmix = {
		.addr = LEFT_MIXER_ADDR,
		.LDACLMX = 1, // Left DAC to LMAIN
		.LBYPLMX = 0, // Left Analog to LMAIN
	};
	return_on_error(write_reg(drv, &lmix));

	struct RightMixer rmix = {
		.addr = RIGHT_MIXER_ADDR,
		.RDACRMX = 1, // Right DAC to RMAIN
		.RBYPRMX = 0, // Right Analog to RMAIN
	};
	return_on_error(write_reg(drv, &rmix));

	struct RightSpeakerSubmix rsubmix = {
		.addr = RIGHT_SPEAKER_SUBMIX_ADDR,
		.RSUBBYP = 1, // Invert signal to drive speaker as bridge
	};
	return_on_error(write_reg(drv, &rsubmix));

	struct InputControl inctrl = {
		.addr = INPUT_CONTROL_ADDR,
		.MICBIASV = 1,	// ~2.2V
		.LLINLPGA = 1,	// Left line into left amplifier (IR)
		.RMICNRPGA = 1, // Right mic P/N into right amplifier (HP)
		.RMICPRPGA = 1,
	};
	return_on_error(write_reg(drv, &inctrl));

	struct LeftADCBoost ladcbst = {
		.addr = LEFT_ADC_BOOST_ADDR,
		.LAUXBSTGAIN = 0b101, // LAUX to LADC at +0 dB
	};
	return_on_error(write_reg(drv, &ladcbst));

	struct RightADCBoost radcbst = {
		.addr = RIGHT_ADC_BOOST_ADDR,
		.RPGABSTGAIN = 0b101, // Right PGA to RADC at +0 dB
	};
	return_on_error(write_reg(drv, &radcbst));

	struct OutputControl outctrl = {
		.addr = OUTPUT_CONTROL_ADDR,
		.TSEN = 1, // Thermal shutdown
		.AUX1BST = 1,
		.AUX2BST = 1,
		.SPKBST = 1,
		.LDACRMX = 0,
		.RDACLMX = 0,
	};
	return_on_error(write_reg(drv, &outctrl));

	struct DACControl dacctrl = {
		.addr = DAC_CONTROL_ADDR,
		.DACOS = 1,
	};
	return_on_error(write_reg(drv, &dacctrl));

	struct DACDither dacdither = {
		.addr = DAC_DITHER_ADDR,
		.ANALOG = 4,
		.MODULATOR = 18,
	};
	return_on_error(write_reg(drv, &dacdither));

	struct ADCControl adcctrl = {
		.addr = ADC_CONTROL_ADDR,
		.ADCOS = 1,
		.HPFEN = 1, // Enable high-pass filter at ~4 Hz
	};
	return_on_error(write_reg(drv, &adcctrl));

	struct MiscControls miscctrl = {
		.addr = MISC_CONTROLS_ADDR,
		.DACOSR256 = 1,
	};
	return_on_error(write_reg(drv, &miscctrl));

#if 0
	struct EQ1LowCutoff eq1 = {
		.addr = EQ1_LOW_CUTOFF_ADDR,
		.EQ1GC = 0b11000, // -12dB
	};
	return_on_error(write_reg(drv, &eq1));

	struct EQ2Peak1 eq2 = {
		.addr = EQ2_PEAK_1_ADDR,
		.EQ2GC = 0b11000, // -12dB
	};
	return_on_error(write_reg(drv, &eq2));

	struct EQ3Peak2 eq3 = {
		.addr = EQ3_PEAK_2_ADDR,
		.EQ3GC = 0b11000, // -12dB
	};
	return_on_error(write_reg(drv, &eq3));

	struct EQ4Peak3 eq4 = {
		.addr = EQ4_PEAK_3_ADDR,
		.EQ4GC = 0b11000, // -12dB
	};
	return_on_error(write_reg(drv, &eq4));

	struct EQ5HighCutoff eq5 = {
		.addr = EQ5_HIGH_CUTOFF_ADDR,
		.EQ5CF = 0b01,	  // 6.9 kHz
		.EQ5GC = 0b11000, // -12dB
	};
	return_on_error(write_reg(drv, &eq5));
#endif

	// pm1.PLLEN = 1;
	// return_on_error(write_reg(drv, &pm1));

	return 0;
}

inline static int clampi(int x, int lo, int hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

int nau88c22_set_output_gain(nau88c22_driver_t drv, float gain)
{
	int dacgain = 255 + clampi(2 * (gain - 6.0f), -255, 0);

	struct LeftDACVolume ldacvol = {
		.addr = LEFT_DAC_VOLUME_ADDR,
		.LDACVU = 1,
		.LDACGAIN = dacgain,
	};
	return_on_error(write_reg(drv, &ldacvol));

	struct RightDACVolume rdacvol = {
		.addr = RIGHT_DAC_VOLUME_ADDR,
		.RDACGAIN = dacgain,
		.RDACVU = 1,
	};
	return_on_error(write_reg(drv, &rdacvol));

	return 0;
}

int nau88c22_enable_headphones(nau88c22_driver_t drv, bool en)
{
	struct PowerManagement2 pm2;
	return_on_error(read_reg(drv, POWER_MANAGEMENT_2_ADDR, &pm2));
	pm2.RHPEN = !!en;
	pm2.LHPEN = !!en;
	return_on_error(write_reg(drv, &pm2));
	return 0;
}

int nau88c22_enable_speaker(nau88c22_driver_t drv, bool en)
{
	// When speaker is active, combine both channels to obtain mono mix.
	// This affects both speaker and headphones, but we can't really do
	// anything about that and likely it's one or the other, not both.
	struct OutputControl outctrl;
	return_on_error(read_reg(drv, OUTPUT_CONTROL_ADDR, &outctrl));
	outctrl.LDACRMX = !!en;
	outctrl.RDACLMX = !!en;
	return_on_error(write_reg(drv, &outctrl));

	struct PowerManagement3 pm3;
	return_on_error(read_reg(drv, POWER_MANAGEMENT_3_ADDR, &pm3));
	pm3.LSPKEN = !!en;
	pm3.RSPKEN = !!en;
	return_on_error(write_reg(drv, &pm3));

	return 0;
}
