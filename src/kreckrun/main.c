#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <krecek.png.h>
#include <back.png.h>

struct k {
	int lp;
};
static struct k k;

void game_reset(void)
{
}

/*
void game_input(unsigned dt_usec)
{
}
*/

void game_start(void)
{
	/*	sdk_set_output_gain_db(6); 
*/
}

/*
void game_audio(int nsamples)
{
		for (int s = 0; s < nsamples; s++) {
		int remaining = ehp_max - ehp;

		if (!remaining)
			remaining = 1;

		int max = INT16_MAX / 6 * remaining / ehp_max;
		int sample = rand() % (2 * max) - max;

		sdk_write_sample(sample);
	}

}
*/

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	k.lp = (time_us_32() >> 18) & 1;

	sdk_draw_tile(0, 0, &ts_back_png, 0);

	sdk_draw_tile(21, 18, &ts_krecek_png, k.lp);
	sdk_draw_tile(21, 18 + 30, &ts_krecek_png, k.lp);
	sdk_draw_tile(21, 18 + 60, &ts_krecek_png, k.lp);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = 0,
	};

	sdk_main(&config);
}
