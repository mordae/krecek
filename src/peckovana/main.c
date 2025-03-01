#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

#include <left-hamster.png.h>
#include <right-hamster.png.h>
#include <bullets.png.h>
#include <hearts.png.h>
#include <powerups.png.h>
#include <background-00.png.h>
#include <background-01.png.h>
#include <background-02.png.h>
#include <background-03.png.h>
#include <background-04.png.h>
#include <menu-button.png.h>

// Colors
#define RED rgb_to_rgb565(255, 0, 0)
#define RED_POWER rgb_to_rgb565(255, 63, 63)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define GREEN_POWER rgb_to_rgb565(63, 255, 63)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define NUM_BACKGROUNDS 5

const color_t *backgrounds[NUM_BACKGROUNDS] = {
	image_background_00_png.data, image_background_01_png.data, image_background_02_png.data,
	image_background_03_png.data, image_background_04_png.data,
};

// Bullets deffine
#define MAX_BULLETS 2
#define BULLET_SPEED (TFT_WIDTH / 2.0)
struct bullet {
	float dx;
	bool spawned;
	sdk_sprite_t s;
};

// Hamsters deffine
#define HAMSTER_SECOND_BULLET_TIME 10.0f
#define HAMSTER_DEATH_DELTA_TIME 3.0f
struct hamster {
	float y;
	float dy;
	float ddt; //ddt (death delta time)
	int hp;
	int hp_style;
	int max_bullets;
	float second_bullet_time;

	sdk_sprite_t s;

	struct bullet bullets[MAX_BULLETS];
};

static struct hamster p1 = {
	.s = {
		.ts = &ts_left_hamster_png,
		.tile = 0,
		.ox = 0,
		.oy = 0,
	},
};

static struct hamster p2 = {
	.s = {
		.ts = &ts_right_hamster_png,
		.tile = 0,
		.ox = 8,
		.oy = 0,
	},
};

// Wall deffine
#define WALL_HEIGHT 24
#define WALL_WIDTH 4
struct wall {
	float y;
	float dy;
};

static struct wall wall;

// Power UP deffine
#define POWER_UP_SIZE 15
#define POWER_UP_SPAWN_TIME 2.0f
#define POWER_UP_RESPAWN_TIME 15.0f
#define POWER_UP_MIN (16 + POWER_UP_SIZE)
#define POWER_UP_MAX (TFT_BOTTOM - POWER_UP_SIZE)

struct power_up {
	float spawn_time;
	sdk_sprite_t s;
};

static struct power_up power_up;
// Second Bullet deffine
#define SECOND_BULLET_SIZE 15
#define SECOND_BULLET_SPAWN_TIME 2.0f
#define SECOND_BULLET_RESPAWN_TIME 15.0f
#define SECOND_BULLET_MIN (16 + POWER_UP_SIZE)
#define SECOND_BULLET_MAX (TFT_BOTTOM - POWER_UP_SIZE)

struct second_bullet {
	float spawn_time;
	sdk_sprite_t s;
};
static struct second_bullet second_bullet;

// menu define
struct menu {
	int game_mode;
};

static struct menu menu;

// Ambiance
static int day = -1;

//Audio
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

void game_start(void)
{
	sdk_set_output_gain_db(6);
	//menu.game_mode = 3;
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
	//Players set
	p1.dy = 0;
	p1.ddt = 0;
	p1.y = TFT_HEIGHT - 31;
	p1.s.y = p1.y;
	p1.s.x = 0;
	p1.s.tile = 0;
	p1.hp = 3;
	p1.max_bullets = 1;
	p1.second_bullet_time = 0;
	p1.hp_style = 0;

	p2.dy = 0;
	p2.ddt = 0;
	p2.y = TFT_HEIGHT - 31;
	p2.s.y = p2.y;
	p2.s.x = TFT_RIGHT - 23;
	p2.s.tile = 0;
	p2.hp = 3;
	p2.max_bullets = 1;
	p2.second_bullet_time = 0;
	p2.hp_style = 2;

	//Bullets define
	for (int i = 0; i < MAX_BULLETS; i++) {
		p1.bullets[i].spawned = false;
		p2.bullets[i].spawned = false;

		p1.bullets[i].s = (sdk_sprite_t){
			.ts = &ts_bullets_png,
			.ox = 2,
			.oy = 2,
			.tile = 0,
		};

		p2.bullets[i].s = (sdk_sprite_t){
			.ts = &ts_bullets_png,
			.ox = 2,
			.oy = 2,
			.tile = 2,
		};
	}

	//wall define
	wall.y = TFT_HEIGHT / 2.0;
	wall.dy = -20.0f;

	//Power UP define
	power_up.s = (sdk_sprite_t){
		.ts = &ts_powerups_png,
		.tile = 0,
		.x = TFT_WIDTH / 2.0f,
		.y = POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) * (rand() / (float)RAND_MAX),
		.ox = 7,
		.oy = 7,
	};
	power_up.spawn_time = POWER_UP_SPAWN_TIME;

	second_bullet.s = (sdk_sprite_t){
		.ts = &ts_powerups_png,
		.tile = 1,
		.x = TFT_WIDTH / 2.0f,
		.y = SECOND_BULLET_MIN +
		     (SECOND_BULLET_MAX - SECOND_BULLET_MIN) * (rand() / (float)RAND_MAX),
		.ox = 7,
		.oy = 7,
	};
	second_bullet.spawn_time = SECOND_BULLET_SPAWN_TIME;

	day = day + 1;
	if (day > ts_bullets_png.count) {
		day = 0;
		menu.game_mode = menu.game_mode + 1;
		if (menu.game_mode == 4)
			menu.game_mode = 1;
	};
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

	// Shooting
#if 1
	if (sdk_inputs_delta.x > 0 && p1.hp > 0) {
		for (int i = 0; i < p1.max_bullets; i++) {
			if (p1.bullets[i].spawned)
				continue;

			p1.bullets[i].spawned = true;
			p1.bullets[i].s.x = 24;
			p1.bullets[i].s.y = p1.y + 16;
			p1.bullets[i].dx = BULLET_SPEED;
			play_effect(INT16_MAX / 5, 440, 6000, square_wave);
			break;
		}
	}

	if (sdk_inputs_delta.b > 0 && p2.hp > 0) {
		for (int i = 0; i < p2.max_bullets; i++) {
			if (p2.bullets[i].spawned)
				continue;

			p2.bullets[i].spawned = true;
			p2.bullets[i].s.x = TFT_RIGHT - 24;
			p2.bullets[i].s.y = p2.y + 16;
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

	tft_draw_sprite(0, 0, TFT_WIDTH, TFT_HEIGHT, backgrounds[day % NUM_BACKGROUNDS],
			TRANSPARENT);

	float bottom = TFT_HEIGHT - 31;

	//				--- Wall --- wall ---
	// Draw wall
	int wall_x = TFT_WIDTH / 2 - WALL_WIDTH / 2;
	int wall_y = wall.y - WALL_HEIGHT / 2.0;
	if (day == 0)
		wall_y = 25;
	if (menu.game_mode != 2)
		tft_draw_rect(wall_x, wall_y, wall_x + WALL_WIDTH, wall_y + WALL_HEIGHT, WHITE);

	// Wall movement
	if (day != 0 && menu.game_mode != 2)
		wall.y += wall.dy * dt;

	if (wall.y - WALL_HEIGHT / 2.0 <= 0) {
		wall.dy = fabs(wall.dy);
	}

	if (wall.y + WALL_HEIGHT / 2.0 >= TFT_BOTTOM) {
		wall.dy = -fabs(wall.dy);
	}

	// Projectile-wall collissions
	float wall_top = wall.y - WALL_HEIGHT / 2.0;
	float wall_bottom = wall.y + WALL_HEIGHT / 2.0;
	float wall_left = TFT_WIDTH / 2.0 - WALL_WIDTH / 2.0;
	float wall_right = TFT_WIDTH / 2.0 + WALL_WIDTH / 2.0;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned) {
			if (p1.bullets[i].s.y >= wall_top && p1.bullets[i].s.y <= wall_bottom &&
			    p1.bullets[i].s.x >= wall_left && p1.bullets[i].s.x <= wall_right) {
				p1.bullets[i].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}

		if (p2.bullets[i].spawned) {
			if (p2.bullets[i].s.y >= wall_top && p2.bullets[i].s.y <= wall_bottom &&
			    p2.bullets[i].s.x >= wall_left && p2.bullets[i].s.x <= wall_right) {
				p2.bullets[i].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}
	}

	//				--- GUI --- gui ---
	// Draw hearts
	for (int i = 0; i < p1.hp; i++)
		sdk_draw_tile(28 + 16 * i, 4, &ts_hearts_png, p1.hp_style);

	for (int i = 0; i < p2.hp; i++)
		sdk_draw_tile(TFT_WIDTH - 17 - (28 + 16 * i), 4, &ts_hearts_png, p2.hp_style);

	if (day == 0)
		sdk_draw_tile(TFT_RIGHT / 2 - 25, TFT_BOTTOM - 25, &ts_menu_button_png,
			      menu.game_mode);
#if 0
	if (day == 0) {
		for (int i = 0; i < 4; i++) {
			sdk_draw_tile(TFT_RIGHT / 2 - 25, TFT_BOTTOM - 25 - i * 15, &ts_menubutton,
				      i);
		}
	}
#endif
#if 1

	if (menu.game_mode == 3) {
		if (p1.hp > 0)
			p1.hp = 1;
		if (p2.hp > 0)
			p2.hp = 1;
	};
#endif

	//				--- Power UP --- power up ---
	// --- Heal Boost ---
	if (power_up.spawn_time <= 0 && day != 0 && menu.game_mode == 1) {
		// Draw power_up
		sdk_draw_sprite(&power_up.s);

		// Bullet/power-up collisions
		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p1.bullets[i].spawned &&
			    sdk_sprites_collide(&p1.bullets[i].s, &power_up.s)) {
				// Bullet of P1 hit power up
				p1.bullets[i].spawned = false;
				p1.hp = clamp(p1.hp + 1, 0, 3);
				power_up.spawn_time = POWER_UP_RESPAWN_TIME;
				power_up.s.y = POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) *
								      (rand() / (float)RAND_MAX);
				play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				break;
			}

			if (p2.bullets[i].spawned &&
			    sdk_sprites_collide(&p2.bullets[i].s, &power_up.s)) {
				// Bullet of P2 hit power up
				p2.bullets[i].spawned = false;
				p2.hp = clamp(p2.hp + 1, 0, 3);
				power_up.spawn_time = POWER_UP_RESPAWN_TIME;
				power_up.s.y = POWER_UP_MIN + (POWER_UP_MAX - POWER_UP_MIN) *
								      (rand() / (float)RAND_MAX);
				play_effect(INT16_MAX / 5, 880, 6000, square_wave);
				break;
			}
		}
	}

	power_up.spawn_time -= dt;

	// --- Second Bullet ---
	if (second_bullet.spawn_time <= 0 && day != 0 && menu.game_mode != 2) {
		// Draw second bullet pickable
		sdk_draw_sprite(&second_bullet.s);

		// Bullet/power-up collisions
		for (int i = 0; i < MAX_BULLETS; i++) {
			if (p1.bullets[i].spawned &&
			    sdk_sprites_collide(&p1.bullets[i].s, &second_bullet.s)) {
				// Bullet of P1 hit "second bullet" pickable
				p1.bullets[i].spawned = false;
				p1.max_bullets = 2;
				p1.second_bullet_time = HAMSTER_SECOND_BULLET_TIME;
				second_bullet.spawn_time = SECOND_BULLET_RESPAWN_TIME;
				second_bullet.s.y = SECOND_BULLET_MIN +
						    (SECOND_BULLET_MAX - SECOND_BULLET_MIN) *
							    (rand() / (float)RAND_MAX);
				play_effect(INT16_MAX / 5, 880, 6000, square_wave);
			}

			if (p2.bullets[i].spawned &&
			    sdk_sprites_collide(&p2.bullets[i].s, &second_bullet.s)) {
				// Bullet of P2 hit "second bullet" pickable
				p2.bullets[i].spawned = false;
				p2.max_bullets = 2;
				p2.second_bullet_time = HAMSTER_SECOND_BULLET_TIME;
				second_bullet.spawn_time = SECOND_BULLET_RESPAWN_TIME;
				second_bullet.s.y = SECOND_BULLET_MIN +
						    (SECOND_BULLET_MAX - SECOND_BULLET_MIN) *
							    (rand() / (float)RAND_MAX);
				play_effect(INT16_MAX / 5, 880, 6000, square_wave);
			}
		}
	}
	//	int cnt = ts_background.last + 1;
	//Second Bullet delta time
	second_bullet.spawn_time -= dt;

	if (second_bullet.spawn_time < 0) {
		second_bullet.spawn_time = 0;
	}

	if (p1.second_bullet_time >= 0) {
		p1.second_bullet_time -= dt;
		p1.max_bullets = 1;
		p1.hp_style = 0;
		for (int i = 0; i < MAX_BULLETS; i++)
			p1.bullets[i].s.tile = 0;
	}

	if (p2.second_bullet_time >= 0) {
		p2.second_bullet_time -= dt;
		p2.max_bullets = 1;
		p2.hp_style = 2;
		for (int i = 0; i < MAX_BULLETS; i++)
			p2.bullets[i].s.tile = 2;
	}

	if (p1.second_bullet_time > 1) {
		p1.max_bullets = 2;
		p1.hp_style = 1;
		for (int i = 0; i < MAX_BULLETS; i++)
			p1.bullets[i].s.tile = 1;
	}

	if (p2.second_bullet_time > 1) {
		p2.max_bullets = 2;
		p2.hp_style = 3;
		for (int i = 0; i < MAX_BULLETS; i++)
			p2.bullets[i].s.tile = 3;
	}

	//				--- Hamsters --- hamsters ---
	// Draw hamsters
	sdk_draw_sprite(&p1.s);
	sdk_draw_sprite(&p2.s);

	// Projectile-hamster collissions
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned && sdk_sprites_collide(&p1.bullets[i].s, &p2.s)) {
			p1.bullets[i].spawned = false;
			p2.hp -= 1;

			play_effect(INT16_MAX / 5, 220, 12000, square_wave);

			if (p2.hp <= 0) {
				p2.ddt = HAMSTER_DEATH_DELTA_TIME;
				p2.hp = -1;
			}
		}

		if (p2.bullets[i].spawned && sdk_sprites_collide(&p2.bullets[i].s, &p1.s)) {
			p2.bullets[i].spawned = false;
			p1.hp -= 1;

			play_effect(INT16_MAX / 5, 220, 12000, square_wave);

			if (p1.hp <= 0) {
				p1.ddt = HAMSTER_DEATH_DELTA_TIME;
				p1.hp = -1;
			}
		}
	}

	// Dead timeout
	if (p2.ddt > 0) {
		p2.ddt -= dt;
		p2.s.tile = 3;
	}

	if (p2.ddt > 0 && p2.ddt < 0.1) {
		game_reset();
		p2.hp = 3;
	}

	if (p1.ddt > 0) {
		p1.ddt -= dt;
		p1.s.tile = 3;
	}

	if (p1.ddt > 0 && p1.ddt < 0.1) {
		game_reset();
		p1.hp = 3;
	}

	//			--- Movement --- movement ---
	// Jumping
	if ((p1.y >= TFT_HEIGHT - 31) && sdk_inputs.a && p1.hp > 0) {
		p1.dy = -TFT_HEIGHT * 1.15;
		if (p1.hp > 0)
			p1.s.tile = 1;
	}

	if ((p2.y >= TFT_HEIGHT - 31) && sdk_inputs.y && p2.hp > 0) {
		p2.dy = -TFT_HEIGHT * 1.15;
		if (p2.hp > 0)
			p2.s.tile = 1;
	}

	// Vertical movement
	p1.y += p1.dy * dt;
	p1.s.y = p1.y;

	p2.y += p2.dy * dt;
	p2.s.y = p2.y;

	// Gravitation
	p1.dy += (float)TFT_HEIGHT * dt;
	p2.dy += (float)TFT_HEIGHT * dt;

	// Fall boosting
	if (p1.dy > 0 && sdk_inputs.a && p1.hp > 0) {
		p1.dy += (float)TFT_HEIGHT * dt;
		p1.s.tile = 2;
	}

	if (p2.dy > 0 && sdk_inputs.y && p2.hp > 0) {
		p2.dy += (float)TFT_HEIGHT * dt;
		p2.s.tile = 2;
	}

	// Cap acceleration and keep hamsters above floor
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

	//	 --- Hands ---
	// Neutral hands
	if (p1.s.y > TFT_BOTTOM - 32 && p1.hp > 0)
		p1.s.tile = 0;

	if (p2.s.y > TFT_BOTTOM - 32 && p2.hp > 0)
		p2.s.tile = 0;

	// Hands up
	if (p1.y < 9 && p1.hp > 0)
		p1.s.tile = 2;

	if (p2.y < 9 && p2.hp > 0)
		p2.s.tile = 2;

	//				--- Projectiles --- projectiles ---
	// Draw projectiles
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (p1.bullets[i].spawned)
			sdk_draw_sprite(&p1.bullets[i].s);

		if (p2.bullets[i].spawned)
			sdk_draw_sprite(&p2.bullets[i].s);
	}

	// Horizontal projectile movement
	for (int i = 0; i < MAX_BULLETS; i++) {
		p1.bullets[i].s.x += p1.bullets[i].dx * dt;
		p2.bullets[i].s.x += p2.bullets[i].dx * dt;

		if (p1.bullets[i].s.x > TFT_RIGHT)
			p1.bullets[i].spawned = false;

		if (p2.bullets[i].s.x < 0)
			p2.bullets[i].spawned = false;
	}

	// Mid-air projectile collissions
	for (int i = 0; i < MAX_BULLETS; i++) {
		for (int j = 0; j < MAX_BULLETS; j++) {
			if (!p1.bullets[i].spawned)
				continue;

			if (!p2.bullets[j].spawned)
				continue;

			if (sdk_sprites_collide(&p1.bullets[i].s, &p2.bullets[j].s)) {
				p1.bullets[i].spawned = false;
				p2.bullets[j].spawned = false;
				play_effect(INT16_MAX / 5, 0, 6000, noise);
			}
		}
	}
	/*
	 * Game mods
	 *  Base Game = 1
	 *  No Walls and Power up = 2
	 *  Low gravity
	 *  One HP =3 
	 */
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
