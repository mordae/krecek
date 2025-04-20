#include <sdk.h>
#include <SDL.h>

struct sdk_inputs sdk_inputs = {};
struct sdk_inputs sdk_inputs_delta = {};

static struct sdk_inputs prev_inputs = {};

void sdk_input_init(void)
{
}

static int joy_up = 0;
static int joy_down = 0;
static int joy_left = 0;
static int joy_right = 0;

static uint32_t select_held_since = 0;

static void on_keydown(SDL_Scancode code)
{
	switch (code) {
	case SDL_SCANCODE_L:
		sdk_inputs.a = 1;
		break;

	case SDL_SCANCODE_P:
		sdk_inputs.b = 1;
		break;

	case SDL_SCANCODE_K:
		sdk_inputs.x = 1;
		break;

	case SDL_SCANCODE_O:
		sdk_inputs.y = 1;
		break;

	case SDL_SCANCODE_KP_PLUS:
		sdk_inputs.vol_up = 1;
		break;

	case SDL_SCANCODE_KP_MINUS:
		sdk_inputs.vol_down = 1;
		break;

	case SDL_SCANCODE_KP_ENTER:
		sdk_inputs.vol_sw = 1;
		break;

	case SDL_SCANCODE_0:
		sdk_inputs.aux[0] = 1;
		break;

	case SDL_SCANCODE_1:
		sdk_inputs.aux[1] = 1;
		break;

	case SDL_SCANCODE_2:
		sdk_inputs.aux[2] = 1;
		break;

	case SDL_SCANCODE_3:
		sdk_inputs.aux[3] = 1;
		break;

	case SDL_SCANCODE_4:
		sdk_inputs.aux[4] = 1;
		break;

	case SDL_SCANCODE_5:
		sdk_inputs.aux[5] = 1;
		break;

	case SDL_SCANCODE_6:
		sdk_inputs.aux[6] = 1;
		break;

	case SDL_SCANCODE_7:
		sdk_inputs.aux[7] = 1;
		break;

	case SDL_SCANCODE_RETURN:
		sdk_inputs.start = 1;
		break;

	case SDL_SCANCODE_BACKSPACE:
		sdk_inputs.select = 1;
		break;

	case SDL_SCANCODE_Q:
		sdk_inputs.brack_l = 1;
		break;

	case SDL_SCANCODE_E:
		sdk_inputs.brack_r = 1;
		break;

	case SDL_SCANCODE_W:
	case SDL_SCANCODE_UP:
		joy_up = 1;
		break;

	case SDL_SCANCODE_S:
	case SDL_SCANCODE_DOWN:
		joy_down = 1;
		break;

	case SDL_SCANCODE_A:
	case SDL_SCANCODE_LEFT:
		joy_left = 1;
		break;

	case SDL_SCANCODE_D:
	case SDL_SCANCODE_RIGHT:
		joy_right = 1;
		break;

	default:
		break;
	}
}

static void on_keyup(SDL_Scancode code)
{
	switch (code) {
	case SDL_SCANCODE_L:
		sdk_inputs.a = 0;
		break;

	case SDL_SCANCODE_P:
		sdk_inputs.b = 0;
		break;

	case SDL_SCANCODE_K:
		sdk_inputs.x = 0;
		break;

	case SDL_SCANCODE_O:
		sdk_inputs.y = 0;
		break;

	case SDL_SCANCODE_KP_PLUS:
		sdk_inputs.vol_up = 0;
		break;

	case SDL_SCANCODE_KP_MINUS:
		sdk_inputs.vol_down = 0;
		break;

	case SDL_SCANCODE_KP_ENTER:
		sdk_inputs.vol_sw = 0;
		break;

	case SDL_SCANCODE_0:
		sdk_inputs.aux[0] = 0;
		break;

	case SDL_SCANCODE_1:
		sdk_inputs.aux[1] = 0;
		break;

	case SDL_SCANCODE_2:
		sdk_inputs.aux[2] = 0;
		break;

	case SDL_SCANCODE_3:
		sdk_inputs.aux[3] = 0;
		break;

	case SDL_SCANCODE_4:
		sdk_inputs.aux[4] = 0;
		break;

	case SDL_SCANCODE_5:
		sdk_inputs.aux[5] = 0;
		break;

	case SDL_SCANCODE_6:
		sdk_inputs.aux[6] = 0;
		break;

	case SDL_SCANCODE_7:
		sdk_inputs.aux[7] = 0;
		break;

	case SDL_SCANCODE_RETURN:
		sdk_inputs.start = 0;
		break;

	case SDL_SCANCODE_BACKSPACE:
		sdk_inputs.select = 0;
		break;

	case SDL_SCANCODE_Q:
		sdk_inputs.brack_l = 0;
		break;

	case SDL_SCANCODE_E:
		sdk_inputs.brack_r = 0;
		break;

	case SDL_SCANCODE_W:
	case SDL_SCANCODE_UP:
		joy_up = 0;
		break;

	case SDL_SCANCODE_S:
	case SDL_SCANCODE_DOWN:
		joy_down = 0;
		break;

	case SDL_SCANCODE_A:
	case SDL_SCANCODE_LEFT:
		joy_left = 0;
		break;

	case SDL_SCANCODE_D:
	case SDL_SCANCODE_RIGHT:
		joy_right = 0;
		break;

	default:
		break;
	}
}

void sdk_input_handle(const SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN) {
		on_keydown(event->key.keysym.scancode);
	}

	if (event->type == SDL_KEYUP) {
		on_keyup(event->key.keysym.scancode);
	}
}

void sdk_input_commit(uint32_t dt)
{
	/* Dummy current control and temperature readings. */
	sdk_inputs.batt_mv = 3900.0f;
	sdk_inputs.cc_mv = 400.0f;
	sdk_inputs.temp = 35.0f;

	sdk_inputs.hps_mv = 2100.0f;
	sdk_inputs.hps = 0;

	/* Calculate input deltas */
	sdk_inputs_delta.a = sdk_inputs.a - prev_inputs.a;
	sdk_inputs_delta.b = sdk_inputs.b - prev_inputs.b;
	sdk_inputs_delta.x = sdk_inputs.x - prev_inputs.x;
	sdk_inputs_delta.y = sdk_inputs.y - prev_inputs.y;

	sdk_inputs_delta.joy_x = sdk_inputs.joy_x - prev_inputs.joy_x;
	sdk_inputs_delta.joy_y = sdk_inputs.joy_y - prev_inputs.joy_y;

	sdk_inputs_delta.vol_up = sdk_inputs.vol_up - prev_inputs.vol_up;
	sdk_inputs_delta.vol_down = sdk_inputs.vol_down - prev_inputs.vol_down;
	sdk_inputs_delta.vol_sw = sdk_inputs.vol_sw - prev_inputs.vol_sw;

	sdk_inputs_delta.hps = sdk_inputs.hps - prev_inputs.hps;

	sdk_inputs_delta.brack_l = sdk_inputs.brack_l - prev_inputs.brack_l;
	sdk_inputs_delta.brack_r = sdk_inputs.brack_r - prev_inputs.brack_r;

	for (int i = 0; i < 8; i++)
		sdk_inputs_delta.aux[i] = sdk_inputs.aux[i] - prev_inputs.aux[i];

	sdk_inputs_delta.start = sdk_inputs.start - prev_inputs.start;
	sdk_inputs_delta.select = sdk_inputs.select - prev_inputs.select;

	sdk_inputs_delta.cc_mv = sdk_inputs.cc_mv - prev_inputs.cc_mv;
	sdk_inputs_delta.temp = sdk_inputs.temp - prev_inputs.temp;
	sdk_inputs_delta.hps_mv = sdk_inputs.hps_mv - prev_inputs.hps_mv;

	sdk_inputs.joy_x = 2047 * joy_right - 2047 * joy_left;
	sdk_inputs.joy_y = 2047 * joy_down - 2047 * joy_up;

	prev_inputs = sdk_inputs;

	if (sdk_config.off_on_select && sdk_inputs.select) {
		if (!select_held_since) {
			select_held_since = time_us_32();
		} else {
			uint32_t select_held_for = time_us_32() - select_held_since;
			if (select_held_for > 3000000) {
				puts("sdk: SELECT held for 3s, turning off...");
				exit(0);
			}
		}
	} else {
		select_held_since = 0;
	}

	game_input(dt);
}
