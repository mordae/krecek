/*
 * Copyright (C) Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once
#include <pico/stdlib.h>
#include <hardware/i2c.h>

#define ES8312_ADDR 0x18
#define ES8312_NUM_REGS 70

struct es8312_driver {
	/* Configured I2C peripheral to use: */
	i2c_inst_t *i2c;

	/* I2C address of the device: */
	uint8_t addr;
};

typedef struct es8312_driver *es8312_driver_t;

/* Ask the device to identify itself. */
int es8312_identify(es8312_driver_t drv);

/* Reset the device. */
int es8312_reset(es8312_driver_t drv);

/* Start the device. */
int es8312_start(es8312_driver_t drv);

/* Control which channel is to be used for output and output mute. */
int es8312_set_output(es8312_driver_t drv, bool right, bool mute);

/* Control output gain. 191 = 0dB, 0.5dB steps, default 0. */
int es8312_set_output_gain(es8312_driver_t drv, uint8_t gain);

/* Get current chip state. */
int es8312_get_state(es8312_driver_t drv);

/* Get current automute state. */
int es8312_get_automute(es8312_driver_t drv);
