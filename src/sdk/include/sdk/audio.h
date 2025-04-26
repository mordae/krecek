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
 * Melody string has following syntax:
 *
 * <instrument> <tempo> [bar-1] [misc-1] [bar-2] ... [misc-m] [bar-n]
 *
 * Where instrument is one of "sine", "square" or "noise".
 * Tempo is a number of beats per minute (BPM). Use 120 if unsure.
 *
 * Bars are little bit more involved. Base duration of a note is fourth.
 * You can use spaces freely to group notes for readability. Underscores
 * are rests and dashes make the notes last longer. For example:
 *
 * cece g-gg cece d-dd cege dde_ cege ddc-
 *
 * Lower case 'a' is A4 or 440 Hz. Upper case 'a' is A5 or 880 Hz.
 * In order to change the scales, you must use misc symbol '<' or '>'.
 *
 * '<' moves following notes an octave lower, so 'a' would be 220 Hz.
 * Using '<<' places 'a' at 110 Hz and 'A' at 220 Hz.
 *
 * '>' moves following notes an octave higher. So '>>' places 'a' at
 * whopping 1760 Hz and 'A' at 3520 Hz.
 *
 * Everything is C-major, so 'c' is C4 by default, at ~261.63 Hz,
 * sharps are available and parsed properly. So the whole range is:
 *
 * c c# d d# e f f# g g# a a# b C C# D D# E F F# G G# A A# B
 *
 * Sharps need to be close to the note, so if you want a long one,
 * use "g#-f#- g#---".
 *
 * Another pair of special symbols are '[' and ']'. These increase and
 * decrease volume by 1/5 respectively. Base melody amplitude starts at
 * roughly 1/2 of the maximum, so "[[ a--- ]]" gives us roughly 48% of
 * maximum and "[[[[ a--- ]]]]" 69% of maximum.
 *
 * There are also '(' and ')', which act identically, except in reverse.
 * The '(' lowers the volume and ')' increases it. Those are mainly for
 * those of us who dislike unbalanced parenthesis.
 *
 * And finally, there is looping. Using '{' and '}' marks part of the
 * melody that is played in an infinite loop. This looping can be stopped
 * with functions below and in most cases the melody will then proceed to
 * play out to the natural conclusion after the closing '}'.
 *
 * Returns `false` when there is no space for another melody.
 */
bool sdk_melody_play(const char *melody);

/*
 * Check if given melody is currently playing.
 *
 * This compares the melodies by pointer address, not by the contents,
 * so you need to use the very same object. Ideally keep the medies in
 * static variables and use those.
 */
bool sdk_melody_is_playing(const char *melody);

#define SDK_ALL_MELODIES (const char *)0
#define SDK_ALL_LOOPING (const char *)1
#define SDK_ALL_NOT_LOOPING (const char *)2

/*
 * Stop playing given melody, fading it out.
 *
 * There are special constants you can use here instead of a specific
 * melody. SDK_ALL_MELODIES will stop all melodies. SDK_ALL_LOOPING will
 * stop and fade out only those that are loopable, SDK_ALL_NOT_LOOPING will
 * stop and fade those that are not loopable, leaving loops alone.
 */
void sdk_melody_stop_playing(const char *melody);

/*
 * Disable looping for given melody, having it end naturally.
 *
 * If NULL, SDK_ALL_MELODIES or SDK_ALL_LOOPING is given instead of a
 * specific melody, all currently playing (looping) melodies are affected.
 * Passing SDK_ALL_NOT_LOOPING accomplishes nothing.
 */
void sdk_melody_stop_looping(const char *melody);
