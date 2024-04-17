#pragma once
#include <pico/stdlib.h>

#include <sdk/embed.h>
#include <sdk/game.h>
#include <sdk/input.h>
#include <sdk/util.h>
#include <sdk/video.h>
#include <sdk/audio.h>

#if !defined(__noreturn)
#define __noreturn __attribute__((__noreturn__))
#endif

struct sdk_config {
	bool wait_for_usb;  // Testing: wait for USB serial connection
	bool show_fps;	    // Testing: display FPS indicator
	bool off_on_select; // Testing: turn off on SELECT button

	uint8_t fps_color;  // Color of the FPS indicator
	unsigned backlight; // Brightness (1 to 256, default 128)
};

/* Current SDK configuration. */
extern struct sdk_config sdk_config;

/*
 * Copy given configuration to sdk_config, initialize the hardware
 * and start the main loop with input, paint, audio and other tasks.
 */
void __noreturn sdk_main(const struct sdk_config *conf);

/*
 * Can be called quite often, will actually only yield after
 * given number of microseconds since the last actual yield.
 */
void sdk_yield_every_us(uint32_t us);

/* Our panic that flushes stdout. */
void __attribute__((__noreturn__, __format__(printf, 1, 2))) sdk_panic(const char *fmt, ...);
