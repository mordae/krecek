#pragma once
#include <pico/stdlib.h>

/* Read from secondary RP2040's memory. */
uint32_t sdk_peek(uint32_t addr);

/* Write to secondary RP2040's memory. */
void sdk_poke(uint32_t addr, uint32_t value);
