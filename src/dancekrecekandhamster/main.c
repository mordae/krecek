#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <math.h>

#include <balls.png.h>
#define GRAY rgb_to_rgb565(63, 63, 63)
#include <cover.png.h>
sdk_game_info("kal", &image_cover_png);

/* Struktura pro objekty typu "tile" (kruhy) */
struct tile_object {
	int x;
	int y;
};

/* Instance pro oba naše "kruhy" */
static struct tile_object tile1;
static struct tile_object tile2;

/* Parametry kruhové dráhy */
#define POLOMER_DRAHY 25.0f
#define RYCHLOST_UHLU 0.05f

/* Aktuální úhel v radiánech */
static float aktualni_uhel = 0.0f;

/* Stav, který určuje, který tile obíhá kolem kterého */
/* TRUE: tile2 obíhá kolem tile1 (výchozí) */
/* FALSE: tile1 obíhá kolem tile2 */
static bool tile2_orbits_tile1 = true;

void game_reset(void)
{
	/* Počáteční pozice tile1 (pevný střed) */
	tile1.x = TFT_WIDTH / 2;
	tile1.y = TFT_HEIGHT / 2;

	/* Počáteční úhel */
	aktualni_uhel = 0.0f;

	/* Počáteční pozice tile2 (obíhá kolem tile1) */
	tile2.x = (int)roundf(tile1.x + POLOMER_DRAHY * cosf(aktualni_uhel));
	tile2.y = (int)roundf(tile1.y + POLOMER_DRAHY * sinf(aktualni_uhel));

	tile2_orbits_tile1 = true;
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec; /* dt_usec zde není použito */

	/* Tento prázdný if blok simuluje stisknutí tlačítka. */
	/* V reálném kódu by zde byla kontrola vstupu, např. zda bylo stisknuto tlačítko. */
	if (sdk_inputs_delta.x > 0) { /* Příklad použití funkce z SDK pro kontrolu tlačítka A */
		/* Přepneme stav obíhání */
		tile2_orbits_tile1 = !tile2_orbits_tile1;
		/* Resetujeme úhel, aby pohyb po přepnutí začal plynule */
		aktualni_uhel = 0.0f;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	float dt = dt_usec / 1000000.0f; /* Konverze delta času na sekundy */

	/* Aktualizujeme úhel */
	aktualni_uhel += RYCHLOST_UHLU * (dt * 60.0f);
	aktualni_uhel = fmodf(aktualni_uhel, 2 * M_PI);

	/* Vypočítáme pozice na základě aktuálního stavu obíhání */
	if (tile2_orbits_tile1) {
		/* Tile2 obíhá kolem tile1 */
		tile2.x = (int)roundf(tile1.x + POLOMER_DRAHY * cosf(aktualni_uhel));
		tile2.y = (int)roundf(tile1.y + POLOMER_DRAHY * sinf(aktualni_uhel));
	} else {
		/* Tile1 obíhá kolem tile2 */
		tile1.x = (int)roundf(tile2.x + POLOMER_DRAHY * cosf(aktualni_uhel));
		tile1.y = (int)roundf(tile2.y + POLOMER_DRAHY * sinf(aktualni_uhel));
	}

	/* Vyčistíme celou obrazovku */
	tft_fill(0);

	/* Vykreslení obou tile */
	sdk_draw_tile(tile1.x, tile1.y, &ts_balls_png, 0);
	sdk_draw_tile(tile2.x, tile2.y, &ts_balls_png, 0);
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
	return 0;
}
