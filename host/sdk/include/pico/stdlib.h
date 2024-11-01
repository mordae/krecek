#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(__unused)
#define __unused __attribute__((__unused__))
#endif

#define PICO_NO_HARDWARE 1

uint32_t time_us_32();
uint64_t time_us_64();

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))