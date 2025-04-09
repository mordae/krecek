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
