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

sdk_game_info("arnost", &image_cover_png);

#define BLACK rgb_to_rgb565(0, 0, 0)
#define GRAY rgb_to_rgb565(63, 63, 63)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define PINK rgb_to_rgb565(255, 0, 180)
#define BROWN rgb_to_rgb565(60, 15, 0)
#define AZURE rgb_to_rgb565(0, 232, 255)

/*
 *  _    _      _
 * | |  | |    | |
 * | |__| | ___| |_ __
 * |  __  |/ _ \ | '_ \
 * | |  | |  __/ | |_) |
 * |_|  |_|\___|_| .__/
 *               | |
 *               |_|
 *
 *
 */

/* vice
 * radkovy
 * komentar
 */

//  jednoradkovy komentar az do konce radku

// && = a pokud také platí..., pokud platí obě podmínky před i za && je celý výrok pravdivý
// || = nebo, pokud platí levá strana, prava strana nebo obe, je vyrok pravdivy
// ! = negace výroku, logicky opak

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
 * Pokud zasáhne připočítají se mu body. -> ZVUK při zásahu.
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
 */

static sdk_sprite_t bird;
static sdk_sprite_t human1;
static sdk_sprite_t human2;
static sdk_sprite_t drop;

// speed of movement???
static float bird_dir = 30;
static float human1_dir = 15;
static float human2_dir = 5;

// old code static float human_pos = 110;
// old code static float human_dir = 6;

//static float drop_x = 0;
//static float drop_y = 0;

// Globální proměnné pro stav kapky
static bool drop_active = false;
static float drop_velocity = 0;
static const float GRAVITY = 200.0f; // gravity acceleration (px/s²)

static int score = 0;

// Cum decore, Tielmann Susato, arr. Jos van den Borre 1551
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
	human1.ts = &ts_people_png;
	human1.tile = 8;
	human1.y = 100;
	human1.x = TFT_WIDTH / 2.0f - human1.ts->width / 2.0f;
	human2.ts = &ts_people_png;
	human2.tile = 4;
	human2.y = 80;
	human2.x = TFT_WIDTH / 2.0f - human2.ts->width / 2.0f;
	drop.ts = &ts_drop_png;
	drop.tile = 0;
	drop.y = 4;
	drop.x = TFT_WIDTH / 2.0f - drop.ts->width / 2.0f;
	drop.ox = 4.0f;
	drop.oy = 7.0f;
	melody1 = sdk_melody_play_get(music1);
	melody2 = sdk_melody_play_get(music2);
	melody3 = sdk_melody_play_get(music3);
}

static int sign(int x)
{
	return x >= 0 ? 1 : -1;
}

// collision of objects
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
	if (bird.x <= 0 || bird.x >= TFT_RIGHT - bird.ts->width) {
		bird_dir = -bird_dir;
		printf("bird hit wall, bird_dir=%f\n", bird_dir);
	}

	// Move human by their speed.
	human1.x += human1_dir * dt;

	// Change human direction when hitting ends of the screen.
	if (human1.x <= 0 || human1.x >= TFT_RIGHT - human1.ts->width) {
		human1_dir = -human1_dir;
		printf("human hit wall, human_dir=%f\n", human1_dir);
	}

	// Move human by their speed.
	human2.x += human2_dir * dt;

	// Change human direction when hitting ends of the screen.
	if (human2.x <= 0 || human2.x >= TFT_RIGHT - human2.ts->width) {
		human2_dir = -human2_dir;
		printf("human hit wall, human_dir=%f\n", human2_dir);
	}

	/* old code: Move human by their speed.
	* human_pos += human_dir * dt;
	*
	* Change human direction when hitting ends of the screen.
	* if (human_pos <= 0 || human_pos >= TFT_RIGHT - 5) {
	*	human_dir = -human_dir;
	*	printf("human hit wall, human_dir=%f\n", human_dir);
	* }
	*/

	// drop movement
	// drop_y += drop_y * dt;

	// if (sdk_inputs_delta.start > 0) {
	//	drop_y = -drop_y;
	// }

	// drop acceleration
	// drop_y = drop_y + 0.1 * drop_y * dt;

	// Make drop by push "a".
	if (sdk_inputs_delta.a > 0 && !drop_active) {
		drop.x = bird.x + 8 - sign(bird_dir) * 4; // Střed ptáka
		drop.y = bird.y + 16;			  // Těsně pod ptákem
		drop_velocity = 0;
		drop_active = true;
		sdk_melody_play("/i:phi D#");
		printf("package delivery pending\n");
	}

	// Aktualizace kapky, pokud je aktivní
	if (drop_active) {
		// Aplikace gravitace
		drop_velocity += GRAVITY * dt;

		// Aktualizace pozice
		drop.y += drop_velocity * dt;

		// Kontrola dopadu na zem
		if (drop.y >= TFT_HEIGHT) {
			drop_active = false;
		}

		if (rects_overlap(drop.x, drop.y, drop.x + 1, drop.y + 3, human1.x, 100,
				  human1.x + 10, 112)) {
			// au, co to bylo?!
			drop_active = false;
			sdk_melody_play("/i:square g");
			score += 1;
		}

		if (rects_overlap(drop.x, drop.y, drop.x + 1, drop.y + 3, human2.x, 80,
				  human2.x + 10, 92)) {
			// au, co to bylo?!
			drop_active = false;
			sdk_melody_play("/i:square g");
			score += 1;
		}
	}
}

void game_paint(unsigned __unused dt_usec)
{
	sdk_draw_image(0, 0, &image_level1_png);

	tft_draw_string(2, 1, BLACK, "%i", score);

	/* tft_draw_rect(drop_x + 3, TFT_HEIGHT / 3 / 4 + 11, drop_x + 3, TFT_HEIGHT / 3 / 4 + 14,
	*	      WHITE);
	*/
	// old code of huma: tft_draw_rect(human_pos, 100, human_pos + 5, 112, AZURE);

	// paint only active drop
	if (drop_active) {
		// tft_draw_rect(drop_x, drop_y, drop_x + 1, drop_y + 3, WHITE);
		drop.tile = fmodf((10.0f * time_us_32()) / 1000000.0f, 2.0f);
		sdk_draw_sprite(&drop);
	}

	// Draw srpite of bird.
	bird.flip_x = bird_dir > 0;
	bird.tile = fmodf((5.0f * time_us_32()) / 1000000.0f, 5.0f);
	sdk_draw_sprite(&bird);

	// Draw sprite of human1.
	human1.flip_x = human1_dir < 0;
	human1.tile = 8 + fmodf((4.0f * time_us_32()) / 1000000.0f, 4.0f);
	sdk_draw_sprite(&human1);

	// Draw sprite of human2.
	human2.flip_x = human2_dir < 0;
	human2.tile = 4 + fmodf((4.0f * time_us_32()) / 1000000.0f, 4.0f);
	sdk_draw_sprite(&human2);
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
