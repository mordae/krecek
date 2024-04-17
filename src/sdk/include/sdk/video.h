#pragma once
#include <pico/stdlib.h>

#define SDK_BACKLIGHT_MIN 2   // Something is still visible
#define SDK_BACKLIGHT_STD 128 // Default brightness
#define SDK_BACKLIGHT_MAX 256 // Maximum brightness

/*
 * Set screen backlight brightness (actually the PWM threshold).
 * Acceptable values are 0 to 256.
 *
 * SDK_BACKLIGHT_{MIN,STD,MAX} might come in handy.
 */
void sdk_set_backlight(unsigned level);
