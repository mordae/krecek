#include <sdk.h>
#include <cover.png.h>

sdk_game_info("menu", &image_cover_png);

extern sdk_scene_t scene_root;

void game_start(void)
{
	sdk_scene_push(&scene_root);
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;
	sdk_scene_handle();
}

void game_paint(unsigned dt_usec)
{
	sdk_scene_paint(dt_usec);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(63, 63, 63),
	};

	sdk_main(&config);
}
