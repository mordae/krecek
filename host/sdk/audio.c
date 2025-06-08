#include <SDL_audio.h>
#include <sdk.h>

static SDL_AudioDeviceID device_id;

#define BUFFER_SIZE 1024
static int16_t buffer[BUFFER_SIZE][2];
static int used = 0;

void sdk_audio_init(void)
{
	SDL_AudioSpec desired = {
		.channels = 2,
		.format = AUDIO_S16SYS,
		.freq = SDK_AUDIO_RATE,
		.samples = BUFFER_SIZE / 2,
	};
	SDL_AudioSpec obtained = {};

	device_id = SDL_OpenAudioDevice(NULL, false, &desired, &obtained, 0);

	if (obtained.channels != desired.channels)
		sdk_panic("SDL_OpenAudioDevice failed to get us 2 channels");

	if (obtained.format != desired.format)
		sdk_panic("SDL_OpenAudioDevice failed to get us S16SYS format");

	if (obtained.freq != desired.freq)
		sdk_panic("SDL_OpenAudioDevice failed to get us %u Hz", SDK_AUDIO_RATE);

	if (obtained.samples < desired.samples)
		sdk_panic("SDL_OpenAudioDevice failed to get us %u sample buffer", desired.samples);
}

void sdk_audio_start(void)
{
	SDL_PauseAudioDevice(device_id, false);
}

void sdk_audio_flush()
{
	if (!used)
		return;

	if (SDL_QueueAudio(device_id, buffer, used * 4))
		sdk_panic("SDL_QueueAudio failed: %s", SDL_GetError());

	used = 0;
}

void sdk_audio_task(void)
{
	int pending = SDL_GetQueuedAudioSize(device_id);

	if (pending > BUFFER_SIZE)
		return;

	game_audio(BUFFER_SIZE);
}

void sdk_write_sample(int16_t left, int16_t right)
{
	if (BUFFER_SIZE == used)
		sdk_audio_flush();

	// Add the sample.
	buffer[used][0] = left;
	buffer[used][1] = right;
	used++;
}

void sdk_read_sample(int16_t *left, int16_t *right)
{
	*left = 0;
	*right = 0;
}

void sdk_set_output_gain_db(float gain)
{
	(void)gain;
}

void sdk_enable_headphones(bool en)
{
	(void)en;
}
