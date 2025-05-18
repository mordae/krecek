#pragma once
#include <stddef.h>
#include <hardware/spi.h>

/* Reset and configure the chip. */
void cc1101_init(spi_inst_t *spi);

/*
 * Begin receiving. Returns false when busy.
 */
bool cc1101_receive(void);

/*
 * Transmit given buffer.
 * Returns true when queued and false when not ready.
 */
bool cc1101_transmit(const void *buf, size_t len);

/* Return to idling and flush both FIFOs. */
void cc1101_idle(void);

/* Get current RSSI. */
float cc1101_get_rssi(void);

#define CC1101_MAXLEN 64

/*
 * Check if a whole packet has been received and if it was,
 * copy it to the buffer and return its length.
 *
 * Otherwise return a negative number.
 */
int cc1101_poll(void *buf);
