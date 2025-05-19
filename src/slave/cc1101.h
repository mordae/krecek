#pragma once
#include <stddef.h>
#include <hardware/spi.h>

/* Reset and configure the chip. */
void cc1101_init(spi_inst_t *spi);

/* Begin receiving. */
void cc1101_begin_receive(void);

/*
 * Transmit given buffer.
 * Returns true when queued and false when not ready.
 */
bool cc1101_transmit(const void *buf, int len);

/* Return to idling and flush both FIFOs. */
void cc1101_idle(void);

/* Run calibration. */
void cc1101_calibrate(void);

/* Get current RSSI. */
float cc1101_get_rssi(void);

#define CC1101_MAXLEN 64

/*
 * Check if a whole packet has been received and if it was,
 * copy it to the buffer and return its length.
 *
 * Otherwise return a negative number.
 */
int cc1101_receive(void *buf);

/*
 * Tune to a different frequency.
 */
void cc1101_set_freq(float freq_target);
