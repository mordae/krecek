#include <math.h>
#include <pico/stdlib.h>

#include <stdio.h>

#include <sdk.h>
#include <tft.h>

#include <level1.png.h>
#include <arnost.png.h>
#include <people.png.h>
#include <cover.png.h>
#include <drop.png.h>
#include <drone.png.h>

sdk_game_info("arnost", &image_cover_png);

#define BLACK rgb_to_rgb565(0, 0, 0)
#define GRAY rgb_to_rgb565(63, 63, 63)
#define WHITE rgb_to_rgb565(255, 255, 255)
// #define PINK rgb_to_rgb565(255, 0, 180)
// #define BROWN rgb_to_rgb565(60, 15, 0)
// #define AZURE rgb_to_rgb565(0, 232, 255)

/*
 *   _____                        _____            _
 *  / ____|                      |  __ \          (_)
 * | |  __  __ _ _ __ ___   ___  | |  | | ___  ___ _  __ _ _ __
 * | | |_ |/ _` | '_ ` _ \ / _ \ | |  | |/ _ \/ __| |/ _` | '_ \
 * | |__| | (_| | | | | | |  __/ | |__| |  __/\__ \ | (_| | | | |
 *  \_____|\__,_|_| |_| |_|\___| |_____/ \___||___/_|\__, |_| |_|
 *                                                    __/ |
 *                                                   |___/
 *
 * Holub Arnoš lítá pomocí joysticku vlevo, vpravo, DOLU, NAHORU.
 * Tlačítkem "a" se vykálí  ve vzduchu.
 * Dole pod Arnoštem na ulici chodí náhodně CHODCI, JEZEVČÍCI a další BYTOSTI.
 * V rámci ulice jsou zde SOCHY, TRUHLÍKY S KVĚTINAMI, KOŠ.
 * Pokud zasáhne připočítají se mu body.
 * Hra je omezena na ČAS.
 * BODOVÉ OHODNOCENÍ zásahů.
 * Arnošt může NĚCO SEZOBNOUT. -> ZVUK při sežrání.
 * KRMENÍ PRO PTÁKY -> může kálet co 1 SEKUNDU po dobu XX sekund.
 * CIGARETU
 * ŽVÝKAČKA - nemůže kálet po dobu 10 sekund
 *
 *
 * Jezevčík se pokusí sežrat Arnošta, pokud přiletí moc blízko. -> ZVUK štěkajícího jezevčíka.
 * DÍTĚ se pokusí arnošta chytit, pokud přiletí moc blízko. -> ZVUK chechtajícího se dítěte.
 * HŘIBÁTOR a PAPARATZI.
 *
 * Náhodná událost: STAR TREK - náhodně se objeví skupina ze Star Treku, budou sondovat prostředí
 * a během toho je potřeba aby, je Arnošt sestřelil. Po sestřelení se zase od teleportují. Za to bude 10 bódů a achivment.
 *
 * Náhodná událost: Objeví se drone, který lítá i nahoru a dolů. Arnošt se mu musí vyhnout. Jinak přijde o obody.
 */

// definice rozvržení paměti pro člověka

struct human {
	sdk_sprite_t s;
	float dir;
	float stop;
};

static struct human human1, human2;

static sdk_sprite_t bird;
static sdk_sprite_t drop;
static sdk_sprite_t drone;

// Speed of movement
static float bird_dir = 30;
static float drone_dir = 40;

// Globals for drop
static bool drop_active = false;
static float drop_velocity = 0;
static const float GRAVITY = 200.0f; // gravity acceleration (px/s²)

static int score = 0;

// Music:Cum decore, 1551, Tielmann Susato, arr. Jos van den Borre
static const char music1[] = "/i:flute (ppp) > /bpm:60 { "
			     "f-fg a-a- aa#Ca a#-g- g-g- g-g- gaa#g a-f- f-fg a-a- aa#Ca a#-g- "
			     "gaa#g a-gf edfe f--- C-C- a-a- D-D- C--- aa#Ca a#agf edfe f--- "
			     "C-C- a-a- D-D- C--- aa#Ca a#agf edfe f--- "
			     "}";

static const char music2[] = "/i:string (ppp) > /bpm:60 { "
			     "f-f- f-f- f-f- d-d- e-e- d-d- e-d- f-f- f-f- f-f- f-ef g-e- "
			     "d-dd f-dd c-c- c--- f-f- f-f- f-a#a g--- f-ff f-dd c-c- c--- "
			     "f-f- f-f- f-a#a g--- f-ff f-dd c-c- c--- "
			     "}";

static const char music3[] = "/i:sine (ppp) > /bpm:60 { "
			     "f-f- f-f- f-f- d-d- e-e- d-d- e-d- f-f- f-f- f-f- f-ef g-e- "
			     "d-dd f-dd c-c- c--- f-f- f-f- f-a#a g--- f-ff f-dd c-c- c--- "
			     "f-f- f-f- f-a#a g--- f-ff f-dd c-c- c--- "
			     "}";

static sdk_melody_t *melody1, *melody2, *melody3;

void game_start(void)
{
}

void game_reset(void)
{
	bird.ts = &ts_arnost_png;
	bird.tile = 0;
	bird.y = 6;
	bird.x = TFT_WIDTH / 2.0f - bird.ts->width / 2.0f;

	human1.s.ts = &ts_people_png;
	human1.s.tile = 8;
	human1.s.y = 100;
	human1.s.x = TFT_WIDTH / 2.0f - human1.s.ts->width / 2.0f;
	human1.stop = -1;
	human1.dir = 15;

	human2.s.ts = &ts_people_png;
	human2.s.tile = 4;
	human2.s.y = 80;
	human2.s.x = TFT_WIDTH / 2.0f - human2.s.ts->width / 2.0f;
	human2.stop = -1;
	human2.dir = 5;

	drop.ts = &ts_drop_png;
	drop.tile = 0;
	drop.y = 4;
	drop.x = TFT_WIDTH / 2.0f - drop.ts->width / 2.0f;
	drop.ox = 4.0f;
	drop.oy = 7.0f;

	drone.ts = &ts_drone_png;
	drone.tile = 0;
	drone.y = 10;
	drone.x = TFT_WIDTH / 2.0f - human1.s.ts->width / 2.0f;

	melody1 = sdk_melody_play_get(music1);
	melody2 = sdk_melody_play_get(music2);
	melody3 = sdk_melody_play_get(music3);
}

// Collision of objects
static bool rects_overlap(int x0, int y0, int x1, int y1, int a0, int b0, int a1, int b1)
{
	int tmp;

	if (x0 > x1)
		tmp = x1, x1 = x0, x0 = tmp;

	if (a0 > a1)
		tmp = a1, a1 = a0, a0 = tmp;

	if (y0 > y1)
		tmp = y1, y1 = y0, y0 = tmp;

	if (b0 > b1)
		tmp = b1, b1 = b0, b0 = tmp;

	if (x1 < a0)
		return false;

	if (x0 > a1)
		return false;

	if (y1 < b0)
		return false;

	if (y0 > b1)
		return false;

	return true;
}

void game_input(unsigned dt_usec)
{
	// Change microseconds to seconds.
	float dt = dt_usec / 1000000.0f;

	// Pigeon fly by his speed.
	bird.x += bird_dir * dt;

	if (sdk_inputs_delta.horizontal > 0) {
		bird_dir = 30;
	}

	if (sdk_inputs_delta.horizontal < 0) {
		bird_dir = -30;
	}

	// Change pigeon direction when hitting ends of the screen.
	if (bird.x < 0 || bird.x > TFT_RIGHT - bird.ts->width) {
		bird_dir = -bird_dir;
		printf("bird hit wall, bird_dir=%f\n", bird_dir);
	}

	bird.x = clamp(bird.x, 0, TFT_RIGHT - bird.ts->width);
	// if (bird.x < 0) {
	// 	bird.x = 0;
	// }
	//
	// if (bird.x > TFT_RIGHT) {
	// 	bird.x = TFT_RIGHT;
	// }

	// Move drone by their speed.
	drone.x += drone_dir * dt;

	// Change drone direction when hitting ends of the screen.
	if (drone.x < -20 || drone.x > TFT_RIGHT + 20 - drone.ts->width) {
		drone_dir = -drone_dir;
		printf("drone hit wall, drone_dir=%f\n", drone_dir);
	}

	drone.x = clamp(drone.x, -20, TFT_RIGHT + 20 - drone.ts->width);
	drone.y = bird.y + 18 * sinf(drone.x * 0.05f);

	// Move human by their speed, if not stopped.
	if (human1.stop < 0) {
		human1.s.x += human1.dir * dt;
	} else {
		human1.stop -= dt;
	}

	// Change human direction when hitting ends of the screen.
	if (human1.s.x < -20 || human1.s.x > TFT_RIGHT + 20 - human1.s.ts->width) {
		human1.dir = -human1.dir;
		printf("human hit wall, human_dir=%f\n", human1.dir);
	}
	human1.s.x = clamp(human1.s.x, -20, TFT_RIGHT + 20 - human1.s.ts->width);

	// Move human by their speed, if not stopped.
	if (human2.stop < 0) {
		human2.s.x += human2.dir * dt;
	} else {
		human2.stop -= dt;
	}

	// Change human direction when hitting ends of the screen.
	if (human2.s.x < -20 || human2.s.x > TFT_RIGHT + 20 - human2.s.ts->width) {
		human2.dir = -human2.dir;
		printf("human hit wall, human_dir=%f\n", human2.dir);
	}

	human2.s.x = clamp(human2.s.x, -20, TFT_RIGHT + 20 - human2.s.ts->width);

	// Make drop by push "a".
	if (sdk_inputs_delta.a > 0 && !drop_active) {
		drop.x = bird.x + 8 - sign(bird_dir) * 4; // Center of the pigeon
		drop.y = bird.y + 16;			  // Just below the pigeon
		drop_velocity = 0;
		drop_active = true;
		sdk_melody_play("/i:phi D#");
		printf("package delivery pending\n");
	}

	// Update of drop
	if (drop_active) {
		// Aplication of gravity
		drop_velocity += GRAVITY * dt;

		// Update of drop position
		drop.y += drop_velocity * dt;

		// Detection of drop fall to ground
		if (drop.y >= TFT_HEIGHT) {
			drop_active = false;
		}

		if (rects_overlap(drop.x, drop.y, drop.x + 1, drop.y + 3, human1.s.x, 100,
				  human1.s.x + 10, 112)) {
			// au, co to bylo?!
			drop_active = false;
			sdk_melody_play("/i:square g");
			score += 1;
			human1.stop = 1.0f;
			// human1 stop for 1 second, jump 6px, running for 4 seconds
		}

		if (rects_overlap(drop.x, drop.y, drop.x + 1, drop.y + 3, human2.s.x, 80,
				  human2.s.x + 10, 92)) {
			// au, co to bylo?!
			drop_active = false;
			sdk_melody_play("/i:square g");
			score += 1;
			human2.stop = 1.0f;
			// human2 stop for 1 second, jump 6px, running for 4 seconds
		}
	}
}

void game_paint(unsigned __unused dt_usec)
{
	sdk_draw_image(0, 0, &image_level1_png);

	tft_draw_string(2, 1, BLACK, "%i", score);

	// Paint only active drop.
	if (drop_active) {
		drop.tile = fmodf((10.0f * time_us_32()) / 1000000.0f, 2.0f);
		sdk_draw_sprite(&drop);
	}

	// Draw sprite of bird.
	bird.flip_x = bird_dir > 0;
	bird.tile = fmodf((5.0f * time_us_32()) / 1000000.0f, 5.0f);
	sdk_draw_sprite(&bird);

	// Draw sprite of human1.
	human1.s.flip_x = human1.dir < 0;
	if (human1.stop >= 0) {
		human1.s.tile = 9;
	} else {
		human1.s.tile = 8 + fmodf((4.0f * time_us_32()) / 1000000.0f, 4.0f);
	}
	sdk_draw_sprite(&human1.s);

	// Draw sprite of human2.
	human2.s.flip_x = human2.dir < 0;
	if (human2.stop > 0) {
		human2.s.tile = 5;
		//}
		//if (jump_up_human2 >= 0.5f) {
		//    	human2.s.tile = 5;
		//    	human2.s.y += human2.s.y - dt
		//}
		//if (jump_down_human2 >= 0.5f) {
		//    	human2.s.tile = 5;
		//    	human2.s.y += human2.s.y + dt
		//}
		//if (panic_human2) {
		//human2.s.tile = 4 + fmodf((8.0f * time_us_32()) / 1000000.0f, 4.0f);
		//
	} else {
		human2.s.tile = 4 + fmodf((4.0f * time_us_32()) / 1000000.0f, 4.0f);
	}
	sdk_draw_sprite(&human2.s);

	// Draw sprite of drone.
	drone.flip_x = drone_dir < 0;
	drone.tile = fmodf((16.0f * time_us_32()) / 1000000.0f, 4.0f);
	sdk_draw_sprite(&drone);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
