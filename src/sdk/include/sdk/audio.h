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

/* Output a stereo audio sample. */
void sdk_write_sample(int16_t left, int16_t right);

/* Input a stereo audio sample. */
void sdk_read_sample(int16_t *left, int16_t *right);
