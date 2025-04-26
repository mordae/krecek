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
static int synth_string(uint32_t position, uint32_t step);

static synth_fn instruments[] = {
	synth_sine, synth_square, synth_noise, synth_phi, synth_prnl, synth_flute, synth_string,
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
			printf("melody: invalid token: %s\n", melody->cursor);
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
					printf("melody: empty loop: %s\n", melody->repeat);
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

static const int16_t noise9[512] = {
	12204,	-12455, -15025, -3754,	100,	-27901, 1266,	-27050, -23445, -32326, 17717,
	-30216, -5034,	20016,	-16747, 18775,	21673,	-10772, 4619,	28818,	531,	11280,
	5375,	7153,	32016,	-380,	-6607,	31136,	-31868, -13796, 26514,	27084,	9923,
	-16739, -1212,	441,	-7363,	-15810, 5314,	-17088, 9043,	-296,	-18355, 24079,
	18194,	-17448, -29422, 22326,	17686,	16975,	-12548, 3166,	-22585, 26607,	-9384,
	-25250, 25738,	6120,	17487,	20279,	-19074, 18943,	18850,	25891,	18423,	-13138,
	-24879, 10452,	-7256,	30679,	16978,	-16547, 25521,	-21800, -10826, -21354, 29838,
	-15069, 19525,	-17044, -25569, 27864,	14923,	-6588,	-13305, 16410,	-12145, 12871,
	22175,	21031,	6170,	-14105, -21904, -26553, -29225, 22378,	4972,	-8588,	-10337,
	17342,	13088,	-10226, -11197, 21925,	-13009, -24252, 3887,	-18042, 9517,	-5944,
	23101,	-6590,	-31081, -31143, 12953,	-936,	29482,	3777,	-29623, 17281,	7981,
	-5878,	-29406, 20511,	6787,	23619,	-1059,	17984,	-18142, 4765,	-5583,	-25953,
	-7937,	11584,	14813,	-16779, -24427, -9664,	23350,	-7301,	16860,	29753,	24991,
	9138,	129,	-12215, -14211, -23835, -31792, 11462,	20182,	11257,	-3070,	-25852,
	24905,	16305,	-23647, -22804, 20733,	20083,	6909,	-24767, -26807, -22261, -30650,
	8506,	11883,	-15178, 49,	-28574, 15687,	-21546, 6294,	24345,	31385,	-32533,
	6384,	-24916, -5414,	-25977, 4424,	13819,	20617,	12076,	-2146,	-20394, -23789,
	-15503, -6810,	-22286, 30915,	17658,	17147,	-3856,	-8920,	15943,	-19335, -25481,
	13565,	-30780, 9552,	11247,	-23394, -16807, -3364,	8728,	30845,	-1514,	24794,
	-21465, 4922,	241,	-17507, -6461,	23124,	15284,	-4223,	27343,	-3141,	-23979,
	-27892, 13401,	-30909, 29601,	13178,	20350,	-17840, 26455,	716,	17230,	-30532,
	-23924, 28449,	11660,	5533,	-25016, -12557, 9355,	-10391, -3220,	-22113, 19406,
	-9153,	-16201, 14809,	-19582, -25169, -32441, 15238,	-26987, 25322,	10735,	18444,
	-26145, -24117, 16188,	20602,	14121,	10679,	23472,	20534,	-18228, -27574, 10191,
	12164,	25468,	25897,	25258,	21293,	-11359, 21658,	-4341,	-3392,	19522,	-20818,
	-24230, 5877,	14265,	-14150, 19135,	-28754, -6388,	219,	-12703, 29317,	-12824,
	-20253, 12886,	-10086, 8572,	-15256, -24122, -6661,	-23531, 23039,	30438,	-12627,
	32701,	-17672, 1004,	6079,	-23550, -6621,	1318,	14166,	13017,	16359,	16480,
	-23006, -558,	-1969,	8714,	-15314, 21412,	20849,	-19501, 15723,	19358,	11730,
	-30227, -2167,	-9391,	-8578,	-22696, 19831,	27146,	6541,	-10821, -30600, 12037,
	-6719,	-21238, -10580, 9686,	26161,	-22118, 24926,	-11836, 11509,	30765,	32694,
	-15249, 14013,	-3335,	-23671, -17154, -28685, -23948, -1250,	-26238, 17665,	17695,
	-30690, 30768,	17840,	15982,	12324,	-26737, -15560, 18902,	11114,	21851,	-4413,
	-31651, 9174,	17521,	-17616, 1421,	26614,	-18361, -8360,	5075,	-28905, -15516,
	-28604, 26956,	-2276,	-434,	4318,	-29704, 17840,	-26657, -10142, 32681,	-28342,
	-14846, 7570,	28782,	-1494,	-17307, 16195,	-6207,	-8898,	-21627, -19094, -16063,
	-25076, -3949,	-11472, 24488,	1943,	-29980, 5034,	-29840, -9164,	-6868,	29773,
	23805,	9925,	15952,	15065,	24075,	23198,	29234,	9549,	12510,	-15782, -1912,
	26096,	-19020, 28906,	3780,	-13516, 833,	-11862, -18569, -9289,	-2916,	-11090,
	-22803, 18183,	25343,	10453,	-7355,	-12915, 27931,	-14964, -26791, -8058,	16576,
	-5779,	24735,	19288,	17854,	-11210, -24374, 16668,	15415,	-27665, -31416, 22312,
	28925,	-17632, 32764,	-11116, 9724,	28085,	-2936,	30949,	16859,	1375,	2244,
	-29589, 18280,	-6133,	19027,	-23158, 30086,	-30304, 32722,	-31783, -11452, 30903,
	-28823, -30803, -10733, 9710,	-11585, -3640,	-13392, 452,	1450,	-8214,	-5992,
	4127,	-19780, 30200,	2875,	-21958, 22953,	17474,	26297,	9491,	-28707, 20191,
	2880,	-22563, -11802, 8730,	24629,	25438,	3013,	-29541, -23732, -20033, 11004,
	21809,	19727,	-808,	-8595,	29101,	-30703
};

static const int16_t noise8[256] = {
	-126,	-9390,	-13901, -12892, -27886, -6250,	7491,	1014,	5450,	16718,	5905,
	6264,	15818,	12264,	-22832, 26799,	-3408,	-386,	-11587, -5887,	4373,	2862,
	373,	-3548,	17330,	-4691,	2011,	-17317, 15929,	18883,	-66,	22370,	2642,
	-7214,	11711,	215,	1860,	-16090, 7384,	1240,	1147,	4167,	1552,	363,
	21603,	-3968,	-24229, -3424,	-1808,	3502,	1431,	5364,	-18631, -7078,	1786,
	8255,	-31112, 6008,	16629,	-6171,	1051,	-4448,	15203,	8462,	-6689,	-15768,
	1823,	-983,	-17046, 8024,	23306,	17064,	-6043,	-19023, -10165, 15719,	-14461,
	20605,	-23226, 20408,	-8929,	-24534, -11072, -1648,	-14263, -2930,	15319,	-574,
	-9266,	-15696, 9121,	16346,	-11270, -19646, -14548, 24286,	6645,	3511,	-22408,
	-8608,	10399,	-20101, 2682,	14665,	1664,	2581,	-11984, 19204,	11560,	-13560,
	-7246,	-654,	16764,	4307,	8973,	-27228, 20054,	-9742,	-1601,	-6806,	-1354,
	-12677, -2387,	-28805, -5875,	18028,	-3851,	-3965,	17361,	17075,	1153,	-8692,
	18816,	25577,	4967,	8658,	8065,	-22524, 10071,	2492,	-17571, -6242,	8246,
	-3684,	-757,	-19689, -15096, 26738,	10037,	-8334,	-8736,	-2652,	13591,	16419,
	-11782, 3372,	3049,	674,	17540,	-9249,	-5779,	-15637, 23488,	-2140,	-9282,
	-13979, -447,	2021,	6545,	21137,	8722,	5339,	-20413, -26317, -13744, 17680,
	39,	16911,	-7207,	1671,	16482,	-18032, 13347,	-8098,	4126,	-1643,	-22211,
	-824,	-1355,	-12693, -4409,	11269,	-21594, 18176,	-9401,	4994,	-15263, -17579,
	-14513, 6508,	-14019, -12403, -8016,	26789,	12938,	19570,	26216,	11029,	-8847,
	3538,	16343,	-6342,	-15216, -6103,	-16947, 21763,	1549,	7508,	-20878, 4259,
	9478,	18571,	-17792, 16041,	-29541, 25618,	7566,	-696,	12574,	23904,	1809,
	-5655,	6447,	3464,	1209,	-21618, 1040,	-20768, -938,	-8516,	951,	-7103,
	-7827,	16537,	497,	21885,	-9608,	11535,	-17183, 16679,	14225,	-26637, -4515,
	20768,	-4702,	-801
};

static const int16_t noise7[128] = {
	-4758,	-13397, -17068, 4252,  11084,  6084,  14041, 1983,  -1897,  -8737,  3617,   -1588,
	6319,	-7653,	17406,	11152, -2286,  5963,  -7115, 4312,  2657,   957,    8817,   -13826,
	847,	3397,	-12854, 5021,  -12552, 5229,  -1698, 11832, -11229, 420,    -4511,  20185,
	-12533, 2777,	3072,	-1409, -16732, -6360, -8596, 7372,  -12481, 12734,  -15458, 4869,
	5078,	-15508, -4851,	8673,  2123,   3610,  -1000, -3950, 10535,  -9128,  5156,   -4204,
	-7016,	-15596, 6077,	-3908, 17218,  -3770, 22196, 6812,  -7230,  6281,   -11907, 2281,
	-10223, 5821,	851,	-5694, 15005,  -4205, 1861,  4146,  -10708, 10674,  -11630, 787,
	13841,	7030,	-23365, 1968,  8475,   -2768, -775,  2625,  1242,   -11518, -7024,  3430,
	-1709,	-2204,	-16421, -4003, -13211, 9386,  16254, 18622, -2655,  5000,   -10659, 2408,
	4528,	-8310,	14024,	-876,  -1961,  3435,  18239, -1923, 4955,   -10205, -9864,  -4727,
	-3076,	4355,	11191,	963,   -252,   -6206, 8126,  -2752
};

static const int16_t noise6[64] = { -9077, -6408,  8584,  8012,	  -5317,  1015,	 -667,	 14279,
				    1839,  -1402,  1807,  -2505,  2122,	  -3917, -3662,	 5067,
				    -5404, 7837,   -4878, 831,	  -11546, -612,	 126,	 -5295,
				    -5215, 1911,   2866,  -2475,  704,	  476,	 -11306, 1084,
				    6724,  14504,  -474,  -4813,  -2201,  -2421, 5400,	 3003,
				    -17,   -5422,  10435, -10699, 2853,	  925,	 -5138,	 -1797,
				    -1957, -10212, -1913, 17438,  1173,	  -4126, -1891,	 6574,
				    737,   8158,   -2625, -7296,  639,	  6077,	 -3229,	 2687 };

static const int16_t noise5[32] = { -7743, 8298,  -2151, 6806,	218,   -349,  -898, 703,
				    1216,  -2024, -6079, -2584, -1652, 195,   590,  -5111,
				    10614, -2644, -2311, 4202,	-2720, -132,  1889, -3468,
				    -6084, 7763,  -1477, 2342,	4447,  -4960, 3358, -271 };

static const int16_t noise4[16] = { 278,  2327, -65,   -97,  -404, -4332, -728, -2260,
				    3985, 945,	-1426, -789, 839,  433,	  -257, 1544 };

static const int16_t noise3[8] = { 1302, -81, -2368, -1494, 2465, -1108, 636, 644 };

static const int16_t noise2[4] = { 611, -1931, 679, 640 };

static const int16_t noise1[2] = { -660, 659 };

static const int16_t noise0[1] = { 0 };

static const int16_t *noise_levels[10] = { noise9, noise8, noise7, noise6, noise5,
					   noise4, noise3, noise2, noise1, noise0 };

static int synth_string(uint32_t position, uint32_t step)
{
	uint32_t mip = clamp(32 - 9 - __builtin_clz(position), 0, 8);
	uint32_t frac = (position * step) >> (32 - 9 + mip);

	int y0 = noise_levels[mip + 0][frac];
	int y1 = noise_levels[mip + 1][frac >> 1];

	return lerp(y0, y1, position >> mip, 512);
}
