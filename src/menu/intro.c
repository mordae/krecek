#include "intro.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern sdk_scene_t scene_games;
void root_set_scene(int idx);

// Hamster Mascot 1-bit bitmap (24x12)
static const uint32_t hamster_bits[12] = {
	0b000000000000000110110000,
	0b000000000000000101111000,
	0b000000000011111100000100,
	0b000000000100000000000010,
	0b000000001000000000010001,
	0b000000001000000000010001,
	0b000000001000000000000101,
	0b000000001000000000000111,
	0b000000000100000000000001,
	0b000000000011111111111111,
	0b000000000001010001010000,
	0b000000000000100000100000,
};

// 46x9 Chunky Retro Font bitmap for "K Ř E Č E K"
static const char *logo_bitmap[9] = {
	"          # #           # #                   ",
	"##  ##  ####   ###### ###### ###### ##  ##    ",
	"##  ##  ##  ## ##     ##     ##     ##  ##    ",
	"## ##   ##  ## ##     ##     ##     ## ##     ",
	"####    #####  #####  ##     #####  ####      ",
	"####    ####   #####  ##     #####  ####      ",
	"## ##   ## ##  ##     ##     ##     ## ##     ",
	"##  ##  ##  ## ##     ##     ##     ##  ##    ",
	"##  ##  ##  ## ###### ###### ###### ##  ##    ",
};

// 4 Retro Handheld Palettes
typedef struct {
	const char *name;
	color_t bg;
	color_t bg_grid;
	color_t light;
	color_t med;
	color_t dark;
	color_t white;
} IntroPalette;

#define NUM_PALETTES 4
static const IntroPalette palettes[NUM_PALETTES] = {
	{
		.name = "DMG-01 CLASSIC",
		.bg = rgb_to_rgb565(155, 188, 15),
		.bg_grid = rgb_to_rgb565(142, 172, 13),
		.light = rgb_to_rgb565(139, 172, 15),
		.med = rgb_to_rgb565(48, 98, 48),
		.dark = rgb_to_rgb565(15, 56, 15),
		.white = rgb_to_rgb565(240, 255, 210),
	},
	{
		.name = "POCKET SILVER",
		.bg = rgb_to_rgb565(196, 207, 161),
		.bg_grid = rgb_to_rgb565(180, 192, 146),
		.light = rgb_to_rgb565(140, 155, 115),
		.med = rgb_to_rgb565(75, 88, 62),
		.dark = rgb_to_rgb565(27, 34, 27),
		.white = rgb_to_rgb565(255, 255, 255),
	},
	{
		.name = "CYBER AMBER",
		.bg = rgb_to_rgb565(255, 170, 0),
		.bg_grid = rgb_to_rgb565(230, 150, 0),
		.light = rgb_to_rgb565(210, 130, 0),
		.med = rgb_to_rgb565(120, 60, 0),
		.dark = rgb_to_rgb565(40, 18, 0),
		.white = rgb_to_rgb565(255, 240, 180),
	},
	{
		.name = "MIDNIGHT CYAN",
		.bg = rgb_to_rgb565(10, 26, 36),
		.bg_grid = rgb_to_rgb565(14, 34, 46),
		.light = rgb_to_rgb565(22, 60, 80),
		.med = rgb_to_rgb565(0, 160, 190),
		.dark = rgb_to_rgb565(0, 240, 255),
		.white = rgb_to_rgb565(230, 255, 255),
	},
};

static int current_palette = 0;
static bool is_boot_mode = true;
static float anim_timer = 0.0f;
static bool chime_played = false;

void intro_set_boot_mode(bool boot_mode)
{
	is_boot_mode = boot_mode;
#if defined(RUNNING_ON_HOST)
	const char *ms = getenv("MENU_START");
	if (ms && strcmp(ms, "intro_section") == 0) {
		is_boot_mode = false;
	}
#endif
	if (is_boot_mode) {
		anim_timer = 0.0f;
		chime_played = false;
	} else {
		anim_timer = 2.0f;
		chime_played = true;
	}
}

bool intro_is_boot_mode(void)
{
	return is_boot_mode;
}

void intro_start_animation(void)
{
	anim_timer = 0.0f;
	chime_played = false;
}

static void intro_finish_boot(void)
{
	if (!is_boot_mode)
		return;
	is_boot_mode = false;
	root_set_scene(0);
	sdk_scene_swap(&scene_games);
}

// Draw sparkling star
static void draw_sparkle(int cx, int cy, float phase, color_t color)
{
	int s = (int)(phase * 3.0f) % 3;
	if (s == 0) {
		tft_draw_pixel(cx, cy, color);
	} else if (s == 1) {
		tft_draw_pixel(cx, cy, color);
		tft_draw_pixel(cx - 1, cy, color);
		tft_draw_pixel(cx + 1, cy, color);
		tft_draw_pixel(cx, cy - 1, color);
		tft_draw_pixel(cx, cy + 1, color);
	} else {
		tft_draw_pixel(cx, cy, color);
		tft_draw_pixel(cx - 2, cy, color);
		tft_draw_pixel(cx + 2, cy, color);
		tft_draw_pixel(cx, cy - 2, color);
		tft_draw_pixel(cx, cy + 2, color);
		tft_draw_pixel(cx - 1, cy - 1, color);
		tft_draw_pixel(cx + 1, cy - 1, color);
		tft_draw_pixel(cx - 1, cy + 1, color);
		tft_draw_pixel(cx + 1, cy + 1, color);
	}
}

static void intro_paint(float dt, int depth)
{
	(void)depth;
	anim_timer += dt;

	const IntroPalette *pal = &palettes[current_palette];

	// 1. Draw retro LCD background with subtle dot-matrix grid scanlines
	tft_fill(pal->bg);
	for (int y = 0; y < TFT_HEIGHT; y += 2) {
		for (int x = 0; x < TFT_WIDTH; x += 2) {
			tft_draw_pixel(x, y, pal->bg_grid);
		}
	}

	// Calculate vertical descent position (Game Boy drop animation)
	float target_y = 20.0f;
	float current_y = target_y;

	if (anim_timer < 1.6f) {
		float progress = anim_timer / 1.6f;
		// Smooth ease-out curve
		float ease = 1.0f - (1.0f - progress) * (1.0f - progress);
		current_y = -44.0f + (target_y - (-44.0f)) * ease;
	} else {
		current_y = target_y;
		if (!chime_played) {
			chime_played = true;
			// The iconic two-tone Game Boy boot chime: high B then ringing sustained E!
			sdk_melody_play("/i:square > b > E---");
		}
	}

	int base_y = (int)current_y;

	// Calculate shine sweep reflection effect across the logo
	float shine_x = -999.0f;
	if (anim_timer >= 1.6f && anim_timer < 2.3f) {
		float s_prog = (anim_timer - 1.6f) / 0.7f;
		shine_x = 20.0f + s_prog * 130.0f;
	} else if (!is_boot_mode) {
		// Continuous gentle breathing gleam in section mode
		float cycle = fmodf(anim_timer, 3.5f);
		if (cycle < 0.8f) {
			shine_x = 20.0f + (cycle / 0.8f) * 130.0f;
		}
	}

	// 2. Draw Hamster mascot sitting on top of the logo
	int hx = 68;
	int hy = base_y - 14;
	if (hy > -12 && hy < TFT_HEIGHT) {
		for (int r = 0; r < 12; r++) {
			int py = hy + r;
			if (py < 0 || py >= TFT_HEIGHT) continue;
			for (int c = 0; c < 24; c++) {
				int px = hx + c;
				if (px < 0 || px >= TFT_WIDTH) continue;
				if ((hamster_bits[r] >> (23 - c)) & 1) {
					color_t col = pal->dark;
					if (fabsf((px - py * 1.2f) - (shine_x - hy * 1.2f)) < 3.0f) {
						col = pal->white;
					}
					tft_draw_pixel(px, py, col);
				}
			}
		}
	}

	// 3. Draw Game Boy style pill banner around KŘEČEK
	int bx0 = 26, by0 = base_y, bx1 = 133, by1 = base_y + 24;
	if (by1 >= 0 && by0 < TFT_HEIGHT) {
		// Outer border
		for (int x = bx0 + 2; x <= bx1 - 2; x++) {
			if (by0 >= 0 && by0 < TFT_HEIGHT) tft_draw_pixel(x, by0, pal->dark);
			if (by1 >= 0 && by1 < TFT_HEIGHT) tft_draw_pixel(x, by1, pal->dark);
		}
		for (int y = by0 + 2; y <= by1 - 2; y++) {
			if (y >= 0 && y < TFT_HEIGHT) {
				tft_draw_pixel(bx0, y, pal->dark);
				tft_draw_pixel(bx1, y, pal->dark);
			}
		}
		// Corner bevels
		if (by0 + 1 >= 0 && by0 + 1 < TFT_HEIGHT) {
			tft_draw_pixel(bx0 + 1, by0 + 1, pal->dark);
			tft_draw_pixel(bx1 - 1, by0 + 1, pal->dark);
		}
		if (by1 - 1 >= 0 && by1 - 1 < TFT_HEIGHT) {
			tft_draw_pixel(bx0 + 1, by1 - 1, pal->dark);
			tft_draw_pixel(bx1 - 1, by1 - 1, pal->dark);
		}

		// Inner pill fill
		for (int y = by0 + 1; y <= by1 - 1; y++) {
			if (y < 0 || y >= TFT_HEIGHT) continue;
			for (int x = bx0 + 1; x <= bx1 - 1; x++) {
				if ((x == bx0 + 1 || x == bx1 - 1) && (y == by0 + 1 || y == by1 - 1))
					continue;
				tft_draw_pixel(x, y, pal->light);
			}
		}

		// 4. Draw Chunky "K Ř E Č E K" Logo letters
		int lx0 = 34;
		int ly0 = base_y + 3;
		for (int r = 0; r < 9; r++) {
			for (int c = 0; c < 46; c++) {
				if (logo_bitmap[r][c] == '#') {
					for (int dy = 0; dy < 2; dy++) {
						int py = ly0 + r * 2 + dy;
						if (py < 0 || py >= TFT_HEIGHT) continue;
						for (int dx = 0; dx < 2; dx++) {
							int px = lx0 + c * 2 + dx;
							if (px < 0 || px >= TFT_WIDTH) continue;
							color_t col = pal->dark;
							if (fabsf((px - py * 1.5f) - (shine_x - ly0 * 1.5f)) < 3.5f) {
								col = pal->white;
							}
							tft_draw_pixel(px, py, col);
						}
					}
				}
			}
		}

		// Trademark symbol (R)
		int rx = 126, ry = base_y + 2;
		if (ry >= 0 && ry + 4 < TFT_HEIGHT) {
			tft_draw_pixel(rx, ry, pal->dark);
			tft_draw_pixel(rx + 1, ry, pal->dark);
			tft_draw_pixel(rx, ry + 1, pal->dark);
			tft_draw_pixel(rx + 2, ry + 1, pal->dark);
			tft_draw_pixel(rx, ry + 2, pal->dark);
			tft_draw_pixel(rx + 1, ry + 2, pal->dark);
			tft_draw_pixel(rx, ry + 3, pal->dark);
			tft_draw_pixel(rx + 2, ry + 3, pal->dark);
		}
	}

	// 5. Sparkles when chime rings
	if (anim_timer >= 1.6f && anim_timer < 3.0f) {
		float sp = anim_timer * 8.0f;
		draw_sparkle(22, base_y + 4, sp, pal->white);
		draw_sparkle(137, base_y + 20, sp + 1.5f, pal->white);
		draw_sparkle(80, base_y - 18, sp + 0.8f, pal->white);
	}

	if (is_boot_mode) {
		// 6. Subtitles below logo (Game Boy hardware styling)
		if (anim_timer >= 1.6f) {
			tft_draw_string_center(80, base_y + 34, pal->dark, "HANDHELD SYSTEM");
			tft_draw_string_center(80, base_y + 52, pal->med, "DOT MATRIX LCD");
			tft_draw_string_center(80, base_y + 70, pal->med, "STEREO SOUND");
		}
		// During boot sequence, auto-advance after hold duration
		if (anim_timer >= 3.6f) {
			intro_finish_boot();
		}
	} else {
		// 7. Interactive Section Display (when viewed in menu section mode)
		tft_draw_string_center(80, 50, pal->med, "%s", pal->name);
		tft_draw_string_center(80, 68, pal->dark, "[A] REPLAY INTRO");
		tft_draw_string_center(80, 84, pal->med, "[B] CHANGE THEME");
		tft_draw_string_center(80, 100, pal->dark, "[Y] NEXT SECTION");
	}
}

static bool intro_handle(sdk_event_t event, int depth)
{
	(void)depth;

	// In boot sequence: ANY button immediately skips to the games carousel!
	if (is_boot_mode) {
		if (event == SDK_PRESSED_A || event == SDK_PRESSED_B ||
		    event == SDK_PRESSED_START || event == SDK_PRESSED_SELECT ||
		    event == SDK_PRESSED_X || event == SDK_PRESSED_Y) {
			intro_finish_boot();
			return true;
		}
		return false;
	}

	// In menu section mode:
	switch (event) {
	case SDK_PRESSED_A:
	case SDK_PRESSED_START:
		// Replay boot animation + chime!
		intro_start_animation();
		return true;

	case SDK_PRESSED_B:
	case SDK_PRESSED_X:
		// Cycle retro color theme!
		current_palette = (current_palette + 1) % NUM_PALETTES;
		sdk_melody_play("/i:square > g");
		return true;

	case SDK_PRESSED_Y:
		// Pass through so root_handle can switch to the next scene!
		return false;

	default:
		return false;
	}
}

sdk_scene_t scene_intro = {
	.paint = intro_paint,
	.handle = intro_handle,
};
