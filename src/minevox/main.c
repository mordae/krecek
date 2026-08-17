#include "minevox.h"

sdk_game_info("minevox", NULL);

void game_start(void)
{
	blocks_init();
	render_init();

	if (world_load()) {
		hud_notice_save(true);
	} else {
		world_generate(sdk_random());
		player_reset();
	}

	game_state = STATE_TITLE;
	sdk_set_backlight(SDK_BACKLIGHT_STD);
}

void game_reset(void)
{
	game_state = STATE_TITLE;
}

void game_input(unsigned dt)
{
	float dts = (float)dt / 1000000.0f;

	if (dts > 0.1f)
		dts = 0.1f;

	if (game_state != STATE_PLAY) {
		hud_input(dts);
		return;
	}

	if (sdk_inputs_delta.start > 0) {
		game_state = STATE_PAUSE;
		return;
	}

	player_input(dts);
	player_update(dts);

	world_time += dts;

	if (world_time >= DAY_LENGTH)
		world_time -= DAY_LENGTH;
}

void game_paint(unsigned dt)
{
	render_world();
	hud_paint((float)dt / 1000000.0f);
}

void game_inbox(sdk_message_t msg)
{
	(void)msg;
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = false,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(255, 255, 255),
	};

	sdk_main(&config);
	return 0;
}
