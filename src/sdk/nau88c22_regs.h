#pragma once
#include <stdint.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

#define RESET_REG_ADDR 0x00
struct Reset {
	uint16_t RESET : 9;
	uint16_t addr;
} __packed;

#define POWER_MANAGEMENT_1_ADDR 0x01
struct PowerManagement1 {
	uint16_t REFIMP : 2;
	uint16_t IOBUFEN : 1;
	uint16_t ABIASEN : 1;
	uint16_t MICBIASEN : 1;
	uint16_t PLLEN : 1;
	uint16_t AUX2MXEN : 1;
	uint16_t AUX1MXEN : 1;
	uint16_t DCBUFEN : 1;
	uint16_t addr : 7;
};

#define POWER_MANAGEMENT_2_ADDR 0x02
struct PowerManagement2 {
	uint16_t LADCEN : 1;
	uint16_t RADCEN : 1;
	uint16_t LPGAEN : 1;
	uint16_t RPGAEN : 1;
	uint16_t LBSTEN : 1;
	uint16_t RBSTEN : 1;
	uint16_t SLEEP : 1;
	uint16_t NHPEN : 1;
	uint16_t RHPEN : 1;
	uint16_t addr : 7;
};

#define POWER_MANAGEMENT_3_ADDR 0x03
struct PowerManagement3 {
	uint16_t LDACEN : 1;
	uint16_t RDACEN : 1;
	uint16_t LMIXEN : 1;
	uint16_t RMIXEN : 1;
	uint16_t reserved1 : 1;
	uint16_t RSPKEN : 1;
	uint16_t LSPKEN : 1;
	uint16_t AUXOUT2EN : 1;
	uint16_t AUXOUT1EN : 1;
	uint16_t addr : 7;
};

#define AUDIO_INTERFACE_ADDR 0x04
struct AudioInterface {
	uint16_t MONO : 1;
	uint16_t ADCPHS : 1;
	uint16_t DACPHS : 1;
	uint16_t AIFMT : 2;
	uint16_t WLEN : 2;
	uint16_t LRP : 1;
	uint16_t BCLKP : 1;
	uint16_t addr : 7;
};

#define COMPANDING_ADDR 0x05
struct Companding {
	uint16_t ADDAP : 1;
	uint16_t ADCCM : 2;
	uint16_t DACCM : 2;
	uint16_t CMB8 : 1;
	uint16_t : 3;
	uint16_t addr : 7;
};

#define CLOCK_CONTROL_1_ADDR 0x06
struct ClockControl1 {
	uint16_t CLKIOEN : 1;
	uint16_t : 1;
	uint16_t BCLKSEL : 3;
	uint16_t MCLKSEL : 3;
	uint16_t CLKM : 1;
	uint16_t addr : 7;
};

#define CLOCK_CONTROL_2_ADDR 0x07
struct ClockControl2 {
	uint16_t SCLKEN : 1;
	uint16_t SMPLR : 3;
	uint16_t : 4;
	uint16_t _4WSPIEN : 1;
	uint16_t addr : 7;
};

#define GPIO_ADDR 0x08
struct GPIO {
	uint16_t GPIO1SEL : 3;
	uint16_t GPIO1PL : 1;
	uint16_t GPIO1PLL : 2;
	uint16_t : 3;
	uint16_t addr : 7;
};

#define JACK_DETECT_1_ADDR 0x09
struct JackDetect1 {
	uint16_t : 4;
	uint16_t JCKDIO : 2;
	uint16_t JCKDEN : 1;
	uint16_t JCKMIDEN : 2;
	uint16_t addr : 7;
};

#define DAC_CONTROL_ADDR 0x0a
struct DACControl {
	uint16_t LDACPL : 1;
	uint16_t RDACPL : 1;
	uint16_t AUTOMT : 1;
	uint16_t DACOS : 1;
	uint16_t : 2;
	uint16_t SOFTMT : 1;
	uint16_t : 2;
	uint16_t addr : 7;
};

#define LEFT_DAC_VOLUME_ADDR 0x0b
struct LeftDACVolume {
	uint16_t LDACGAIN : 8;
	uint16_t LDACVU : 1;
	uint16_t addr : 7;
};

#define RIGHT_DAC_VOLUME_ADDR 0x0c
struct RightDACVolume {
	uint16_t RDACGAIN : 8;
	uint16_t RDACVU : 1;
	uint16_t addr : 7;
};

#define JACK_DETECT_2_ADDR 0x0d
struct JackDetect2 {
	uint16_t JCKDOEN0 : 4;
	uint16_t JCKDOEN1 : 4;
	uint16_t : 1;
	uint16_t addr : 7;
};

#define ADC_CONTROL_ADDR 0x0e
struct ADCControl {
	uint16_t LADCPL : 1;
	uint16_t RADCPL : 1;
	uint16_t : 1;
	uint16_t ADCOS : 1;
	uint16_t HPF : 3;
	uint16_t HPFAM : 1;
	uint16_t HPFEN : 1;
	uint16_t addr : 7;
};

#define LEFT_ADC_VOLUME_ADDR 0x0f
struct LeftADCVolume {
	uint16_t LADCGAIN : 8;
	uint16_t LADCVU : 1;
	uint16_t addr : 7;
};

#define RIGHT_ADC_VOLUME_ADDR 0x10
struct RightADCVolume {
	uint16_t RADCGAIN : 8;
	uint16_t RADCVU : 1;
	uint16_t addr : 7;
};

#define EQ1_LOW_CUTOFF_ADDR 0x12
struct EQ1LowCutoff {
	uint16_t EQ1GC : 5;
	uint16_t EQ1CF : 2;
	uint16_t : 1;
	uint16_t EQM : 1;
	uint16_t addr : 7;
};

#define EQ2_PEAK_1_ADDR 0x13
struct EQ2Peak1 {
	uint16_t EQ2GC : 5;
	uint16_t EQ2CF : 2;
	uint16_t : 1;
	uint16_t EQ2BW : 1;
	uint16_t addr : 7;
};

#define EQ3_PEAK_2_ADDR 0x14
struct EQ3Peak2 {
	uint16_t EQ3GC : 5;
	uint16_t EQ3CF : 3;
	uint16_t : 1;
	uint16_t EQ3BW : 1;
	uint16_t addr : 7;
};

#define EQ4_PEAK_3_ADDR 0x15
struct EQ4Peak3 {
	uint16_t EQ4GC : 5;
	uint16_t EQ4CF : 3;
	uint16_t : 1;
	uint16_t EQ4BW : 1;
	uint16_t addr : 7;
};

#define EQ5_HIGH_CUTOFF_ADDR 0x16
struct EQ5HighCutoff {
	uint16_t EQ5GC : 5;
	uint16_t EQ5CF : 3;
	uint16_t : 2;
	uint16_t addr : 7;
};

#define DAC_LIMITER_1_ADDR 0x18
struct DACLimiter1 {
	uint16_t DACLIMATK : 4;
	uint16_t DACLIMDCY : 4;
	uint16_t DACLIMEN : 1;
	uint16_t addr : 7;
};

#define DAC_LIMITER_2_ADDR 0x19
struct DACLimiter2 {
	uint16_t DACLIMBST : 4;
	uint16_t DACLIMTHL : 3;
	uint16_t : 2;
	uint16_t addr : 7;
};

#define NOTCH_FILTER_1_ADDR 0x1b
struct NotchFilter1 {
	uint16_t NFCA0 : 7;
	uint16_t NFCEN : 1;
	uint16_t NFCU1 : 1;
	uint16_t addr : 7;
};

#define NOTCH_FILTER_2_ADDR 0x1c
struct NotchFilter2 {
	uint16_t NFCA0 : 7;
	uint16_t : 1;
	uint16_t NFCU2 : 1;
	uint16_t addr : 7;
};

#define NOTCH_FILTER_3_ADDR 0x1d
struct NotchFilter3 {
	uint16_t NFCA1 : 7;
	uint16_t : 1;
	uint16_t NFCU3 : 1;
	uint16_t addr : 7;
};

#define NOTCH_FILTER_4_ADDR 0x1e
struct NotchFilter4 {
	uint16_t NFCA1 : 7;
	uint16_t : 1;
	uint16_t NFCU4 : 1;
	uint16_t addr : 7;
};

#define ALC_CONTROL_1_ADDR 0x20
struct ALCControl1 {
	uint16_t ALCMNGAIN : 3;
	uint16_t ALCMXGAIN : 3;
	uint16_t : 1;
	uint16_t ALCEN : 2;
	uint16_t addr : 7;
};

#define ALC_CONTROL_2_ADDR 0x21
struct ALCControl2 {
	uint16_t ALCSL : 4;
	uint16_t ALCHT : 4;
	uint16_t : 1;
	uint16_t addr : 7;
};

#define ALC_CONTROL_3_ADDR 0x22
struct ALCControl3 {
	uint16_t ALCATK : 4;
	uint16_t ALCDCY : 4;
	uint16_t ALCM : 1;
	uint16_t addr : 7;
};

#define NOISE_GATE_ADDR 0x23
struct NoiseGate {
	uint16_t ALCNTH : 3;
	uint16_t ALCNEN : 1;
	uint16_t : 5;
	uint16_t addr : 7;
};

#define PLL_N_ADDR 0x24
struct PLLN {
	uint16_t PLLN : 4;
	uint16_t PLLMCLK : 1;
	uint16_t : 4;
	uint16_t addr : 7;
};

#define PLL_K1_ADDR 0x25
struct PLLK1 {
	uint16_t PLLK : 6;
	uint16_t : 9;
	uint16_t addr : 7;
};

#define PLL_K2_ADDR 0x26
struct PLLK2 {
	uint16_t PLLK : 9;
	uint16_t addr : 7;
};

#define PLL_K3_ADDR 0x27
struct PLLK3 {
	uint16_t PLLK : 9;
	uint16_t addr : 7;
};
