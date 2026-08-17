#include "minevox.h"

#include <stdio.h>
#include <string.h>

#define COL_WHITE rgb_to_rgb565(255, 255, 255)
#define COL_GRAY rgb_to_rgb565(170, 170, 170)
#define COL_DARK rgb_to_rgb565(30, 30, 34)
#define COL_YELLOW rgb_to_rgb565(255, 220, 90)
#define COL_GREEN rgb_to_rgb565(120, 220, 120)

enum game_state game_state = STATE_TITLE;

bool opt_sfx = true;
bool opt_music = false;
bool opt_debug = false;

static bool have_save = false;
static float toast_timer = 0.0f;
static char toast_text[24];
static int last_slot = -1;

static sdk_melody_t *music;

/* Pause menu. */
enum menu_item {
	MENU_RESUME = 0,
	MENU_SAVE,
	MENU_NEW,
	MENU_DETAIL,
	MENU_SOUND,
	MENU_MUSIC,
	MENU_DEBUG,
	MENU_HELP,
	MENU_TITLE,
	NUM_MENU_ITEMS,
};

static int menu_sel = 0;
static bool show_help = false;

/* Block picker. */
#define PICK_COLS 5
#define PICK_ROWS 3
static int pick_sel = 0;

static const char *music_tune = "{ /i:sine /bpm:80 (pp) c-e-g-E- g-e-c-g- "
				"f-a-C-a- e-g-b-g- }";

void sfx_play(const char *melody)
{
	if (opt_sfx)
		sdk_melody_play(melody);
}

static void music_update(void)
{
	if (opt_music && !music)
		music = sdk_melody_play_get(music_tune);

	if (!opt_music && music) {
		sdk_melody_stop_and_release(music);
		music = NULL;
	}
}

static void toast(const char *text)
{
	snprintf(toast_text, sizeof(toast_text), "%s", text);
	toast_timer = 1.6f;
}

/* Text with a drop shadow, so that it survives any background. */
static void shadow_text(int x, int y, color_t color, const char *text)
{
	tft_draw_string(x + 1, y + 1, COL_DARK, "%s", text);
	tft_draw_string(x, y, color, "%s", text);
}

static void shadow_text_center(int x, int y, color_t color, const char *text)
{
	tft_draw_string_center(x + 1, y + 1, COL_DARK, "%s", text);
	tft_draw_string_center(x, y, color, "%s", text);
}

static void draw_frame(int x0, int y0, int x1, int y1, color_t color)
{
	tft_draw_rect(x0, y0, x1, y0, color);
	tft_draw_rect(x0, y1, x1, y1, color);
	tft_draw_rect(x0, y0, x0, y1, color);
	tft_draw_rect(x1, y0, x1, y1, color);
}

static void draw_crosshair(void)
{
	int cx = SCREEN_W / 2;
	int cy = SCREEN_H / 2;

	render_darken(cx - 4, cy, cx + 4, cy, 8);
	render_darken(cx, cy - 4, cx, cy + 4, 8);
	tft_draw_rect(cx - 3, cy, cx + 3, cy, COL_WHITE);
	tft_draw_rect(cx, cy - 3, cx, cy + 3, COL_WHITE);
}

#define HOTBAR_SLOT 14
#define HOTBAR_TOP (SCREEN_H - HOTBAR_SLOT - 2)

/* Icons follow the daylight so they do not glare at night. */
static uint8_t icon_bright(void)
{
	float light = 1.0f;
	render_sky_colors(NULL, NULL, &light);
	return (uint8_t)clampi((int)(light * 32.0f), 16, 32);
}

static void draw_hotbar(void)
{
	const int slot = HOTBAR_SLOT;
	const int total = slot * 9;
	int x0 = (SCREEN_W - total) / 2;
	int y0 = HOTBAR_TOP;
	uint8_t bright = icon_bright();

	render_darken(x0 - 1, y0 - 1, x0 + total, y0 + slot, 12);

	for (int i = 0; i < 9; i++) {
		int x = x0 + i * slot;

		blocks_draw_icon(x + 1, y0 + 1, slot - 2, player.hotbar[i], bright);

		draw_frame(x - 1, y0 - 1, x + slot - 1, y0 + slot,
			   i == player.slot ? COL_WHITE : rgb_to_rgb565(90, 90, 96));
	}
}

/* The block we are holding, waved about when we use it. */
static void draw_held_block(void)
{
	float swing = player.swing > 0.0f ? player.swing : 0.0f;
	int lift = (int)(sinf(swing * (float)M_PI) * 6.0f);

	int size = 28;
	int x = SCREEN_W - size - 3 + lift / 2;
	int y = HOTBAR_TOP - size - 2 + lift;

	render_darken(x - 1, y - 1, x + size, y + size, 20);
	blocks_draw_icon(x, y, size, player.hotbar[player.slot], icon_bright());
}

static void draw_debug(void)
{
	char line[32];

	snprintf(line, sizeof(line), "%d %d %d", (int)player.x, (int)player.y, (int)player.z);
	shadow_text(2, 2, COL_GREEN, line);

	int hour = (int)(world_time / DAY_LENGTH * 24.0f) % 24;
	snprintf(line, sizeof(line), "%02d:00 %s", hour, player.flying ? "fly" : "walk");
	shadow_text(2, 15, COL_GREEN, line);

	if (player.has_target) {
		uint8_t block = world_get(player.tx, player.ty, player.tz);
		shadow_text(2, 28, COL_GREEN, block_info[block].name);
	}
}

static void draw_title(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 15);

	/* A row of blocks as a makeshift logo shelf. */
	for (int i = 0; i < 5; i++)
		blocks_draw_icon(24 + i * 24, 4, 22, BLOCK_GRASS + i, 32);

	shadow_text_center(SCREEN_W / 2, 34, COL_WHITE, "M I N E V O X");
	shadow_text_center(SCREEN_W / 2, 50, COL_GRAY, "a world of blocks");

	shadow_text_center(SCREEN_W / 2, 72, COL_YELLOW, have_save ? "A: continue" : "A: play");
	shadow_text_center(SCREEN_W / 2, 86, COL_GRAY, "B: new world");
	shadow_text_center(SCREEN_W / 2, 100, COL_GRAY, "START: controls");
}

static void draw_help(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 6);

	shadow_text_center(SCREEN_W / 2, 2, COL_YELLOW, "CONTROLS  (B: back)");

	static const char *lines[] = {
		"stick  walk & turn",  "L / R  look up/down", "A      jump, AA fly",
		"B      mine (hold)",  "X      place block",  "Y      next block",
		"hold Y block picker",
	};

	for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++)
		shadow_text(4, 20 + i * 14, COL_WHITE, lines[i]);
}

static void draw_menu(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 10);

	shadow_text_center(SCREEN_W / 2, 1, COL_YELLOW, "PAUSED");

	char line[32];

	for (int i = 0; i < NUM_MENU_ITEMS; i++) {
		switch (i) {
		case MENU_RESUME:
			snprintf(line, sizeof(line), "Resume");
			break;
		case MENU_SAVE:
			snprintf(line, sizeof(line), "Save world");
			break;
		case MENU_NEW:
			snprintf(line, sizeof(line), "New world");
			break;
		case MENU_DETAIL:
			snprintf(line, sizeof(line), "Detail    %s",
				 render_scale == 1 ? "full" : (render_scale == 2 ? "half" : "low"));
			break;
		case MENU_SOUND:
			snprintf(line, sizeof(line), "Sound     %s", opt_sfx ? "on" : "off");
			break;
		case MENU_MUSIC:
			snprintf(line, sizeof(line), "Music     %s", opt_music ? "on" : "off");
			break;
		case MENU_DEBUG:
			snprintf(line, sizeof(line), "Info      %s", opt_debug ? "on" : "off");
			break;
		case MENU_HELP:
			snprintf(line, sizeof(line), "Controls");
			break;
		default:
			snprintf(line, sizeof(line), "Quit to title");
			break;
		}

		int y = 18 + i * 11;
		shadow_text(18, y, i == menu_sel ? COL_YELLOW : COL_WHITE, line);

		if (i == menu_sel)
			shadow_text(8, y, COL_YELLOW, ">");
	}
}

static void draw_picker(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 10);

	shadow_text_center(SCREEN_W / 2, 1, COL_YELLOW, "PICK A BLOCK");

	const int cell = 22;
	int x0 = (SCREEN_W - PICK_COLS * cell) / 2;
	int y0 = 19;

	for (int i = 0; i < PICK_COLS * PICK_ROWS; i++) {
		int block = i + 1;

		if (block >= NUM_BLOCKS)
			break;

		int x = x0 + (i % PICK_COLS) * cell;
		int y = y0 + (i / PICK_COLS) * cell;

		blocks_draw_icon(x + 2, y + 2, cell - 4, block, 32);

		if (i == pick_sel)
			draw_frame(x, y, x + cell - 1, y + cell - 1, COL_WHITE);
	}

	shadow_text_center(SCREEN_W / 2, 87, COL_WHITE, block_info[pick_sel + 1].name);
	shadow_text_center(SCREEN_W / 2, 103, COL_GRAY, "A: take   B: back");
}

void hud_paint(float dt)
{
	if (show_help) {
		draw_help();
		return;
	}

	switch (game_state) {
	case STATE_TITLE:
		draw_title();
		return;

	case STATE_PAUSE:
		draw_menu();
		return;

	case STATE_PICKER:
		draw_picker();
		return;

	default:
		break;
	}

	draw_crosshair();
	draw_held_block();
	draw_hotbar();

	if (player.slot != last_slot) {
		last_slot = player.slot;
		toast(block_info[player.hotbar[player.slot]].name);
	}

	if (toast_timer > 0.0f) {
		toast_timer -= dt;
		shadow_text_center(SCREEN_W / 2, HOTBAR_TOP - 18, COL_WHITE, toast_text);
	}

	if (opt_debug)
		draw_debug();
}

static void start_new_world(void)
{
	world_generate(sdk_random());
	player_reset();
	world_time = 90.0f;
	have_save = false;
	toast("New world");
}

static void menu_activate(void)
{
	switch (menu_sel) {
	case MENU_RESUME:
		game_state = STATE_PLAY;
		break;

	case MENU_SAVE:
		world_save();
		have_save = true;
		game_state = STATE_PLAY;
		toast("Saved");
		break;

	case MENU_NEW:
		start_new_world();
		game_state = STATE_PLAY;
		break;

	case MENU_DETAIL:
		render_scale = render_scale % 3 + 1;
		break;

	case MENU_SOUND:
		opt_sfx = !opt_sfx;
		break;

	case MENU_MUSIC:
		opt_music = !opt_music;
		music_update();
		break;

	case MENU_DEBUG:
		opt_debug = !opt_debug;
		break;

	case MENU_HELP:
		show_help = true;
		break;

	default:
		world_save();
		have_save = true;
		game_state = STATE_TITLE;
		break;
	}
}

void hud_input(float dt)
{
	(void)dt;

	if (show_help) {
		if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.a > 0 || sdk_inputs_delta.start > 0)
			show_help = false;

		return;
	}

	int move = sdk_inputs_delta.vertical;
	int side = sdk_inputs_delta.horizontal;

	switch (game_state) {
	case STATE_TITLE:
		/* Let the camera drift so the title has something to show. */
		player.yaw += dt * 0.12f;

		if (sdk_inputs_delta.a > 0) {
			game_state = STATE_PLAY;
			sfx_play("/i:sine (p) ceg");
		} else if (sdk_inputs_delta.b > 0) {
			start_new_world();
			game_state = STATE_PLAY;
			sfx_play("/i:sine (p) ceg");
		} else if (sdk_inputs_delta.start > 0) {
			show_help = true;
		}

		break;

	case STATE_PAUSE:
		if (move > 0)
			menu_sel = (menu_sel + 1) % NUM_MENU_ITEMS;
		if (move < 0)
			menu_sel = (menu_sel + NUM_MENU_ITEMS - 1) % NUM_MENU_ITEMS;

		if (sdk_inputs_delta.a > 0 || sdk_inputs_delta.x > 0)
			menu_activate();

		if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.start > 0)
			game_state = STATE_PLAY;

		break;

	case STATE_PICKER: {
		int last = NUM_BLOCKS - 2;

		if (side > 0)
			pick_sel++;
		if (side < 0)
			pick_sel--;
		if (move > 0)
			pick_sel += PICK_COLS;
		if (move < 0)
			pick_sel -= PICK_COLS;

		pick_sel = clampi(pick_sel, 0, last);

		if (sdk_inputs_delta.a > 0 || sdk_inputs_delta.x > 0) {
			player.hotbar[player.slot] = pick_sel + 1;
			last_slot = -1;
			game_state = STATE_PLAY;
		}

		if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.start > 0)
			game_state = STATE_PLAY;

		break;
	}

	default:
		break;
	}
}

void hud_notice_save(bool exists)
{
	have_save = exists;
}
