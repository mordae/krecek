#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>
#include <stdlib.h>
#include <stdio.h>

#include <krecek.png.h>
#include <back.png.h>
#include <extras.png.h>
#include <walls.png.h>

#define RED rgb_to_rgb565(255, 0, 0)
struct game {
	float score;
};
static struct game game;

struct object {
	int l1p0;
	int l1p0pos;
	int l2p0;
	int l3p0;
};

struct k {
	sdk_sprite_t s;
	float pt;
};

static struct k k = {
	.s = {
		.ts = &ts_krecek_png,
	},
};

#define OBSTACLE_SPEED (TFT_WIDTH / 5.0f)

#define NUM_OBSTACLES 6
static sdk_sprite_t obstacles[NUM_OBSTACLES] = {
	{
		.y = 18,
		.ts = &ts_walls_png,
	},
	{
		.y = 18 + 30,
		.ts = &ts_walls_png,
	},
	{
		.y = 18 + 60,
		.ts = &ts_walls_png,
	},
	{
		.y = 18,
		.x = TFT_WIDTH,
		.ts = &ts_walls_png,
	},
	{
		.y = 18 + 30,
		.x = TFT_WIDTH,
		.ts = &ts_walls_png,
	},
	{
		.y = 18 + 60,
		.x = TFT_WIDTH,
		.ts = &ts_walls_png,
	},

};

void game_reset(void)
{
	for (int i = 0; i < NUM_OBSTACLES; i++) {
		obstacles[i].tile = rand() % ts_walls_png.count;
		obstacles[i].x = TFT_WIDTH + (rand() % TFT_WIDTH);
	}
	game.score = 0;
	k.s.y = 18 + 30;
	k.pt = -5.0f;
}

void game_input(unsigned __unused dt_usec)
{
	if (sdk_inputs_delta.x > 0 && k.s.y < 18 + 30 + 30) {
		k.s.y += 30;
	}
	if (sdk_inputs_delta.y > 0 && k.s.y >= 18 + 30) {
		k.s.y -= 30;
	}
}

void game_start(void)
{
}

void game_paint(unsigned __unused dt_usec)
{
	float __unused dt = dt_usec / 1000000.0f;

	tft_fill(0);

	game.score = game.score + dt;

	sdk_draw_tile(0, 0, &ts_back_png, 0);

	k.s.tile = (time_us_32() >> 18) & 1;
	sdk_draw_sprite(&k.s);

	for (int i = 0; i < NUM_OBSTACLES; i++) {
		obstacles[i].x -= OBSTACLE_SPEED * dt;

		if (obstacles[i].x < -obstacles[i].ts->width) {
			// Completely out of screen.
			obstacles[i].tile = rand() % ts_walls_png.count;
			obstacles[i].x = TFT_WIDTH + (rand() % TFT_WIDTH);
		}

		//sdk_draw_sprite(&obstacles[i]);

		if (sdk_sprites_collide(&k.s, &obstacles[i]) && k.pt >= 0) {
			game_reset();
		}
	}

	char buf[16];
	snprintf(buf, sizeof buf, "%6.1f", game.score);
	tft_draw_string(0, 0, RED, buf);
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
