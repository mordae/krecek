#include <sdk.h>
#include <stdio.h>

float volume = 0.5f;

extern int fast_add(int a, int b);

void game_start(void)
{
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	int result = fast_add(5, 7);
	printf("Result = %d\n", result);
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0); //reset screen every frame to 0-black
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
