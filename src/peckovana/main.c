#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

embed_tileset(ts_left_hamster, 4, 24, 32, 237, "left.data");
embed_tileset(ts_right_hamster, 4, 24, 32, 237, "right.data");

#define RED 255
#define RED_POWER (RED - 31)
#define YELLOW 242
#define GREEN 244
#define GREEN_POWER (GREEN - 32)
#define BLUE 250
#define GRAY 8
#define WHITE 15

#define MAX_BULLETS 2
#define BULLET_SPEED (TFT_WIDTH / 2)

struct bullet {
	float x, y;
	float dx;
	bool spawned;
};

#define HAMSTER_SECOND_BULLET_TIME 10.0f

struct hamster {
	float y;
	float dy;
	uint8_t color;
	int hp;
	int max_bullets;
	float second_bullet_time;

	sdk_sprite_t s;

	struct bullet bullets[MAX_BULLETS];
};

#define WALL_HEIGHT 32
#define WALL_WIDTH 4

struct wall {
	float y;
	float dy;
};

#define POWER_UP_LENGTH 15
#define POWER_UP_SPAWN_TIME 2.0f
#define POWER_UP_RESPAWN_TIME 15.0f
#define POWER_UP_MIN (16 + POWER_UP_LENGTH)
#define POWER_UP_MAX (TFT_BOTTOM - POWER_UP_LENGTH)

struct power_up {
	float y;
	float spawn_time;
};

#define SECOND_BULLET_LENGTH 15
#define SECOND_BULLET_SPAWN_TIME 2.0f
#define SECOND_BULLET_RESPAWN_TIME 15.0f
#define SECOND_BULLET_MIN (16 + POWER_UP_LENGTH)
#define SECOND_BULLET_MAX (TFT_BOTTOM - POWER_UP_LENGTH)

struct second_bullet {
	float y;
	float spawn_time;
};

static struct hamster p1 = {
	.s = {
		.ts = &ts_left_hamster,
		.tile = 0,
		.ox = 0,
		.oy = 0,
	},
};

static struct hamster p2 = {
	.s = {
		.ts = &ts_right_hamster,
		.tile = 0,
		.ox = 0,
		.oy = 0,
	},
};

static struct wall wall;
static struct power_up power_up;
static struct second_bullet second_bullet;

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
	p1.s.y = p1.y;
	p1.s.x = 0;
	p1.s.tile = 0;
	p1.hp = 3;
	p1.max_bullets = 1;
	p1.second_bullet_time = 0;

	p2.color = GREEN;
	p2.dy = 0;
	p2.y = TFT_HEIGHT - 31;
	p2.s.y = p2.y;
	p2.s.x = TFT_RIGHT - 23;
	p2.s.tile = 0;
	p2.hp = 3;
	p2.max_bullets = 1;
	p2.second_bullet_time = 0;

	for (int i = 0; i < MAX_BULLETS; i++) {
		p1.bullets[i].spawned = false;
		p2.bullets[i].spawned = false;
	}

	wall.y = TFT_HEIGHT / 2;
	wall.dy = -20.0f;

	power_up.y = POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) * ((float)rand() / RAND_MAX);
	power_up.spawn_time = POWER_UP_SPAWN_TIME;

	second_bullet.y = SECOND_BULLET_MIN +
			  (SECOND_BULLET_MAX - SECOND_BULLET_MIN) * ((float)rand() / RAND_MAX);
	second_bullet.spawn_time = SECOND_BULLET_SPAWN_TIME;
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

	/*
	 * Shooting
	 */

#if 1
	if (sdk_inputs_delta.x > 0) {
		for (int i = 0; i < p1.max_bullets; i++) {
			if (p1.bullets[i].spawned)
				continue;

			p1.bullets[i].spawned = true;
			p1.bullets[i].x = 24;
			p1.bullets[i].y = p1.y + 16;
			p1.bullets[i].dx = BULLET_SPEED;
			play_effect(INT16_MAX / 5, 440, 6000, square_wave);
			break;
		}
	}

	if (sdk_inputs_delta.b > 0) {
		for (int i = 0; i < p2.max_bullets; i++) {
			if (p2.bullets[i].spawned)
				continue;

			p2.bullets[i].spawned = true;
			p2.bullets[i].x = TFT_RIGHT - 24;
			p2.bullets[i].y = p2.y + 16;
			p2.bullets[i].dx = -BULLET_SPEED;
			play_effect(INT16_MAX / 5, 440, 6000, square_wave);
			break;
		}
	}

#endif
#if 0
	// Laser krecek

	if (sdk_inputs.x > 0) {
		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p1.bullets[i].spawned)
				continue;

			p1.bullets[i].spawned = true;
			p1.bullets[i].x = 24;
			p1.bullets[i].y = p1.y + 16;
			p1.bullets[i].dx = BULLET_SPEED;
			play_effect(INT16_MAX / 5, 440, 6000, square_wave);
			break;
		}
	}

	if (sdk_inputs.b > 0) {
		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p2.bullets[i].spawned)
				continue;

			p2.bullets[i].spawned = true;
			p2.bullets[i].x = TFT_RIGHT - 24;
			p2.bullets[i].y = p2.y + 16;
			p2.bullets[i].dx = -BULLET_SPEED;
			play_effect(INT16_MAX / 5, 440, 6000, square_wave);
			break;
		}
	}
#endif
}

void game_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	tft_fill(0);

	float bottom = TFT_HEIGHT - 31;

	/*
	 * Draw hamsters
	 */

	//tft_draw_rect(0, p1.y, 23, p1.y + 31, p1.color);
	sdk_draw_sprite(&p1.s);

	sdk_draw_sprite(&p2.s);

	//tft_draw_rect(TFT_WIDTH - 24, p2.y, TFT_WIDTH - 1, p2.y + 31, p2.color);

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

	tft_draw_rect(TFT_WIDTH / 2 - WALL_WIDTH / 2, wall.y - WALL_HEIGHT / 2,
		      TFT_WIDTH / 2 + WALL_WIDTH / 2, wall.y + WALL_HEIGHT / 2, WHITE);

	/*
	 * Projectile-wall collissions
	 */

	float wall_top = wall.y - WALL_HEIGHT / 2;
	float wall_bottom = wall.y + WALL_HEIGHT / 2;
	float wall_left = TFT_WIDTH / 2 - WALL_WIDTH / 2;
	float wall_right = TFT_WIDTH / 2 + WALL_WIDTH / 2;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned) {
			if (p1.bullets[i].y >= wall_top && p1.bullets[i].y <= wall_bottom &&
			    p1.bullets[i].x >= wall_left && p1.bullets[i].x <= wall_right) {
				p1.bullets[i].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}

		if (p2.bullets[i].spawned) {
			if (p2.bullets[i].y >= wall_top && p2.bullets[i].y <= wall_bottom &&
			    p2.bullets[i].x >= wall_left && p2.bullets[i].x <= wall_right) {
				p2.bullets[i].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}
	}

	/*
	 * Wall movement
	 */
	wall.y += wall.dy * dt;

	if (wall.y - WALL_HEIGHT / 2 <= 0) {
		wall.dy = fabs(wall.dy);
	}

	if (wall.y + WALL_HEIGHT / 2 >= TFT_BOTTOM) {
		wall.dy = -fabs(wall.dy);
	}

	if (power_up.spawn_time <= 0) {
		/*
		 * Draw power_up
		 */

		tft_draw_rect(TFT_WIDTH / 2 - POWER_UP_LENGTH / 2, power_up.y - POWER_UP_LENGTH / 2,
			      TFT_WIDTH / 2 + POWER_UP_LENGTH / 2, power_up.y + POWER_UP_LENGTH / 2,
			      YELLOW);

		/*
		 * projectile-power_up collisions
		 */

		float power_up_top = power_up.y - POWER_UP_LENGTH / 2;
		float power_up_bottom = power_up.y + POWER_UP_LENGTH / 2;
		float power_up_left = TFT_WIDTH / 2 - POWER_UP_LENGTH / 2;
		float power_up_right = TFT_WIDTH / 2 + POWER_UP_LENGTH / 2;

		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p1.bullets[i].spawned) {
				if (p1.bullets[i].y >= power_up_top &&
				    p1.bullets[i].y <= power_up_bottom &&
				    p1.bullets[i].x >= power_up_left &&
				    p1.bullets[i].x <= power_up_right) {
					p1.bullets[i].spawned = false;
					p1.hp = clamp(p1.hp + 1, 0, 3);
					power_up.spawn_time = POWER_UP_RESPAWN_TIME;
					power_up.y =
						POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) *
								       ((float)rand() / RAND_MAX);
					play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				}
			}

			if (p2.bullets[i].spawned) {
				if (p2.bullets[i].y >= power_up_top &&
				    p2.bullets[i].y <= power_up_bottom &&
				    p2.bullets[i].x >= power_up_left &&
				    p2.bullets[i].x <= power_up_right) {
					p2.bullets[i].spawned = false;
					p2.hp = clamp(p2.hp + 1, 0, 3);
					power_up.spawn_time = POWER_UP_RESPAWN_TIME;
					power_up.y =
						POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) *
								       ((float)rand() / RAND_MAX);
					play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				}
			}
		}
	}

	power_up.spawn_time -= dt;

	if (second_bullet.spawn_time <= 0) {
		/*
		 * Draw second_bullet
		 */

		tft_draw_rect(TFT_WIDTH / 2 - SECOND_BULLET_LENGTH / 2,
			      second_bullet.y - SECOND_BULLET_LENGTH / 2,
			      TFT_WIDTH / 2 + SECOND_BULLET_LENGTH / 2,
			      second_bullet.y + SECOND_BULLET_LENGTH / 2, BLUE);

		/*
		 * projectile-second_bullet collisions
		 */

		float second_bullet_top = second_bullet.y - POWER_UP_LENGTH / 2;
		float second_bullet_bottom = second_bullet.y + POWER_UP_LENGTH / 2;
		float second_bullet_left = TFT_WIDTH / 2 - SECOND_BULLET_LENGTH / 2;
		float second_bullet_right = TFT_WIDTH / 2 + SECOND_BULLET_LENGTH / 2;

		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p1.bullets[i].spawned) {
				if (p1.bullets[i].y >= second_bullet_top &&
				    p1.bullets[i].y <= second_bullet_bottom &&
				    p1.bullets[i].x >= second_bullet_left &&
				    p1.bullets[i].x <= second_bullet_right) {
					p1.bullets[i].spawned = false;
					p1.color = WHITE;
					p1.max_bullets = 2;
					p1.second_bullet_time = HAMSTER_SECOND_BULLET_TIME;
					second_bullet.spawn_time = SECOND_BULLET_RESPAWN_TIME;
					second_bullet.y = SECOND_BULLET_MIN +
							  (SECOND_BULLET_MAX - SECOND_BULLET_MIN) *
								  ((float)rand() / RAND_MAX);
					play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				}
			}

			if (p2.bullets[i].spawned) {
				if (p2.bullets[i].y >= second_bullet_top &&
				    p2.bullets[i].y <= second_bullet_bottom &&
				    p2.bullets[i].x >= second_bullet_left &&
				    p2.bullets[i].x <= second_bullet_right) {
					p2.bullets[i].spawned = false;
					p2.color = WHITE;
					p2.max_bullets = 2;
					p2.second_bullet_time = HAMSTER_SECOND_BULLET_TIME;

					second_bullet.spawn_time = SECOND_BULLET_RESPAWN_TIME;
					second_bullet.y = SECOND_BULLET_MIN +
							  (SECOND_BULLET_MAX - SECOND_BULLET_MIN) *
								  ((float)rand() / RAND_MAX);
					play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				}
			}
		}
	}

	second_bullet.spawn_time -= dt;

	if (second_bullet.spawn_time < 0) {
		second_bullet.spawn_time = 0;
	}

	if (p1.second_bullet_time >= 0)
		p1.second_bullet_time -= dt;
	p1.color = RED;
	p1.max_bullets = 1;

	if (p2.second_bullet_time >= 0)
		p2.second_bullet_time -= dt;
	p2.color = GREEN;
	p2.max_bullets = 1;

	if (p1.second_bullet_time > 1) {
		p1.max_bullets = 2;
		p1.color = RED - 32;
	}

	if (p2.second_bullet_time > 1) {
		p2.max_bullets = 2;
		p2.color = GREEN - 32;
	}
	/*
	 * Jumping
	 */

	if ((p1.y >= TFT_HEIGHT - 31) && sdk_inputs.a) {
		p1.dy = -TFT_HEIGHT * 1.15;
		p1.s.tile = 1;
	}
	if ((p2.y >= TFT_HEIGHT - 31) && sdk_inputs.y) {
		p2.dy = -TFT_HEIGHT * 1.15;
		p2.s.tile = 1;
	}
	/*
	 * Vertical movement
	 */

	p1.y += p1.dy * dt;
	p1.s.y = p1.y;

	p2.y += p2.dy * dt;
	p2.s.y = p2.y;

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
		p1.s.tile = 2;
	}

	if (p2.dy > 0 && sdk_inputs.y) {
		p2.dy += (float)TFT_HEIGHT * dt;
		p2.s.tile = 2;
	}

	/*
	 * Neutral hands
	 */

	if (p1.s.y > TFT_BOTTOM - 32)
		p1.s.tile = 0;

	if (p2.s.y > TFT_BOTTOM - 32)
		p2.s.tile = 0;

	/*
	 * Hands up
	 */

	if (p1.y < 9)
		p1.s.tile = 2;

	if (p2.y < 9)
		p2.s.tile = 2;

	/*
	 * Cap acceleration and keep hamsters above floor
	 */

	if (p1.dy > TFT_HEIGHT)
		p1.dy = TFT_HEIGHT;

	if (p2.dy > TFT_HEIGHT)
		p2.dy = TFT_HEIGHT;

	if (p1.y >= bottom) {
		p1.y = bottom;
		p1.s.y = p1.y;
	}

	if (p2.y >= bottom) {
		p2.y = bottom;
		p2.s.y = p2.y;
	}

	//	if (p2.y >= bottom)
	//		p2.y = bottom;

	/*
	 * Draw projectiles
	 */

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned) {
			tft_draw_rect(p1.bullets[i].x - 1, p1.bullets[i].y - 1, p1.bullets[i].x + 1,
				      p1.bullets[i].y + 1, p1.color);
		}

		if (p2.bullets[i].spawned) {
			tft_draw_rect(p2.bullets[i].x - 1, p2.bullets[i].y - 1, p2.bullets[i].x + 1,
				      p2.bullets[i].y + 1, p2.color);
		}
	}

	/*
	 * Mid-air projectile collissions
	 */

	for (int i = 0; i < MAX_BULLETS; i++) {
		for (int j = 0; j < MAX_BULLETS; j++) {
			if (!p1.bullets[i].spawned)
				continue;

			if (!p2.bullets[j].spawned)
				continue;

			float p1b_top = p1.bullets[i].y - 1;
			float p1b_bottom = p1.bullets[i].y + 1;
			float p1b_left = p1.bullets[i].x - 1;
			float p1b_right = p1.bullets[i].x + 1;

			float p2b_top = p2.bullets[j].y - 1;
			float p2b_bottom = p2.bullets[j].y + 1;
			float p2b_left = p2.bullets[j].x - 1;
			float p2b_right = p2.bullets[j].x + 1;

			if (p1b_bottom >= p2b_top && p1b_top <= p2b_bottom &&
			    p1b_right >= p2b_left && p1b_left <= p2b_right) {
				p1.bullets[i].spawned = false;
				p2.bullets[j].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}
	}

	/*
	 * Horizontal projectile movement
	 */

	for (int i = 0; i < MAX_BULLETS; i++) {
		p1.bullets[i].x += p1.bullets[i].dx * dt;
		p2.bullets[i].x += p2.bullets[i].dx * dt;

		if (p1.bullets[i].x > TFT_RIGHT)
			p1.bullets[i].spawned = false;

		if (p2.bullets[i].x < 0)
			p2.bullets[i].spawned = false;
	}

	/*
	 * Projectile-hamster collissions
	 */

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned) {
			if (p1.bullets[i].y >= p2.y && p1.bullets[i].y < (p2.y + 32)) {
				if (p1.bullets[i].x >= TFT_WIDTH - 24) {
					p1.bullets[i].spawned = false;
					p2.hp -= 1;

					play_effect(INT16_MAX / 5, 220, 12000, square_wave);

					if (p2.hp < 1)
						game_reset();
				}
			}
		}

		if (p2.bullets[i].spawned) {
			if (p2.bullets[i].y >= p1.y && p2.bullets[i].y < (p1.y + 32)) {
				if (p2.bullets[i].x < 24) {
					p2.bullets[i].spawned = false;
					p1.hp -= 1;

					play_effect(INT16_MAX / 5, 220, 12000, square_wave);

					if (p1.hp < 1)
						game_reset();
				}
			}
		}
		//printy

		//second bullet detector
		//printf("%3.3f %3.3f\n", p2.second_bullet_time, p1.second_bullet_time);

		//players names
		//puts("green = p2 // red = p1");
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
	char bulletBreak[] = "a";
	printf("%s", bulletBreak);
	sdk_main(&config);
}
