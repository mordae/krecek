#pragma once
#include <pico/stdlib.h>

#define clamp(X, LO, HI) \
	_Generic((X), unsigned: clampu, float: clampf, double: clampq, default: clampi)(X, LO, HI)

inline static int __unused clampi(int x, int lo, int hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static int __unused clampu(unsigned x, unsigned lo, unsigned hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static int __unused clampf(float x, float lo, float hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static int __unused clampq(double x, double lo, double hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}
