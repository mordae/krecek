#pragma once
#include <stdint.h>
#include <stdio.h>

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
