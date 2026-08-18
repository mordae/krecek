#include "minevox.h"

/*
 * ===========================================================================
 * Entry points
 * ===========================================================================
 *
 * The SDK owns the main loop and calls into us. Our whole game is four
 * functions:
 *
 *   game_start   once, at power on
 *   game_input   before each frame, with the time since the last one
 *   game_paint   to fill the frame buffer
 *   game_inbox   when another console sends us something, which we ignore
 *
 * Audio is deliberately not overridden. The SDK provides a default
 * game_audio that plays the melodies we start with sdk_melody_play, and
 * that is all the sound this game makes.
 */

sdk_game_info("minevox", NULL);

void game_start(void)
{
	/*
	 * Order matters. mem_init hands out the arena the pools are carved
	 * from, so it must run before anything that allocates.
	 */
	mem_init();
	blocks_init();
	render_init();
	items_init();
	mobs_init();
	nether_init();

	inventory_clear();
	save_scan();

	/*
	 * Build a throwaway world straight away. Nothing is loaded yet, but
	 * the title screen is drawn over the live world, so there has to be
	 * one to look at.
	 */
	survival_mode = true;
	world_generate(sdk_random());
	player_reset(false);
	mobs_populate();

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

	/*
	 * Clamp the step. A long stall, say while a world generates, would
	 * otherwise arrive as one enormous step and teleport the player
	 * through the floor.
	 */
	if (dts > 0.1f)
		dts = 0.1f;

	pointer_update();

	if (game_state != STATE_PLAY) {
		menu_input(dts);
		return;
	}

	if (sdk_inputs_delta.start > 0) {
		menu_open(STATE_PAUSE);
		return;
	}

	player_input(dts);
	player_update(dts);
	mobs_update(dts);
	items_update(dts);
	nether_update(dts);

	world_time += dts;

	if (world_time >= DAY_LENGTH)
		world_time -= DAY_LENGTH;

	world_play_time += dts;
}

void game_paint(unsigned dt)
{
	float dts = (float)dt / 1000000.0f;

	/* The world is drawn behind every screen, menus included. */
	render_world();

	if (game_state == STATE_PLAY)
		hud_paint(dts);

	/* Handles the menu pages, and the toast that floats over them all. */
	menu_paint(dts);
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
