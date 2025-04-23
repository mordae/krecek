#pragma once
#include <pico/stdlib.h>
#include <sdk/util.h>

#define SDK_GAIN_MIN -121.5f // Minimum audio gain
#define SDK_GAIN_STD 0.0f    // Default audio gain
#define SDK_GAIN_MAX 0.0f    // Maximum audio gain

/*
 * Set audio output gain (in decibels).
 * When gain is below SDK_GAIN_MIN, amplifier is turned off.
 */
void sdk_set_output_gain_db(float gain);

/* Enable output to headphones, disable speaker or the other way round. */
void sdk_enable_headphones(bool en);

/* Audio sample rate. */
#if !defined(SDK_AUDIO_RATE)
#define SDK_AUDIO_RATE 48000
#endif

/* Output a stereo audio sample. */
void sdk_write_sample(int16_t left, int16_t right);

/* Input a stereo audio sample. */
void sdk_read_sample(int16_t *left, int16_t *right);

/* Function implementing an instrument. */
typedef int16_t (*sdk_instrument_fn)(void *self, int16_t amplitude);

/* Simple audio track. */
typedef struct sdk_track {
	/* Function to call to obtain new sample. */
	sdk_instrument_fn fn;

	/* Linear panning. Negative to the left, positive to the right. */
	int16_t pan;

	/* Durations of individual parts. */
	uint32_t attack, decay, sustain, release;

	/* Base and peak volume. */
	int16_t base, peak;

	/* Current playback position. */
	uint32_t position;

	/* Next track in the player. */
	struct sdk_track *next;

	/* Sequencer this track is currently playing on. */
	struct sdk_player *player;
} sdk_track_t;

/* Play sine wave. */
int16_t sdk_play_sine(void *self, int16_t amplitude);

typedef struct sdk_track_sine {
	sdk_track_t track; /* Base track. */
	int frequency;	   /* Sine wave frequency. */
	uint32_t phase;	   /* Current phase offset. */
} sdk_track_sine_t;

/* Play square wave. */
int16_t sdk_play_square(void *self, int16_t amplitude);

typedef struct sdk_track_square {
	sdk_track_t track; /* Base track. */
	int frequency;	   /* Square wave frequency. */
	uint32_t phase;	   /* Current phase offset. */
} sdk_track_square_t;

/* Play white noise. */
int16_t sdk_play_noise(void *self, int16_t amplitude);

typedef struct sdk_track_noise {
	sdk_track_t track; /* Base track. */
} sdk_track_noise_t;

/* Simple audio player. */
typedef struct sdk_player {
	sdk_track_t *tracks;
} sdk_player_t;

/*
 * Add track to the player.
 *
 * It is not possible to add a track that is already playing.
 */
inline static void sdk_add_track(sdk_player_t *player, sdk_track_t *track)
{
	if (track->player)
		return;

	track->player = player;
	track->next = player->tracks;
	player->tracks = track;
}

/*
 * Obtain next player sample.
 *
 * Once a track finishes playing, it is automatically removed from
 * the player and its position is reset to 0.
 */
void sdk_player_sample(sdk_player_t *player, int16_t *left, int16_t *right);
