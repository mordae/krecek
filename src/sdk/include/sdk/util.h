#pragma once
#include <pico/stdlib.h>

#define clamp(X, LO, HI)                                                                           \
	_Generic((X), unsigned: clampu, float: clampf, double: clampq, default: clampi)((X), (LO), \
											(HI))

inline static int clampi(int x, int lo, int hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static unsigned clampu(unsigned x, unsigned lo, unsigned hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static float clampf(float x, float lo, float hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

inline static double clampq(double x, double lo, double hi)
{
	return x < lo ? lo : (x > hi ? hi : x);
}

uint32_t sdk_random();

inline static int lerp(int x, int y, int a, int b)
{
	return x + ((y - x) * a) / b;
}

inline static int signi(int x)
{
	return x >= 0 ? 1 : -1;
}

inline static int signf(float x)
{
	return x >= 0 ? 1 : -1;
}

inline static int signq(double x)
{
	return x >= 0 ? 1 : -1;
}

#define sign(X) _Generic((X), float: signf, double: signq, default: signi)((X))
