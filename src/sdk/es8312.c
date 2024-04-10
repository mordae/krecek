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

#include <es8312.h>

#include "es8312_regs.h"

#include <stdio.h>

#define TIMEOUT 10000
#define TAG "es8312: "

#define return_on_error(expr)         \
	({                            \
		int __res = (expr);   \
		if (__res < 0)        \
			return __res; \
		__res;                \
	})

struct state {
	struct RESET reset;
	struct CLOCKS clocks;
	struct SDP sdp;
	struct SYSTEM system;
	struct ADC adc;
	struct ADCEQ adceq;
	struct DAC dac;
	struct DACEQ daceq;
	struct GPIO gpio;
} __packed;

static_assert(ES8312_NUM_REGS == sizeof(struct state), "BUG: invalid register map size");

static int write_regs(es8312_driver_t drv, uint8_t base, const void *buf, size_t len)
{
	const uint8_t *bytes = buf;
	size_t i = 0;

	for (i = 0; i < len; i++) {
		uint8_t cmd[2] = { base + i, bytes[i] };

		if (2 != i2c_write_timeout_us(drv->i2c, drv->addr, cmd, 2, false, TIMEOUT))
			goto fail;
	}

	return 0;

fail:
	printf(TAG "write register %hhx failed\n", base + i);
	return -1;
}

static int read_regs(es8312_driver_t drv, uint8_t base, void *buf, size_t len)
{
	uint8_t *bytes = buf;
	size_t i = 0;

	for (i = 0; i < len; i++) {
		uint8_t reg = base + i;

		if (1 != i2c_write_timeout_us(drv->i2c, drv->addr, &reg, 1, true, TIMEOUT))
			goto fail;

		if (1 != i2c_read_timeout_us(drv->i2c, drv->addr, bytes + i, 1, false, TIMEOUT))
			goto fail;
	}

	return 0;

fail:
	printf(TAG "read register %hhx failed\n", base + i);
	return -1;
}

int es8312_identify(es8312_driver_t drv)
{
	struct CHIP chip;

	if (0 > read_regs(drv, CHIP_REG_BASE, &chip, sizeof chip))
		return -1;

	return (chip.CHIP_ID1 << 16) | (chip.CHIP_ID2 << 8) | chip.CHIP_VER;
}

int es8312_reset(es8312_driver_t drv)
{
	struct I2C iic = { .INI_REG = true };
	return_on_error(write_regs(drv, I2C_REG_BASE, &iic, sizeof iic));

	iic.INI_REG = false;
	return_on_error(write_regs(drv, I2C_REG_BASE, &iic, sizeof iic));

	return 0;
}

int es8312_start(es8312_driver_t drv)
{
	struct state state;
	return_on_error(read_regs(drv, 0, &state, sizeof state));

	/* Resets */
	state.reset.RST_DAC_DIG = false;
	state.reset.RST_ADC_DIG = false;
	state.reset.RST_MST = false;
	state.reset.RST_CMG = false;
	state.reset.RST_DIG = false;
	state.reset.SEQ_DIS = false;
	state.reset.MSC = false;
	state.reset.CSM_ON = true;

	/* Clocks */
	state.clocks.ANACLKDAC_ON = true;
	state.clocks.ANACLKADC_ON = true;
	state.clocks.CLKDAC_ON = true;
	state.clocks.CLKADC_ON = true;
	state.clocks.BCLK_ON = true;
	state.clocks.MCLK_ON = false;
	state.clocks.MCLK_INV = false;
	state.clocks.MCLK_SEL = MCLK_SEL_BCLK;
	state.clocks.DIV_PRE = 0;  /* 1× BCLK */
	state.clocks.MULT_PRE = 3; /* 8× BCLK = 8 × 1.536 = 12.288 MHz */
	state.clocks.DIV_CLKDAC = 0;
	state.clocks.DIV_CLKADC = 0;
	state.clocks.ADC_FSMODE = ADC_FSMODE_SINGLE;
	state.clocks.ADC_OSR = 32; /* 128× oversampling */
	state.clocks.DAC_OSR = 32;

	/* Serial Data Port */
	state.sdp.SDP_IN_FMT = SDP_IN_FMT_I2S;
	state.sdp.SDP_IN_WL = SDP_IN_WL_16_BIT;
	state.sdp.SDP_IN_SEL = SDP_IN_SEL_LEFT;
	state.sdp.SDP_IN_MUTE = true;
	state.sdp.SDP_OUT_FMT = SDP_OUT_FMT_I2S;
	state.sdp.SDP_OUT_WL = SDP_OUT_WL_16_BIT;
	state.sdp.SDP_OUT_MUTE = false;

	/* System */
	state.system.PWRUP_A = 0;
	state.system.PWRUP_B_HI = 0;
	state.system.PWRUP_B_LO = 0;
	state.system.PWRUP_C = 0;
	state.system.VMIDSEL = 1;
	state.system.PDN_VREF = false;
	state.system.PDN_DACVREFGEN = false;
	state.system.PDN_ADCVREFGEN = false;
	state.system.PDN_ADCBIASGEN = false;
	state.system.PDN_IBIASGEN = false;
	state.system.PDN_ANA = false;
	state.system.VROI = 0;
	state.system.RST_MOD = false;
	state.system.PDN_MOD = false;
	state.system.PDN_PGA = false;
	state.system.VX1SEL = 1;
	state.system.VX2OFF = true;
	state.system.IBIAS_SW = 0;
	state.system.DAC_IBIAS_SW = true;
	state.system.PDN_DAC = false;
	state.system.ENREFR = true;
	state.system.PGAGAIN = 0; /* Start at +0 dB */
	state.system.LINSEL = LINSEL_MIC1PN;
	state.system.ENMONOOUT = false;
	state.system.ENREFR_MONO = true;
	state.system.MONOOUT_MUTE = true;
	state.system.LD2MONOOUT = false;
	state.system.HPSW = false;

	/* ADC */
	state.adc.ADC_RAMPRATE = 2;
	state.adc.ADC_SCALE = 4;
	state.adc.ADC_RAMCLR = false;
	state.adc.ADC_SYNC = 0;
	state.adc.ADC_VOLUME = 191;
	state.adc.ALC_WINSIZE = 15;
	state.adc.ADC_AUTOMUTE_EN = false;
	state.adc.ALC_EN = false;
	state.adc.ALC_MINLEVEL = 0;
	state.adc.ALC_MAXLEVEL = 15;
	state.adc.ADC_AUTOMUTE_NG = 15;
	state.adc.ADC_AUTOMUTE_WS = 15;
	state.adc.ADC_HPFS1 = 1;
	state.adc.ADC_AUTOMUTE_VOL = 0;
	state.adc.ADC_HPFS2 = 1;
	state.adc.ADC_HPF = ADC_HPF_DYNAMIC;
	state.adc.ADC_EQBYPASS = true;

	/* DAC */
	state.dac.DAC_VOLUME = 0;
	state.dac.DAC_DSMDITH_OFF = false;
	state.dac.DAC_EQBYPASS = true;
	state.dac.DAC_RAMPRATE = 2;

	/* GPIO */
	state.gpio.GPIO_SEL = GPIO_SEL_0;
	state.gpio.ADCDAT_SEL = ADCDAT_SEL_ADC_0;
	state.gpio.ADC2DAC_SEL = false;
	state.gpio.PULLUP_SE = 0;

	return_on_error(write_regs(drv, 0, &state, sizeof state));
	return 0;
}

int es8312_set_output(es8312_driver_t drv, bool right, bool mute)
{
	struct SDP sdp;
	return_on_error(read_regs(drv, SDP_REG_BASE, &sdp, sizeof sdp));
	sdp.SDP_IN_SEL = right ? SDP_IN_SEL_RIGHT : SDP_IN_SEL_LEFT;
	sdp.SDP_IN_MUTE = mute;
	return_on_error(write_regs(drv, SDP_REG_BASE, &sdp, sizeof sdp));
	return 0;
}

int es8312_set_output_gain(es8312_driver_t drv, uint8_t gain)
{
	struct DAC dac;
	return_on_error(read_regs(drv, DAC_REG_BASE, &dac, sizeof dac));
	dac.DAC_VOLUME = gain;
	return_on_error(write_regs(drv, DAC_REG_BASE, &dac, sizeof dac));
	return 0;
}

int es8312_get_state(es8312_driver_t drv)
{
	struct FLAG flag;
	return_on_error(read_regs(drv, FLAG_REG_BASE, &flag, sizeof flag));
	return flag.FLAG_CSM_CHIP;
}

int es8312_get_automute(es8312_driver_t drv)
{
	struct FLAG flag;
	return_on_error(read_regs(drv, FLAG_REG_BASE, &flag, sizeof flag));
	return flag.FLAG_ADCAM;
}
