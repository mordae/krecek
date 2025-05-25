#pragma once
#include <stdint.h>

/* Multiply color by 0-32/32 without checking for overflow. */
static inline uint16_t color_mul(uint16_t color, uint8_t amount)
{
	const uint32_t mask = 0b00000111111000001111100000011111;

	// Widen and introduce 5 bits of separation.
	uint32_t tmp = ((color << 16) | color) & mask;

	// Multiply in parallel, masking the spillage off.
	tmp = ((tmp * amount) >> 5) & mask;

	// Join back together.
	return (tmp >> 16) | ((tmp << 16) >> 16);
}

/* Add two colors without checking for overflow. */
static inline uint16_t color_add(uint16_t a, uint16_t b)
{
	const uint32_t mask = 0b00000111111000001111100000011111;

	uint32_t tmp_a = ((a << 16) | a) & mask;
	uint32_t tmp_b = ((b << 16) | b) & mask;

	// Add in parallel.
	uint32_t tmp = (tmp_a + tmp_b) & mask;

	// Join back together.
	return (tmp >> 16) | ((tmp << 16) >> 16);
}

/* Blend two colors in 0-32/32 ratio. */
static inline uint16_t color_blend(uint16_t a, uint16_t b, uint8_t factor)
{
	a = color_mul(a, 32 - factor);
	b = color_mul(b, factor);
	return color_add(a, b);
}
