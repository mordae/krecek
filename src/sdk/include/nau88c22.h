#pragma once
#include <pico/stdlib.h>
#include <hardware/i2c.h>

#define NAU88C22_ADDR 0x1a

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

/* Control output gain. */
int nau88c22_set_output_gain(nau88c22_driver_t drv, float gain);

/* Enable output to headphones. */
int nau88c22_enable_headphones(nau88c22_driver_t drv, bool en);

/* Enable output to speaker. */
int nau88c22_enable_speaker(nau88c22_driver_t drv, bool en);
