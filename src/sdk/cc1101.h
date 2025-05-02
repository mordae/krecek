#pragma once
#include <stddef.h>

/* Reset and configure the chip. */
void cc1101_init(void);

/* Begin receiving. */
void cc1101_receive(void);

/* Transmit given buffer. */
void cc1101_transmit(const void *buf, size_t len);

/* Return to idling. */
void cc1101_idle(void);

/*
 * Check if a whole packet has been received and if it was,
 * copy it to the buffer and update the length to match.
 *
 * Neither buffer nor length are modified unless `true` is returned.
 */
bool cc1101_poll(void *buf, size_t *len);
