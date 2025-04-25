#pragma once
#include <pico/stdlib.h>

#define clamp(X, LO, HI) \
	_Generic((X), unsigned: clampu, float: clampf, double: clampq, default: clampi)(X, LO, HI)

inline static int __unused clampi(int x, int lo, int hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static unsigned __unused clampu(unsigned x, unsigned lo, unsigned hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static float __unused clampf(float x, float lo, float hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static double __unused clampq(double x, double lo, double hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

uint32_t sdk_random();

inline static int lerp(int x, int y, int a, int b)
{
	return x + ((y - x) * a) / b;
}
