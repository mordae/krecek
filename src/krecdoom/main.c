//#include "volume.h"
#include "common.h"

#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <tft.h>

// sdk_game_info("krecdoom", &image_cover_png);

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)

float volume = 0;

void game_handle_audio(float dt, float volume);

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
	(void)dt_usec;
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
