#pragma once
#include <pico/stdlib.h>

#define SDK_GAIN_MIN -95.0f // Minimum audio gain
#define SDK_GAIN_STD 0.0f   // Default audio gain
#define SDK_GAIN_MAX 30.0f  // Maximum audio gain

/*
 * Set audio output gain (in decibels).
 * When gain is below SDK_GAIN_MIN, amplifier is turned off.
 */
void sdk_set_output_gain_db(float gain);

/* Audio sample rate. */
#if !defined(SDK_AUDIO_RATE)
#define SDK_AUDIO_RATE 48000
#endif

/*
 * Output an audio sample.
 * Returns false when there was not enough space in the buffer.
 */
bool sdk_write_sample(int16_t sample);

/*
 * Input an audio sample.
 * Returns false where there was no sample in the buffer available.
 */
bool sdk_read_sample(int16_t *sample);

/* Write multiple samples, returning number of samples actually written. */
int sdk_write_samples(const int16_t *buf, int len);

/* Read multiple samples, returning number of samples actually read. */
int sdk_read_samples(int16_t *buf, int len);
