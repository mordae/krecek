#include <sdk.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <string.h>

static pa_simple *pa;

#define BUFFER_SIZE 1024
static int16_t buffer[BUFFER_SIZE];
static int used = 0;

/*
 * This is a private API, I shouldn't be declaring it here.
 *
 * But I just need to be able to tell how much to feed
 * pa_simple_write() without it blocking
 */
struct pa_simple {
	pa_threaded_mainloop *mainloop;
	pa_context *context;
	pa_stream *stream;
	pa_stream_direction_t direction;
};

void sdk_audio_init(void)
{
	pa_sample_spec ss = {
		.format = PA_SAMPLE_S16NE,
		.channels = 1,
		.rate = SDK_AUDIO_RATE,
	};

	pa = pa_simple_new(NULL, "Krecek", PA_STREAM_PLAYBACK, NULL, "Game", &ss, NULL, NULL, NULL);

	if (NULL == pa)
		sdk_panic("pa_simple_new failed");
}

void sdk_audio_start(void)
{
}

void sdk_audio_flush()
{
	int error = 0;

	if (!used)
		return;

	// How much can we write?
	int writable = pa_stream_writable_size(pa->stream) / 2;

	if (writable < used)
		return; // No can do.

	// We can flush it now, good.
	if (0 > pa_simple_write(pa, buffer, used * 2, &error))
		sdk_panic("pa_simple_write failed, error=%i", error);

	used = 0;
}

void sdk_audio_task(void)
{
	int writable = pa_stream_writable_size(pa->stream) / 2;
	if (writable)
		game_audio(writable);
}

void sdk_write_sample(int16_t left, int16_t right)
{
	int16_t sample = ((int32_t)left + (int32_t)right) >> 1;

	int error = 0;

	if (used == BUFFER_SIZE) {
		// Buffer is full, check if we can flush it.
		int writable = pa_stream_writable_size(pa->stream) / 2;

		if (writable < used)
			return; // No can do.

		// Yes, we can flush it. Let's do so.
		if (0 > pa_simple_write(pa, buffer, BUFFER_SIZE * 2, &error))
			sdk_panic("pa_simple_write failed, error=%i", error);

		// We now have a nice, empty buffer.
		used = 0;
	}

	// Add the sample.
	buffer[used++] = sample;
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
