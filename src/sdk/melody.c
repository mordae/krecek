#include <sdk/audio.h>
#include <sdk/melody-lexer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(KRECEK)
#include <pico/mutex.h>
auto_init_mutex(freelist_mutex);
auto_init_mutex(pending_mutex);
#else
static int freelist_mutex = 0;
static int pending_mutex = 0;
#endif

typedef int (*synth_fn)(uint32_t position, uint32_t step);

static int synth_sine(uint32_t position, uint32_t step);
static int synth_square(uint32_t position, uint32_t step);
static int synth_phi(uint32_t position, uint32_t step);
static int synth_noise(uint32_t position, uint32_t step);
static int synth_prnl(uint32_t position, uint32_t step);
static int synth_flute(uint32_t position, uint32_t step);

static synth_fn instruments[] = {
	synth_sine, synth_square, synth_noise, synth_phi, synth_prnl, synth_flute,
};

#define BASE_VOLUME 3
#define NUM_VOLUMES 8
static int16_t volumes[NUM_VOLUMES] = { 2920, 4125, 5827, 8231, 11626, 16422, 23197, 32767 };

/* Phase increment for 32b NCO stepping at SDK_AUDIO_RATE to wrap after 1 second. */
#define HZ (uint32_t)((1ull << 32) / SDK_AUDIO_RATE)

#define NUM_TONES 24
#define BASE_OCTAVE 5
static const uint32_t tones[NUM_TONES] = {
	261.6256 * 32 * HZ, // C4
	277.1826 * 32 * HZ, // C#4
	293.6648 * 32 * HZ, // D4
	311.1270 * 32 * HZ, // D#4
	329.6276 * 32 * HZ, // E4
	349.2282 * 32 * HZ, // F4
	369.9944 * 32 * HZ, // F#4
	391.9954 * 32 * HZ, // G4
	415.3047 * 32 * HZ, // G#4
	440.0000 * 32 * HZ, // A4
	466.1638 * 32 * HZ, // A#4
	493.8833 * 32 * HZ, // B4

	261.6256 * 64 * HZ, // C5
	277.1826 * 64 * HZ, // C#5
	293.6648 * 64 * HZ, // D5
	311.1270 * 64 * HZ, // D#5
	329.6276 * 64 * HZ, // E5
	349.2282 * 64 * HZ, // F5
	369.9944 * 64 * HZ, // F#5
	391.9954 * 64 * HZ, // G5
	415.3047 * 64 * HZ, // G#5
	440.0000 * 64 * HZ, // A5
	466.1638 * 64 * HZ, // A#5
	493.8833 * 64 * HZ, // B5
};

struct sdk_melody {
	struct sdk_melody *next; /* Next melody in either active or free list. */

	const char *melody; /* Whole melody string. */
	const char *cursor; /* Cursor into the melody to play next. */
	const char *repeat; /* Cursor to return to on '}'. */

	uint16_t duration; /* Ticks per quarter note. */
	uint16_t rest;	   /* Remaining rest time. */
	uint8_t octave;	   /* Current octave as shift right argument. */
	uint8_t volume;	   /* Current volume table index. */
	uint8_t panning;   /* Panning from 0 (L) through 3 (C) to 6 (R). */

	bool released; /* Has been released and can be recycled. */

	synth_fn synth;

	struct {
		uint16_t attack, decay, sustain, release;
		uint32_t position;
		uint32_t step;
	} note;
};

#define MAX_MELODIES 16
static sdk_melody_t melodies[MAX_MELODIES] = {
	{ .next = &melodies[1] },  { .next = &melodies[2] },
	{ .next = &melodies[3] },  { .next = &melodies[4] },
	{ .next = &melodies[5] },  { .next = &melodies[6] },
	{ .next = &melodies[7] },  { .next = &melodies[8] },
	{ .next = &melodies[9] },  { .next = &melodies[10] },
	{ .next = &melodies[11] }, { .next = &melodies[12] },
	{ .next = &melodies[13] }, { .next = &melodies[14] },
	{ .next = &melodies[15] }, {},
};

static sdk_melody_t *playing = NULL;
static sdk_melody_t *freelist = &melodies[0];
static sdk_melody_t *pending = NULL;

static const sdk_melody_t template = {
	.octave = BASE_OCTAVE,
	.volume = BASE_VOLUME,
	.duration = SDK_AUDIO_RATE * 15 / 120,
	.synth = synth_sine,
	.panning = 3,
};

static void lock_freelist(void)
{
#if defined(KRECEK)
	mutex_enter_blocking(&freelist_mutex);
#else
	freelist_mutex++;

	if (freelist_mutex >= 2)
		printf("freelist_mutex = %i\n", freelist_mutex);
#endif
}

static void unlock_freelist(void)
{
#if defined(KRECEK)
	mutex_exit(&freelist_mutex);
#else
	freelist_mutex--;

	if (freelist_mutex < 0)
		printf("freelist_mutex = %i\n", freelist_mutex);
#endif
}

static void lock_pending(void)
{
#if defined(KRECEK)
	mutex_enter_blocking(&pending_mutex);
#else
	pending_mutex++;

	if (pending_mutex >= 2)
		printf("pending_mutex = %i\n", pending_mutex);
#endif
}

static void unlock_pending(void)
{
#if defined(KRECEK)
	mutex_exit(&pending_mutex);
#else
	pending_mutex--;

	if (pending_mutex < 0)
		printf("pending_mutex = %i\n", pending_mutex);
#endif
}

static void advance(sdk_melody_t *melody)
{
	bool empty_loop = false;

	while (true) {
		sdk_melody_token_t token = sdk_melody_lex(&melody->cursor);

		switch (token.type) {
		case SDK_MELODY_TOKEN_ERROR:
			printf("melody: invalid token: %s", melody->cursor);
			melody->melody = NULL;
			return;

		case SDK_MELODY_TOKEN_END:
			melody->melody = NULL;
			return;

		case SDK_MELODY_TOKEN_BPM:
			melody->duration = SDK_AUDIO_RATE * 15 / MAX(15, token.bpm);
			break;

		case SDK_MELODY_TOKEN_DYNAMIC:
			melody->volume = token.dynamic;
			break;

		case SDK_MELODY_TOKEN_OCTAVE_DOWN:
			melody->octave = MIN(melody->octave + 1, 10);
			break;

		case SDK_MELODY_TOKEN_OCTAVE_UP:
			melody->octave = MAX(melody->octave - 1, 0);
			break;

		case SDK_MELODY_TOKEN_INSTRUMENT:
			melody->synth = instruments[token.instrument];
			break;

		case SDK_MELODY_TOKEN_PANNING:
			melody->panning = token.panning;
			break;

		case SDK_MELODY_TOKEN_LOOP_START:
			break;

		case SDK_MELODY_TOKEN_LOOP_END:
			if (melody->repeat) {
				if (empty_loop) {
					printf("melody: empty loop: %s", melody->repeat);
					melody->melody = NULL;
					return;
				}

				empty_loop = true;
				melody->cursor = melody->repeat;
			}
			break;

		case SDK_MELODY_TOKEN_REST:
			melody->rest = melody->duration * token.length;
			return;

		case SDK_MELODY_TOKEN_NOTE:
			int jiffy = melody->duration >> 3;
			melody->note.attack = jiffy;
			melody->note.decay = jiffy;
			melody->note.sustain = melody->duration * token.length - 4 * jiffy;
			melody->note.release = 2 * jiffy;
			melody->note.position = 0;
			melody->note.step = tones[token.note] >> melody->octave;
			melody->rest = 0;
			return;
		}
	}
}

static void recycle(sdk_melody_t *slot)
{
	lock_freelist();
	slot->next = freelist;
	freelist = slot;
	unlock_freelist();
}

sdk_melody_t *sdk_melody_play_get(const char *melody)
{
	if (!freelist) {
		printf("melody: no free slots\n");
		return NULL;
	}

	lock_freelist();
	sdk_melody_t *slot = freelist;
	freelist = freelist->next;
	unlock_freelist();

	*slot = template;
	slot->melody = melody;
	slot->cursor = melody;
	slot->repeat = strchr(melody, '{');

	lock_pending();
	slot->next = pending;
	pending = slot;
	unlock_pending();

	return slot;
}

bool sdk_melody_play(const char *melody)
{
	sdk_melody_t *slot = sdk_melody_play_get(melody);

	if (!slot)
		return false;

	sdk_melody_release(slot);
	return true;
}

bool sdk_melody_is_playing(sdk_melody_t *melody)
{
	if (!melody)
		return false;

	return !!melody->melody;
}

void sdk_melody_stop_and_release(sdk_melody_t *melody)
{
	if (!melody)
		return;

	melody->repeat = NULL;
	melody->melody = "";
	melody->cursor = "";
	melody->released = true;
}

void sdk_melody_stop_looping(sdk_melody_t *melody)
{
	if (!melody)
		return;

	melody->repeat = NULL;
}

void sdk_melody_release(sdk_melody_t *melody)
{
	if (!melody)
		return;

	melody->released = true;
}

inline static int fakesin(uint32_t phase)
{
	int x = (int)phase >> 16;
	return x - ((x * abs(x)) >> 15);
}

static int synth_sine(uint32_t position, uint32_t step)
{
	return fakesin(position * step) << 2;
}

static int synth_flute(uint32_t position, uint32_t step)
{
	float y0 = fakesin(position * step) << 1;
	float y1 = fakesin(position * step * 2 - (1 << 31)) << 1;
	return y0 + y1;
}

static int synth_square(uint32_t position, uint32_t step)
{
	return (int)(position * step) >= 0 ? INT16_MAX : -INT16_MAX;
}

inline static int16_t pseudorandom_noise(uint32_t position)
{
	// Multiply position with 1/pi, because why not.
	uint32_t value = 0x517cc1b7u * position;

	// Stir the bits around like xorshift32 does.
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;

	// Make top bits more uniformly random via 1/phi.
	value *= 0x9e3779b9u;

	// Return those noisy bits.
	return value >> 16;
}

static int synth_prnl(uint32_t position, uint32_t step)
{
	int period = UINT32_MAX / step;
	return pseudorandom_noise(position % period);
}

static int synth_noise(uint32_t position, uint32_t step)
{
	(void)step;
	return pseudorandom_noise(position);
}

static int synth_phi(uint32_t position, uint32_t step)
{
	return (0x9e3779b9u * (position * step)) >> 16;
}

void sdk_melody_sample(int16_t *left, int16_t *right)
{
	int accleft = 0;
	int accright = 0;

	if (pending) {
		lock_pending();

		while (pending) {
			sdk_melody_t *next = pending->next;
			pending->next = playing;
			playing = pending;
			pending = next;
		}

		unlock_pending();
	}

	sdk_melody_t **ptr = &playing;

	for (sdk_melody_t *iter = playing; iter; iter = iter->next) {
next:
		if (!iter->melody) {
			if (!iter->released)
				continue;

			sdk_melody_t *slot = iter;
			iter = iter->next;
			*ptr = iter;

			recycle(slot);

			if (iter) {
				goto next;
			} else {
				break;
			}
		}

		ptr = &iter->next;

		if (iter->rest > 0) {
			iter->rest--;

			if (!iter->rest)
				advance(iter);

			continue;
		}

		int16_t amplitude;

		uint32_t offset = iter->note.position;
		uint32_t milestone = iter->note.attack;

		int peak = volumes[iter->volume];
		int base = (3 * peak) >> 2;

		if (iter->note.position < milestone) {
			amplitude = lerp(0, peak, offset, iter->note.attack);
			goto sample;
		}

		offset -= iter->note.attack;
		milestone += iter->note.decay;

		if (iter->note.position < milestone) {
			amplitude = lerp(peak, base, offset, iter->note.decay);
			goto sample;
		}

		offset -= iter->note.decay;
		milestone += iter->note.sustain;

		if (iter->note.position < milestone) {
			amplitude = base;
			goto sample;
		}

		offset -= iter->note.sustain;
		milestone += iter->note.release;

		if (iter->note.position < milestone) {
			amplitude = lerp(base, 0, offset, iter->note.release);
			goto sample;
		}

		// Note is finished playing.
		advance(iter);
		continue;

sample:
		int sample = iter->synth(iter->note.position++, iter->note.step);
		sample = (sample * amplitude) >> 16;

		accleft += lerp(sample, 0, iter->panning, 6);
		accright += lerp(0, sample, iter->panning, 6);
	}

	*left = clamp(accleft, INT16_MIN, INT16_MAX);
	*right = clamp(accright, INT16_MIN, INT16_MAX);
}
