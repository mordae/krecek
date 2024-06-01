#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>

#include <sdk.h>
#include <tft.h>

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

struct hamster {
	float y;
	float dy;
	uint8_t color;
	float px, py;
	int hp;
};

#define WALL_HEIGHT 32
#define WALL_WIDTH 4

struct wall {
	float y;
	float dy;
};

static struct hamster p1, p2;
static struct wall wall;

struct effect;

typedef int16_t (*effect_gen_fn)(struct effect *eff);

struct effect {
	int offset;
	int length;
	int volume;
	int period;
	effect_gen_fn generator;
};

#define MAX_EFFECTS 8
struct effect effects[MAX_EFFECTS];

static int16_t __unused square_wave(struct effect *eff)
{
	if ((eff->offset % eff->period) < (eff->period / 2))
		return eff->volume;
	else
		return -eff->volume;
}

static int16_t __unused noise(struct effect *eff)
{
	return rand() % (2 * eff->volume) - eff->volume;
}

uint32_t heart_sprite[32] = {
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00011100011100000000000000000000, /* do not wrap please */
	0b00111110111110000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b01111111111111000000000000000000, /* do not wrap please */
	0b00111111111110000000000000000000, /* do not wrap please */
	0b00011111111100000000000000000000, /* do not wrap please */
	0b00001111111000000000000000000000, /* do not wrap please */
	0b00000111110000000000000000000000, /* do not wrap please */
	0b00000011100000000000000000000000, /* do not wrap please */
	0b00000001000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
	0b00000000000000000000000000000000, /* do not wrap please */
};

static void draw_sprite(int x0, int y0, uint32_t sprite[32], int color, bool transp)
{
	int x1 = x0 + 32;
	int y1 = y0 + 32;

	for (int y = y0; y < y1; y++) {
		for (int x = x0; x < x1; x++) {
			bool visible = (sprite[(y - y0)] << (x - x0)) >> 31;

			if (!visible && transp)
				continue;

			int c = color * visible;
			tft_draw_pixel(x, y, c);
		}
	}
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int nsamples)
{
	for (int s = 0; s < nsamples; s++) {
		int sample = 0;

		for (int i = 0; i < MAX_EFFECTS; i++) {
			struct effect *e = effects + i;

			if (!e->volume)
				continue;

			sample += e->generator(e);

			if (e->offset++ >= e->length)
				e->volume = 0;
		}

		sdk_write_sample(sample);
	}
}

static void __unused play_effect(int volume, int frequency, int length, effect_gen_fn gen)
{
	for (int i = 0; i < MAX_EFFECTS; i++) {
		struct effect *e = effects + i;

		if (e->volume)
			continue;

		e->offset = 0;
		e->volume = volume;
		e->length = length;
		e->period = frequency ? 48000 / frequency : 1;
		e->generator = gen;

		break;
	}
}

void game_reset(void)
{
	p1.color = RED;
	p1.dy = 0;
	p1.y = TFT_HEIGHT - 31;
	p1.px = -1;
	p1.py = -1;
	p1.hp = 3;

	p2.color = GREEN;
	p2.dy = 0;
	p2.y = TFT_HEIGHT - 31;
	p2.px = -1;
	p2.py = -1;
	p2.hp = 3;

	wall.y = TFT_HEIGHT / 2;
	wall.dy = -20.0f;
}

void game_input(unsigned __unused dt_usec)
{
	if (sdk_inputs_delta.vol_up > 0) {
		sdk_set_backlight(
			clamp(sdk_config.backlight * 2, SDK_BACKLIGHT_MIN, SDK_BACKLIGHT_MAX));
		printf("backlight: %u\n", sdk_config.backlight);
	}

	if (sdk_inputs_delta.vol_down > 0) {
		sdk_set_backlight(
			clamp(sdk_config.backlight / 2, SDK_BACKLIGHT_MIN, SDK_BACKLIGHT_MAX));
		printf("backlight: %u\n", sdk_config.backlight);
	}
}

void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	tft_fill(0);

	float bottom = TFT_HEIGHT - 31;

	/*
	 * Draw hamsters
	 */

	tft_draw_rect(0, p1.y, 23, p1.y + 31, p1.color);
	tft_draw_rect(TFT_WIDTH - 24, p2.y, TFT_WIDTH - 1, p2.y + 31, p2.color);

	/*
	 * Draw hearts
	 */

	for (int i = 0; i < p1.hp; i++)
		draw_sprite(28 + 16 * i, 4, heart_sprite, RED, true);

	for (int i = 0; i < p2.hp; i++)
		draw_sprite(TFT_WIDTH - 17 - (28 + 16 * i), 4, heart_sprite, GREEN, true);

	/*
	 * Draw wall 
	 */

	tft_draw_rect(TFT_WIDTH / 2 - WALL_WIDTH / 2,
		      wall.y - WALL_HEIGHT / 2,
		      TFT_WIDTH / 2 + WALL_WIDTH / 2,
		      wall.y + WALL_HEIGHT / 2,
		      WHITE);

	/*
	 * Projectile-wall collissions
	 */

	float wall_top = wall.y - WALL_HEIGHT / 2;
	float wall_bottom = wall.y + WALL_HEIGHT / 2;
	float wall_left = TFT_WIDTH / 2 - WALL_WIDTH / 2;
	float wall_right = TFT_WIDTH / 2 + WALL_WIDTH / 2;

	if (p1.px >= 0) {
		if (p1.py >= wall_top && p1.py <= wall_bottom &&
		    p1.px >= wall_left && p1.px <= wall_right)
		{
			p1.px = -1;
			play_effect(INT16_MAX / 5, 0, 6000, noise);
		}
	
	}
	
	if (p2.px >= 0) {
		if (p2.py >= wall_top && p2.py <= wall_bottom &&
		    p2.px >= wall_left && p2.px <= wall_right)
		{
			p2.px = -1;
			play_effect(INT16_MAX / 5, 0, 6000, noise);
		}
	}

	/*
	 * Wall movement
	 */
	wall.y += wall.dy * dt;

	if (wall.y - WALL_HEIGHT / 2 <= 0) {
		wall.dy *= -1;
	}

	if (wall.y + WALL_HEIGHT / 2 >= TFT_BOTTOM) {
		wall.dy *= -1;
	}

	/*
	 * Jumping
	 */

	if ((p1.y >= TFT_HEIGHT - 31) && sdk_inputs.a)
		p1.dy = -TFT_HEIGHT * 1.15;

	if ((p1.px < 0) && sdk_inputs.x) {
		p1.px = 24;
		p1.py = p1.y + 16;
		play_effect(INT16_MAX / 5, 440, 6000, square_wave);
	}

	if ((p2.y >= TFT_HEIGHT - 31) && sdk_inputs.y)
		p2.dy = -TFT_HEIGHT * 1.15;

	if ((p2.px < 0) && sdk_inputs.b) {
		p2.px = TFT_WIDTH - 25;
		p2.py = p2.y + 16;
		play_effect(INT16_MAX / 5, 440, 6000, square_wave);
	}

	/*
	 * Vertical movement
	 */

	p1.y += p1.dy * dt;
	p2.y += p2.dy * dt;

	/*
	 * Gravitation
	 */

	p1.dy += (float)TFT_HEIGHT * dt;
	p2.dy += (float)TFT_HEIGHT * dt;

	/*
	 * Fall boosting
	 */

	if (p1.dy > 0 && sdk_inputs.a) {
		p1.dy += (float)TFT_HEIGHT * dt;
	}

	if (p2.dy > 0 && sdk_inputs.y) {
		p2.dy += (float)TFT_HEIGHT * dt;
	}

	/*
	 * Cap acceleration and keep hamsters above floor
	 */

	if (p1.dy > TFT_HEIGHT)
		p1.dy = TFT_HEIGHT;

	if (p2.dy > TFT_HEIGHT)
		p2.dy = TFT_HEIGHT;

	if (p1.y >= bottom)
		p1.y = bottom;

	if (p2.y >= bottom)
		p2.y = bottom;

	/*
	 * Draw projectiles
	 */

	if (p1.px >= 0)
		tft_draw_rect(p1.px - 1, p1.py - 1, p1.px + 1, p1.py + 1, p1.color);

	if (p2.px >= 0)
		tft_draw_rect(p2.px - 1, p2.py - 1, p2.px + 1, p2.py + 1, p2.color);

	/*
	 * Mid-air projectile collissions
	 */

	if (p1.px >= 0 && p2.px >= 0) {
		if ((p1.py <= p2.py + 1) && (p1.py >= p2.py - 1)) {
			/* Projectiles are at about the same height. */

			if (p1.px >= p2.px) {
				/* They must have collided. */
				p1.px = -1;
				p2.px = -1;

				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}
	}

	/*
	 * Horizontal projectile movement
	 */

	float pdistance = 0.5 * (float)TFT_WIDTH * dt;

	if (p1.px >= 0)
		p1.px += pdistance;

	if (p2.px >= 0)
		p2.px -= pdistance;

	if (p1.px >= TFT_WIDTH)
		p1.px = -1;

	if (p2.px < 0)
		p2.px = -1;

	/*
	 * Projectile-hamster collissions
	 */

	if (p1.px >= 0) {
		if (p1.py >= p2.y && p1.py < (p2.y + 32)) {
			if (p1.px >= TFT_WIDTH - 24) {
				p1.px = -1;
				p2.hp -= 1;

				play_effect(INT16_MAX / 5, 220, 12000, square_wave);

				if (p2.hp < 1)
					game_reset();
			}
		}
	}

	if (p2.px >= 0) {
		if (p2.py >= p1.y && p2.py < (p1.y + 32)) {
			if (p2.px < 24) {
				p2.px = -1;
				p1.hp -= 1;

				play_effect(INT16_MAX / 5, 220, 12000, square_wave);

				if (p1.hp < 1)
					game_reset();
			}
		}
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
