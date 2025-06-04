#include "common.h"

#include "volume.h"

#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <math.h>
#include <tft.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

float volume = 0.5f;

void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_handle_audio(dt, volume);
}

void game_paint(unsigned dt_usec)
{
	tft_fill(0);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = WHITE,
	};
	sdk_main(&config);
}
