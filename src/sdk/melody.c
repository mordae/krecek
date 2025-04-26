#include <math.h>
#include <sdk/audio.h>
#include <sdk/melody-lexer.h>
#include <stdio.h>
#include <string.h>

#if defined(KRECEK)
#include <pico/mutex.h>
auto_init_mutex(melodies_mutex);
#else
static int melodies_mutex = 0;
#endif

#if !defined(SDK_MAX_MELODIES)
#define SDK_MAX_MELODIES 16
#endif

typedef int16_t (*synth_fn)(uint32_t position, uint32_t step);

static int16_t synth_sine(uint32_t position, uint32_t step);
static int16_t synth_square(uint32_t position, uint32_t step);
static int16_t synth_phi(uint32_t position, uint32_t step);
static int16_t synth_noise(uint32_t position, uint32_t step);
static int16_t synth_prnl(uint32_t position, uint32_t step);

static synth_fn instruments[] = {
	synth_sine, synth_square, synth_noise, synth_phi, synth_prnl,
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

typedef struct sdk_melody {
	const char *melody; /* Whole melody string. */
	const char *cursor; /* Cursor into the melody to play next. */
	const char *repeat; /* Cursor to return to on '}'. */

	uint16_t duration; /* Ticks per quarter note. */
	uint16_t rest;	   /* Remaining rest time. */
	uint8_t octave;	   /* Current octave as shift right argument. */
	uint8_t volume;	   /* Current volume table index. */
	uint8_t panning;   /* Panning from 0 (L) through 3 (C) to 6 (R). */

	synth_fn synth;

	struct {
		uint16_t attack, decay, sustain, release;
		uint32_t position;
		uint32_t step;
	} note;
} sdk_melody_t;

static sdk_melody_t melodies[SDK_MAX_MELODIES];

static void lock_melodies(void)
{
#if defined(KRECEK)
	mutex_enter_blocking(&melodies_mutex);
#else
	melodies_mutex++;

	if (melodies_mutex >= 2)
		printf("melodies_mutex = %i\n", melodies_mutex);
#endif
}

static void unlock_melodies(void)
{
#if defined(KRECEK)
	mutex_exit(&melodies_mutex);
#else
	melodies_mutex--;

	if (melodies_mutex < 0)
		printf("melodies_mutex = %i\n", melodies_mutex);
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
			lock_melodies();
			memset(melody, 0, sizeof(*melody));
			unlock_melodies();
			return;

		case SDK_MELODY_TOKEN_END:
			lock_melodies();
			memset(melody, 0, sizeof(*melody));
			unlock_melodies();
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
					lock_melodies();
					memset(melody, 0, sizeof(*melody));
					unlock_melodies();
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

static void setup(sdk_melody_t *melody, const char *str)
{
	melody->cursor = str;
	melody->repeat = strchr(str, '{');
	melody->octave = BASE_OCTAVE;
	melody->volume = BASE_VOLUME;
	melody->duration = SDK_AUDIO_RATE * 15 / 120;
	melody->synth = synth_sine;
	melody->panning = 3;

	// This has to come last since that's what the sampler checks.
	// Once we turn it non-null, it will try to advance it.
	melody->melody = str;
}

bool sdk_melody_play(const char *melody)
{
	lock_melodies();

	int free_spot = -1;

	for (int i = 0; i < SDK_MAX_MELODIES; i++) {
		if (melody == melodies[i].melody) {
			unlock_melodies();
			return false;
		} else if (!melodies[i].melody) {
			free_spot = i;
		}
	}

	if (free_spot < 0) {
		unlock_melodies();
		return false;
	}

	setup(&melodies[free_spot], melody);
	unlock_melodies();
	return true;
}

bool sdk_melody_is_playing(const char *melody)
{
	lock_melodies();

	for (int i = 0; i < SDK_MAX_MELODIES; i++) {
		if (melody == melodies[i].melody) {
			unlock_melodies();
			return true;
		}
	}

	unlock_melodies();
	return false;
}

static void stop_playing(sdk_melody_t *melody)
{
	melody->repeat = NULL;
	melody->melody = "";
	melody->cursor = "";
}

void sdk_melody_stop_playing(const char *melody)
{
	lock_melodies();

	for (int i = 0; i < SDK_MAX_MELODIES; i++) {
		if (!melodies[i].melody)
			continue;

		if (melodies[i].melody == melody || melody == SDK_ALL_MELODIES ||
		    (melody == SDK_ALL_LOOPING && melodies[i].repeat) ||
		    (melody == SDK_ALL_NOT_LOOPING && !melodies[i].repeat)) {
			stop_playing(&melodies[i]);
		}
	}

	unlock_melodies();
}

void sdk_melody_stop_looping(const char *melody)
{
	lock_melodies();

	for (int i = 0; i < SDK_MAX_MELODIES; i++) {
		if (!melodies[i].melody)
			continue;

		if (melodies[i].melody == melody || melody == SDK_ALL_MELODIES ||
		    (melody == SDK_ALL_LOOPING && melodies[i].repeat) ||
		    (melody == SDK_ALL_NOT_LOOPING && !melodies[i].repeat)) {
			melodies[i].repeat = NULL;
		}
	}

	unlock_melodies();
}

static int16_t synth_sine(uint32_t position, uint32_t step)
{
	return INT16_MAX * sinf((int)(position * step) * (float)M_PI / (1 << 31));
}

static int16_t synth_square(uint32_t position, uint32_t step)
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

static int16_t synth_prnl(uint32_t position, uint32_t step)
{
	int period = UINT32_MAX / step;
	return pseudorandom_noise(position % period);
}

static int16_t synth_noise(uint32_t position, uint32_t step)
{
	(void)step;
	return pseudorandom_noise(position);
}

static int16_t synth_phi(uint32_t position, uint32_t step)
{
	return (0x9e3779b9u * (position * step)) >> 16;
}

void sdk_melody_sample(int16_t *left, int16_t *right)
{
	int accleft = 0;
	int accright = 0;

	for (int i = 0; i < SDK_MAX_MELODIES; i++) {
		sdk_melody_t *melody = &melodies[i];

		if (!melody->melody)
			continue;

		if (melody->rest > 0) {
			melody->rest--;

			if (!melody->rest)
				advance(melody);

			continue;
		}

		int16_t amplitude;

		uint32_t offset = melody->note.position;
		uint32_t milestone = melody->note.attack;

		int peak = volumes[melody->volume];
		int base = (3 * peak) >> 2;

		if (melody->note.position < milestone) {
			amplitude = lerp(0, peak, offset, melody->note.attack);
			goto sample;
		}

		offset -= melody->note.attack;
		milestone += melody->note.decay;

		if (melody->note.position < milestone) {
			amplitude = lerp(peak, base, offset, melody->note.decay);
			goto sample;
		}

		offset -= melody->note.decay;
		milestone += melody->note.sustain;

		if (melody->note.position < milestone) {
			amplitude = base;
			goto sample;
		}

		offset -= melody->note.sustain;
		milestone += melody->note.release;

		if (melody->note.position < milestone) {
			amplitude = lerp(base, 0, offset, melody->note.release);
			goto sample;
		}

		// Note is finished playing.
		advance(melody);
		continue;

sample:
		int sample = melody->synth(melody->note.position++, melody->note.step);
		sample = (sample * amplitude) >> 16;

		accleft += lerp(sample, 0, melody->panning, 6);
		accright += lerp(0, sample, melody->panning, 6);
	}

	*left = clamp(accleft, INT16_MIN, INT16_MAX);
	*right = clamp(accright, INT16_MIN, INT16_MAX);
}
