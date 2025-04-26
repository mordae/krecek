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

/* Opaque handle for playing melodies. */
struct sdk_melody;
typedef struct sdk_melody sdk_melody_t;

/*
 * Sample melody player.
 *
 * This gets done automatically inside game_audio() unless you override it.
 * If you do, you need to call it yourself as needed.
 */
void sdk_melody_sample(int16_t *left, int16_t *right);

/*
 * Start playing given melody.
 *
 * Melody is composed from 1/4 notes that can be extended up to 8/4.
 * Whole range is two octaves and sharps are supported as well.
 *
 * c c# d d# e f f# g g# a a# b C C# D D# E F F# G G# A A# B
 *
 * To extend note duration, use '-'. To insert rest, use '_'.
 * For example:
 *
 * cece g-gg cece d-dd cege dde_ cege ddc-
 *
 * Default instrument is sine wave, playing at 120 BPM.
 * These can be modified at any point. Like this:
 *
 * /i:square cece /bpm:60 /i:sine g /bpm:120 gg /i:square d-dd
 *
 * Lowercase 'a' is A4 or 440 Hz. Uppercase 'A' is A5 or 880 Hz.
 * You can use symbols '<' and '>' to shift scale down (or up) one octave.
 *
 * Volume can be adjusted by using dynamic marks that go like (from the
 * quietest to the loudest):
 *
 * (ppp) c (pp) c (p) c (mp) c (mf) c (f) c (ff) c (fff) c
 *
 * Default is (mp).
 *
 * Marks '{' and '}' establish an infinitely looping segment. Anything in
 * between will be repeated forever.
 *
 * Function returns `NULL` when there is no space for another melody.
 *
 * You have to call `sdk_melody_release()` once you are done with the melody.
 * It will keep playing until it finishes or, in case it loops, will play
 * forever. If you wish to stop the melody prematurely or just disable the
 * looping and let it finish naturally, use proper combination of the
 * procedures listed below.
 */
sdk_melody_t *sdk_melody_play_get(const char *melody);

/*
 * Shortcut to call sdk_melody_release(sdk_melody_play_get()).
 * Returns `true` if a slot was found and `false` if not.
 */
bool sdk_melody_play(const char *melody);

/*
 * Check if given melody is still playing.
 */
bool sdk_melody_is_playing(sdk_melody_t *melody);

/*
 * Stop playing given melody after current note or rest ends,
 * releasing the handle so that the slot can be reused.
 */
void sdk_melody_stop_and_release(sdk_melody_t *melody);

/*
 * Disable looping for given melody, having it end naturally.
 * You still need to call sdk_melody_release() at some point.
 */
void sdk_melody_stop_looping(sdk_melody_t *melody);

/*
 * Release the melody so that the slot can be reused.
 * Melody will continue playing until it ends.
 * If it loops, it will keep playing indefinitely.
 */
void sdk_melody_release(sdk_melody_t *melody);
