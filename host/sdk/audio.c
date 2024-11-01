#include <limits.h>

#include <sdk.h>
#include <sdk/slave.h>

void sdk_audio_task(void)
{
	// TODO: Actually implement PulseAudio.
	//game_audio(128);
}

bool sdk_write_sample(int16_t sample)
{
	(void)sample;
	return true;
}

bool sdk_read_sample(int16_t *sample)
{
	(void)sample;
	return true;
}

int sdk_write_samples(const int16_t *buf, int len)
{
	for (int i = 0; i < len; i++)
		if (!sdk_write_sample(buf[i]))
			return i;

	return len;
}

int sdk_read_samples(int16_t *buf, int len)
{
	for (int i = 0; i < len; i++)
		if (!sdk_read_sample(buf + i))
			return i;

	return len;
}

void sdk_audio_init(void)
{
}

void sdk_audio_start(void)
{
}

void sdk_set_output_gain_db(float gain)
{
	(void)gain;
}
