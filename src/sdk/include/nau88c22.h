#pragma once
#include <pico/stdlib.h>
#include <hardware/i2c.h>

#define NAU88C22_ADDR 0x1a
#define NAU88C22_MIN_GAIN -57.0f
#define NAU88C22_MAX_GAIN 6.0f

struct nau88c22_driver {
	/* Configured I2C peripheral to use: */
	i2c_inst_t *i2c;

	/* I2C address of the device: */
	uint8_t addr;
};

typedef struct nau88c22_driver *nau88c22_driver_t;

/* Ask the device to identify itself. */
int nau88c22_identify(nau88c22_driver_t drv);

/* Reset the device. */
int nau88c22_reset(nau88c22_driver_t drv);

/* Start the device. */
int nau88c22_start(nau88c22_driver_t drv);

/*
 * Control output gain.
 *
 * - NAU88C22_MIN_GAIN = -57 dB
 * - NAU88C22_MAX_GAIN =  +6 dB
 */
int nau88c22_set_output_gain(nau88c22_driver_t drv, float gain);
