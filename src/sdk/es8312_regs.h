#pragma once
#include <stdint.h>
#include <stdio.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

struct RESET {
	/* 0x00 */
	uint8_t RST_DAC_DIG : 1;
	uint8_t RST_ADC_DIG : 1;
	uint8_t RST_MST : 1;
	uint8_t RST_CMG : 1;
	uint8_t RST_DIG : 1;
	uint8_t SEQ_DIS : 1;
	uint8_t MSC : 1;
	uint8_t CSM_ON : 1;
} __packed;

#define RESET_REG_BASE 0x00

inline static void __unused dump_reset(struct RESET *data)
{
	printf("RST_DAC_DIG = %hhu\n", data->RST_DAC_DIG);
	printf("RST_ADC_DIG = %hhu\n", data->RST_ADC_DIG);
	printf("RST_MST = %hhu\n", data->RST_MST);
	printf("RST_CMG = %hhu\n", data->RST_CMG);
	printf("RST_DIG = %hhu\n", data->RST_DIG);
	printf("SEQ_DIS = %hhu\n", data->SEQ_DIS);
	printf("MSC = %hhu\n", data->MSC);
	printf("CSM_ON = %hhu\n", data->CSM_ON);
}

struct CLOCKS {
	/* 0x01 */
	uint8_t ANACLKDAC_ON : 1;
	uint8_t ANACLKADC_ON : 1;
	uint8_t CLKDAC_ON : 1;
	uint8_t CLKADC_ON : 1;
	uint8_t BCLK_ON : 1;
	uint8_t MCLK_ON : 1;
	uint8_t MCLK_INV : 1;
	uint8_t MCLK_SEL : 1;
#define MCLK_SEL_MCLK 0
#define MCLK_SEL_BCLK 1

	/* 0x02 */
	uint8_t DELYSEL : 2;
	uint8_t PATHSEL : 1;
	uint8_t MULT_PRE : 2;
	uint8_t DIV_PRE : 3;

	/* 0x03 */
	uint8_t ADC_OSR : 6;
#define ADC_OSR_MIN 60
#define ADC_OSR_MAX 252
#define ADC_OSR_STEP 4
	uint8_t ADC_FSMODE : 1;
#define ADC_FSMODE_SINGLE 0
#define ADC_FSMODE_DOUBLE 1
	uint8_t : 1;

	/* 0x04 */
	uint8_t DAC_OSR : 7;
#define DAC_OSR_MIN 60
#define DAC_OSR_MAX 508
#define DAC_OSR_STEP 4
	uint8_t : 1;

	/* 0x05 */
	uint8_t DIV_CLKDAC : 4;
	uint8_t DIV_CLKADC : 4;

	/* 0x06 */
	uint8_t DIV_BCLK : 5;
	uint8_t BCLK_INV : 1;
	uint8_t BCLK_CON : 1;
	uint8_t : 1;

	/* 0x07 */
	uint8_t DIV_LRCK_HI : 4;
	uint8_t TRI_ADCDAT : 1;
	uint8_t TRI_BLRCK : 1;
	uint8_t : 2;

	/* 0x08 */
	uint8_t DIV_LRCK_LO : 8;
} __packed;

#define CLOCKS_REG_BASE 0x01

inline static void __unused dump_clocks(struct CLOCKS *data)
{
	printf("ANACLKDAC_ON = %hhu\n", data->ANACLKDAC_ON);
	printf("ANACLKADC_ON = %hhu\n", data->ANACLKADC_ON);
	printf("CLKDAC_ON = %hhu\n", data->CLKDAC_ON);
	printf("CLKADC_ON = %hhu\n", data->CLKADC_ON);
	printf("BCLK_ON = %hhu\n", data->BCLK_ON);
	printf("MCLK_ON = %hhu\n", data->MCLK_ON);
	printf("MCLK_INV = %hhu\n", data->MCLK_INV);
	printf("MCLK_SEL = %hhu\n", data->MCLK_SEL);
	printf("DELYSEL = %hhu\n", data->DELYSEL);
	printf("PATHSEL = %hhu\n", data->PATHSEL);
	printf("MULT_PRE = %hhu\n", data->MULT_PRE);
	printf("DIV_PRE = %hhu\n", data->DIV_PRE);
	printf("ADC_OSR = %hhu\n", data->ADC_OSR);
	printf("ADC_FSMODE = %hhu\n", data->ADC_FSMODE);
	printf("DAC_OSR = %hhu\n", data->DAC_OSR);
	printf("DIV_CLKDAC = %hhu\n", data->DIV_CLKDAC);
	printf("DIV_CLKADC = %hhu\n", data->DIV_CLKADC);
	printf("DIV_BCLK = %hhu\n", data->DIV_BCLK);
	printf("BCLK_INV = %hhu\n", data->BCLK_INV);
	printf("BCLK_CON = %hhu\n", data->BCLK_CON);
	printf("DIV_LRCK_HI = %hhu\n", data->DIV_LRCK_HI);
	printf("TRI_ADCDAT = %hhu\n", data->TRI_ADCDAT);
	printf("TRI_BLRCK = %hhu\n", data->TRI_BLRCK);
	printf("DIV_LRCK_LO = %hhu\n", data->DIV_LRCK_LO);
}

struct SDP {
	/* 0x09 */
	uint8_t SDP_IN_FMT : 2;
#define SDP_IN_FMT_I2S 0
#define SDP_IN_FMT_LJUST 1
#define SDP_IN_FMT_DSP_PCM 3
	uint8_t SDP_IN_WL : 3;
#define SDP_IN_WL_24_BIT 0
#define SDP_IN_WL_20_BIT 1
#define SDP_IN_WL_18_BIT 2
#define SDP_IN_WL_16_BIT 3
#define SDP_IN_WL_32_BIT 4
	uint8_t SDP_IN_LRP : 1;
	uint8_t SDP_IN_MUTE : 1;
	uint8_t SDP_IN_SEL : 1;
#define SDP_IN_SEL_LEFT 0
#define SDP_IN_SEL_RIGHT 1

	/* 0x0A */
	uint8_t SDP_OUT_FMT : 2;
#define SDP_OUT_FMT_I2S SDP_IN_FMT_I2S
#define SDP_OUT_FMT_LJUST SDP_IN_FMT_LJUST
#define SDP_OUT_FMT_DSP_PCM SDP_IN_FMT_DSP_PCM
	uint8_t SDP_OUT_WL : 3;
#define SDP_OUT_WL_24_BIT SDP_IN_WL_24_BIT
#define SDP_OUT_WL_20_BIT SDP_IN_WL_20_BIT
#define SDP_OUT_WL_18_BIT SDP_IN_WL_18_BIT
#define SDP_OUT_WL_16_BIT SDP_IN_WL_16_BIT
#define SDP_OUT_WL_32_BIT SDP_IN_WL_32_BIT
	uint8_t SDP_OUT_LRP : 1;
	uint8_t SDP_OUT_MUTE : 1;
	uint8_t : 1;
} __packed;

#define SDP_REG_BASE 0x09

inline static void __unused dump_sdp(struct SDP *data)
{
	printf("SDP_IN_FMT = %hhu\n", data->SDP_IN_FMT);
	printf("SDP_IN_WL = %hhu\n", data->SDP_IN_WL);
	printf("SDP_IN_LRP = %hhu\n", data->SDP_IN_LRP);
	printf("SDP_IN_MUTE = %hhu\n", data->SDP_IN_MUTE);
	printf("SDP_IN_SEL = %hhu\n", data->SDP_IN_SEL);
	printf("SDP_OUT_FMT = %hhu\n", data->SDP_OUT_FMT);
	printf("SDP_OUT_WL = %hhu\n", data->SDP_OUT_WL);
	printf("SDP_OUT_LRP = %hhu\n", data->SDP_OUT_LRP);
	printf("SDP_OUT_MUTE = %hhu\n", data->SDP_OUT_MUTE);
}

struct SYSTEM {
	/* 0x0B */
	uint8_t PWRUP_B_HI : 3;
	uint8_t PWRUP_A : 5;

	/* 0x0C */
	uint8_t PWRUP_C : 7;
	uint8_t PWRUP_B_LO : 1;

	/* 0x0D */
	uint8_t VMIDSEL : 2;
	uint8_t PDN_VREF : 1;
	uint8_t PDN_DACVREFGEN : 1;
	uint8_t PDN_ADCVREFGEN : 1;
	uint8_t PDN_ADCBIASGEN : 1;
	uint8_t PDN_IBIASGEN : 1;
	uint8_t PDN_ANA : 1;

	/* 0x0E */
	uint8_t : 2;
	uint8_t LPVREFBUF : 1;
	uint8_t VROI : 1;
	uint8_t RST_MOD : 1;
	uint8_t PDN_MOD : 1;
	uint8_t PDN_PGA : 1;
	uint8_t : 1;

	/* 0x0F */
	uint8_t LPADC3 : 1;
	uint8_t LPADC2 : 1;
	uint8_t LPDACVRP : 1;
	uint8_t LPADCVRP : 1;
	uint8_t LPADC1 : 1;
	uint8_t LPPGA2 : 1;
	uint8_t LPPGA1 : 1;
	uint8_t LPDAC : 1;

	/* 0x10 */
	uint8_t VX1SEL : 1;
	uint8_t VX2OFF : 1;
	uint8_t IBIAS_SW : 2;
	uint8_t DAC_IBIAS_SW : 1;
	uint8_t VMIDLOW : 2;
	uint8_t SYNCMODE : 1;

	/* 0x11 */
	uint8_t VSEL : 7;
	uint8_t : 1;

	/* 0x12 */
	uint8_t ENREFR : 1;
	uint8_t PDN_DAC : 1;
	uint8_t ENREFR_MONO : 1;
	uint8_t LPMONO : 1;
	uint8_t MONOOUT_A6DB : 1;
	uint8_t MONOOUT_MUTE : 1;
	uint8_t ENMONOOUT : 1;
	uint8_t LD2MONOOUT : 1;

	/* 0x13 */
	uint8_t : 4;
	uint8_t HPSW : 1;
	uint8_t FMSW : 1;
	uint8_t FMGAIN2 : 2;
#define FMGAIN2_12DB 0
#define FMGAIN2_6DB 1
#define FMGAIN2_0DB 2

	/* 0x14 */
	uint8_t PGAGAIN : 4;
#define PGAGAIN_MAX 30
#define PGAGAIN_STEP 3
	uint8_t LINSEL : 2;
#define LINSEL_NONE 0
#define LINSEL_MIC1PN 1
#define LINSEL_MIC2PN 2
#define LINSEL_BOTH 3
	uint8_t DMIC_ON : 1;
	uint8_t : 1;
} __packed;

#define SYSTEM_REG_BASE 0x0B

inline static void __unused dump_system(struct SYSTEM *data)
{
	printf("PWRUP_B_HI = %hhu\n", data->PWRUP_B_HI);
	printf("PWRUP_A = %hhu\n", data->PWRUP_A);
	printf("PWRUP_C = %hhu\n", data->PWRUP_C);
	printf("PWRUP_B_LO = %hhu\n", data->PWRUP_B_LO);
	printf("VMIDSEL = %hhu\n", data->VMIDSEL);
	printf("PDN_VREF = %hhu\n", data->PDN_VREF);
	printf("PDN_DACVREFGEN = %hhu\n", data->PDN_DACVREFGEN);
	printf("PDN_ADCVREFGEN = %hhu\n", data->PDN_ADCVREFGEN);
	printf("PDN_ADCBIASGEN = %hhu\n", data->PDN_ADCBIASGEN);
	printf("PDN_IBIASGEN = %hhu\n", data->PDN_IBIASGEN);
	printf("PDN_ANA = %hhu\n", data->PDN_ANA);
	printf("LPVREFBUF = %hhu\n", data->LPVREFBUF);
	printf("VROI = %hhu\n", data->VROI);
	printf("RST_MOD = %hhu\n", data->RST_MOD);
	printf("PDN_MOD = %hhu\n", data->PDN_MOD);
	printf("PDN_PGA = %hhu\n", data->PDN_PGA);
	printf("LPADC3 = %hhu\n", data->LPADC3);
	printf("LPADC2 = %hhu\n", data->LPADC2);
	printf("LPDACVRP = %hhu\n", data->LPDACVRP);
	printf("LPADCVRP = %hhu\n", data->LPADCVRP);
	printf("LPADC1 = %hhu\n", data->LPADC1);
	printf("LPPGA2 = %hhu\n", data->LPPGA2);
	printf("LPPGA1 = %hhu\n", data->LPPGA1);
	printf("LPDAC = %hhu\n", data->LPDAC);
	printf("VX1SEL = %hhu\n", data->VX1SEL);
	printf("VX2OFF = %hhu\n", data->VX2OFF);
	printf("IBIAS_SW = %hhu\n", data->IBIAS_SW);
	printf("DAC_IBIAS_SW = %hhu\n", data->DAC_IBIAS_SW);
	printf("VMIDLOW = %hhu\n", data->VMIDLOW);
	printf("SYNCMODE = %hhu\n", data->SYNCMODE);
	printf("VSEL = %hhu\n", data->VSEL);
	printf("ENREFR = %hhu\n", data->ENREFR);
	printf("PDN_DAC = %hhu\n", data->PDN_DAC);
	printf("ENREFR_MONO = %hhu\n", data->ENREFR_MONO);
	printf("LPMONO = %hhu\n", data->LPMONO);
	printf("MONOOUT_A6DB = %hhu\n", data->MONOOUT_A6DB);
	printf("MONOOUT_MUTE = %hhu\n", data->MONOOUT_MUTE);
	printf("ENMONOOUT = %hhu\n", data->ENMONOOUT);
	printf("LD2MONOOUT = %hhu\n", data->LD2MONOOUT);
	printf("HPSW = %hhu\n", data->HPSW);
	printf("FMSW = %hhu\n", data->FMSW);
	printf("FMGAIN2 = %hhu\n", data->FMGAIN2);
	printf("PGAGAIN = %hhu\n", data->PGAGAIN);
	printf("LINSEL = %hhu\n", data->LINSEL);
	printf("DMIC_ON = %hhu\n", data->DMIC_ON);
}

struct ADC {
	/* 0x15 */
	uint8_t DMIC_SENSE : 1;
	uint8_t : 3;
	uint8_t ADC_RAMPRATE : 4;

	/* 0x16 */
	uint8_t ADC_SCALE : 3;
#define ADC_SCALE_MAX 42
#define ADC_SCALE_STEP 6
#define ADC_SCALE_NORMAL 24
	uint8_t ADC_RAMCLR : 1;
	uint8_t ADC_INV : 1;
	uint8_t ADC_SYNC : 1;
	uint8_t : 2;

	/* 0x17 */
	uint8_t ADC_VOLUME : 8;

	/* 0x18 */
	uint8_t ALC_WINSIZE : 4;
	uint8_t : 2;
	uint8_t ADC_AUTOMUTE_EN : 1;
	uint8_t ALC_EN : 1;

	/* 0x19 */
	uint8_t ALC_MINLEVEL : 4;
	uint8_t ALC_MAXLEVEL : 4;

	/* 0x1A */
	uint8_t ADC_AUTOMUTE_NG : 4;
	uint8_t ADC_AUTOMUTE_WS : 4;

	/* 0x1B */
	uint8_t ADC_HPFS1 : 5;
	uint8_t ADC_AUTOMUTE_VOL : 3;

	/* 0x1C */
	uint8_t ADC_HPFS2 : 5;
	uint8_t ADC_HPF : 1;
#define ADC_HPF_STATIC 0
#define ADC_HPF_DYNAMIC 1
	uint8_t ADC_EQBYPASS : 1;
	uint8_t : 1;
} __packed;

#define ADC_REG_BASE 0x15

inline static void __unused dump_adc(struct ADC *data)
{
	printf("DMIC_SENSE = %hhu\n", data->DMIC_SENSE);
	printf("ADC_RAMPRATE = %hhu\n", data->ADC_RAMPRATE);
	printf("ADC_SCALE = %hhu\n", data->ADC_SCALE);
	printf("ADC_RAMCLR = %hhu\n", data->ADC_RAMCLR);
	printf("ADC_INV = %hhu\n", data->ADC_INV);
	printf("ADC_SYNC = %hhu\n", data->ADC_SYNC);
	printf("ADC_VOLUME = %hhu\n", data->ADC_VOLUME);
	printf("ALC_WINSIZE = %hhu\n", data->ALC_WINSIZE);
	printf("ADC_AUTOMUTE_EN = %hhu\n", data->ADC_AUTOMUTE_EN);
	printf("ALC_EN = %hhu\n", data->ALC_EN);
	printf("ALC_MINLEVEL = %hhu\n", data->ALC_MINLEVEL);
	printf("ALC_MAXLEVEL = %hhu\n", data->ALC_MAXLEVEL);
	printf("ADC_AUTOMUTE_NG = %hhu\n", data->ADC_AUTOMUTE_NG);
	printf("ADC_AUTOMUTE_WS = %hhu\n", data->ADC_AUTOMUTE_WS);
	printf("ADC_HPFS1 = %hhu\n", data->ADC_HPFS1);
	printf("ADC_AUTOMUTE_VOL = %hhu\n", data->ADC_AUTOMUTE_VOL);
	printf("ADC_HPFS2 = %hhu\n", data->ADC_HPFS2);
	printf("ADC_HPF = %hhu\n", data->ADC_HPF);
	printf("ADC_EQBYPASS = %hhu\n", data->ADC_EQBYPASS);
}

struct ADCEQ {
	/* 0x1D - 0x20 */
	uint8_t ADCEQ_B0_3 : 8;
	uint8_t ADCEQ_B0_2 : 8;
	uint8_t ADCEQ_B0_1 : 8;
	uint8_t ADCEQ_B0_0 : 8;

	/* 0x21 - 0x24 */
	uint8_t ADCEQ_A1_3 : 8;
	uint8_t ADCEQ_A1_2 : 8;
	uint8_t ADCEQ_A1_1 : 8;
	uint8_t ADCEQ_A1_0 : 8;

	/* 0x25 - 0x28 */
	uint8_t ADCEQ_A2_3 : 8;
	uint8_t ADCEQ_A2_2 : 8;
	uint8_t ADCEQ_A2_1 : 8;
	uint8_t ADCEQ_A2_0 : 8;

	/* 0x29 - 0x2C */
	uint8_t ADCEQ_B1_3 : 8;
	uint8_t ADCEQ_B1_2 : 8;
	uint8_t ADCEQ_B1_1 : 8;
	uint8_t ADCEQ_B1_0 : 8;

	/* 0x2D - 0x30 */
	uint8_t ADCEQ_B2_3 : 8;
	uint8_t ADCEQ_B2_2 : 8;
	uint8_t ADCEQ_B2_1 : 8;
	uint8_t ADCEQ_B2_0 : 8;
} __packed;

#define ADCEQ_REG_BASE 0x1D

inline static void __unused dump_adceq(struct ADCEQ *data)
{
	printf("ADCEQ_B0_3 = %hhu\n", data->ADCEQ_B0_3);
	printf("ADCEQ_B0_2 = %hhu\n", data->ADCEQ_B0_2);
	printf("ADCEQ_B0_1 = %hhu\n", data->ADCEQ_B0_1);
	printf("ADCEQ_B0_0 = %hhu\n", data->ADCEQ_B0_0);
	printf("ADCEQ_A1_3 = %hhu\n", data->ADCEQ_A1_3);
	printf("ADCEQ_A1_2 = %hhu\n", data->ADCEQ_A1_2);
	printf("ADCEQ_A1_1 = %hhu\n", data->ADCEQ_A1_1);
	printf("ADCEQ_A1_0 = %hhu\n", data->ADCEQ_A1_0);
	printf("ADCEQ_A2_3 = %hhu\n", data->ADCEQ_A2_3);
	printf("ADCEQ_A2_2 = %hhu\n", data->ADCEQ_A2_2);
	printf("ADCEQ_A2_1 = %hhu\n", data->ADCEQ_A2_1);
	printf("ADCEQ_A2_0 = %hhu\n", data->ADCEQ_A2_0);
	printf("ADCEQ_B1_3 = %hhu\n", data->ADCEQ_B1_3);
	printf("ADCEQ_B1_2 = %hhu\n", data->ADCEQ_B1_2);
	printf("ADCEQ_B1_1 = %hhu\n", data->ADCEQ_B1_1);
	printf("ADCEQ_B1_0 = %hhu\n", data->ADCEQ_B1_0);
	printf("ADCEQ_B2_3 = %hhu\n", data->ADCEQ_B2_3);
	printf("ADCEQ_B2_2 = %hhu\n", data->ADCEQ_B2_2);
	printf("ADCEQ_B2_1 = %hhu\n", data->ADCEQ_B2_1);
	printf("ADCEQ_B2_0 = %hhu\n", data->ADCEQ_B2_0);
}

struct DAC {
	/* 0x31 */
	uint8_t : 2;
	uint8_t DAC_DSMDITH_OFF : 1;
	uint8_t DAC_RAMCLR : 1;
	uint8_t DAC_INV : 1;
	uint8_t : 3;

	/* 0x32 */
	uint8_t DAC_VOLUME : 8;

	/* 0x33 */
	uint8_t DAC_OFFSET : 8;

	/* 0x34 */
	uint8_t DRC_WINSIZE : 4;
	uint8_t : 3;
	uint8_t DRC_EN : 1;

	/* 0x35 */
	uint8_t DRC_MINLEVEL : 4;
	uint8_t DRC_MAXLEVEL : 4;

	/* 0x36 */
	uint8_t : 8;

	/* 0x37 */
	uint8_t : 3;
	uint8_t DAC_EQBYPASS : 1;
	uint8_t DAC_RAMPRATE : 4;
} __packed;

#define DAC_REG_BASE 0x31

inline static void __unused dump_dac(struct DAC *data)
{
	printf("DAC_DSMDITH_OFF = %hhu\n", data->DAC_DSMDITH_OFF);
	printf("DAC_RAMCLR = %hhu\n", data->DAC_RAMCLR);
	printf("DAC_INV = %hhu\n", data->DAC_INV);
	printf("DAC_VOLUME = %hhu\n", data->DAC_VOLUME);
	printf("DAC_OFFSET = %hhu\n", data->DAC_OFFSET);
	printf("DRC_WINSIZE = %hhu\n", data->DRC_WINSIZE);
	printf("DRC_EN = %hhu\n", data->DRC_EN);
	printf("DRC_MINLEVEL = %hhu\n", data->DRC_MINLEVEL);
	printf("DRC_MAXLEVEL = %hhu\n", data->DRC_MAXLEVEL);
	printf("DAC_EQBYPASS = %hhu\n", data->DAC_EQBYPASS);
	printf("DAC_RAMPRATE = %hhu\n", data->DAC_RAMPRATE);
}

struct DACEQ {
	/* 0x38 - 0x3B */
	uint8_t DACEQ_B0_3 : 8;
	uint8_t DACEQ_B0_2 : 8;
	uint8_t DACEQ_B0_1 : 8;
	uint8_t DACEQ_B0_0 : 8;

	/* 0x3C - 0x3F */
	uint8_t DACEQ_B1_3 : 8;
	uint8_t DACEQ_B1_2 : 8;
	uint8_t DACEQ_B1_1 : 8;
	uint8_t DACEQ_B1_0 : 8;

	/* 0x40 - 0x43 */
	uint8_t DACEQ_A1_3 : 8;
	uint8_t DACEQ_A1_2 : 8;
	uint8_t DACEQ_A1_1 : 8;
	uint8_t DACEQ_A1_0 : 8;
} __packed;

#define DACEQ_REG_BASE 0x38

inline static void __unused dump_daceq(struct DACEQ *data)
{
	printf("DACEQ_B0_3 = %hhu\n", data->DACEQ_B0_3);
	printf("DACEQ_B0_2 = %hhu\n", data->DACEQ_B0_2);
	printf("DACEQ_B0_1 = %hhu\n", data->DACEQ_B0_1);
	printf("DACEQ_B0_0 = %hhu\n", data->DACEQ_B0_0);
	printf("DACEQ_B1_3 = %hhu\n", data->DACEQ_B1_3);
	printf("DACEQ_B1_2 = %hhu\n", data->DACEQ_B1_2);
	printf("DACEQ_B1_1 = %hhu\n", data->DACEQ_B1_1);
	printf("DACEQ_B1_0 = %hhu\n", data->DACEQ_B1_0);
	printf("DACEQ_A1_3 = %hhu\n", data->DACEQ_A1_3);
	printf("DACEQ_A1_2 = %hhu\n", data->DACEQ_A1_2);
	printf("DACEQ_A1_1 = %hhu\n", data->DACEQ_A1_1);
	printf("DACEQ_A1_0 = %hhu\n", data->DACEQ_A1_0);
}

struct GPIO {
	/* 0x44 */
	uint8_t GPIO_SEL : 3;
#define GPIO_SEL_0 0
#define GPIO_SEL_DMIC_CLK 1
#define GPIO_SEL_ADC_MUTE_OUT 3
#define GPIO_SEL_1 4
#define GPIO_SEL_INV_DMIC_CLK 5
#define GPIO_SEL_INV_ADC_MUTE_OUT 7
	uint8_t : 1;
	uint8_t ADCDAT_SEL : 3;
#define ADCDAT_SEL_ADC_ADC 0
#define ADCDAT_SEL_ADC_0 1
#define ADCDAT_SEL_0_ADC 2
#define ADCDAT_SEL_0_0 3
#define ADCDAT_SEL_DACL_ADC 4
#define ADCDAT_SEL_ADC_DACR 5
#define ADCDAT_SEL_DACL_DACR 6
	uint8_t ADC2DAC_SEL : 1;

	/* 0x45 */
	uint8_t PULLUP_SE : 1;
	uint8_t : 7;
} __packed;

#define GPIO_REG_BASE 0x44

inline static void __unused dump_gpio(struct GPIO *data)
{
	printf("GPIO_SEL = %hhu\n", data->GPIO_SEL);
	printf("ADCDAT_SEL = %hhu\n", data->ADCDAT_SEL);
	printf("ADC2DAC_SEL = %hhu\n", data->ADC2DAC_SEL);
	printf("PULLUP_SE = %hhu\n", data->PULLUP_SE);
}

struct I2C {
	/* 0xFA */
	uint8_t INI_REG : 1;
	uint8_t : 7;
} __packed;

#define I2C_REG_BASE 0xFA

inline static void __unused dump_i2c(struct I2C *data)
{
	printf("INI_REG = %hhu\n", data->INI_REG);
}

struct FLAG {
	/* 0xFC */
	uint8_t : 1;
	uint8_t FLAG_ADCAM : 1;
	uint8_t : 2;
	uint8_t FLAG_CSM_CHIP : 3;
	uint8_t : 1;
} __packed;

#define FLAG_REG_BASE 0xFC

inline static void __unused dump_flag(struct FLAG *data)
{
	printf("FLAG_ADCAM = %hhu\n", data->FLAG_ADCAM);
	printf("FLAG_CSM_CHIP = %hhu\n", data->FLAG_CSM_CHIP);
}

struct CHIP {
	/* 0xFD - 0xFF */
	uint8_t CHIP_ID1 : 8;
	uint8_t CHIP_ID2 : 8;
	uint8_t CHIP_VER : 8;
} __packed;

#define CHIP_REG_BASE 0xFD

inline static void __unused dump_chip(struct CHIP *data)
{
	printf("CHIP_ID1 = %hhu\n", data->CHIP_ID1);
	printf("CHIP_ID2 = %hhu\n", data->CHIP_ID2);
	printf("CHIP_VER = %hhu\n", data->CHIP_VER);
}
