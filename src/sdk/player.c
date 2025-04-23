#include <stdlib.h>
#include <math.h>
#include <sdk/audio.h>

void sdk_player_sample(sdk_player_t *player, int16_t *left, int16_t *right)
{
	// Tracks we want to keep.
	sdk_track_t *next_tracks = NULL;

	int accleft = 0;
	int accright = 0;

	// Go over all tracks and make them output samples.
	for (sdk_track_t *track = player->tracks; track; track = track->next) {
		int16_t amplitude;

		uint offset = track->position;
		uint milestone = track->attack;

		if (track->position < milestone) {
			amplitude = lerp(0, track->peak, offset, track->attack);
			goto pan;
		}

		offset -= track->attack;
		milestone += track->decay;

		if (track->position < milestone) {
			amplitude = lerp(track->peak, track->base, offset, track->decay);
			goto pan;
		}

		offset -= track->decay;
		milestone += track->sustain;

		if (track->position < milestone) {
			amplitude = track->base;
			goto pan;
		}

		offset -= track->sustain;
		milestone += track->release;

		if (track->position < milestone) {
			amplitude = lerp(track->base, 0, offset, track->release);
			goto pan;
		}

		// Track is done.
		track->position = 0;
		track->next = NULL;
		track->player = NULL;
		continue;

pan:
		int sample = track->fn(track, amplitude);

		track->position++;
		track->next = next_tracks;
		next_tracks = track;

		int pan = INT16_MAX + track->pan;

		accleft += lerp(sample, 0, pan, 1 << 16);
		accright += lerp(0, sample, pan, 1 << 16);
	}

	player->tracks = next_tracks;

	*left = clamp(accleft, INT16_MIN, INT16_MAX);
	*right = clamp(accright, INT16_MIN, INT16_MAX);
}

int16_t sdk_play_sine(void *_self, int16_t amplitude)
{
	sdk_track_sine_t *self = _self;

	if (!self->track.position)
		self->phase = 0;

	int16_t sample = sinf(self->phase * M_PI / INT32_MAX) * amplitude;
	self->phase += UINT32_MAX / (SDK_AUDIO_RATE / self->frequency);
	return sample;
}

int16_t sdk_play_square(void *_self, int16_t amplitude)
{
	sdk_track_square_t *self = _self;

	if (!self->track.position)
		self->phase = 0;

	int16_t sample = (self->phase >> 31) ? -amplitude : amplitude;
	self->phase += UINT32_MAX / (SDK_AUDIO_RATE / self->frequency);
	return sample;
}

int16_t sdk_play_noise(void *_self, int16_t amplitude)
{
	(void)_self;
	return (((int)rand() >> 16) * amplitude) >> 16;
}
