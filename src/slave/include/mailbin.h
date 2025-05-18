#pragma once
#include <stdint.h>

#define MAILBIN_MAGIC 0xdc56a6f7
#define MAILBIN_BASE 0x2003fc00
#define MAILBIN_STDOUT_SIZE 96

struct mailbin {
	uint32_t magic;
	uint32_t gpio_input, qspi_input;
	uint16_t adc[4];
	uint16_t touch[4];
	uint32_t stdout_head;
	char stdout_buffer[MAILBIN_STDOUT_SIZE];
};

static_assert(sizeof(struct mailbin) == 128);
