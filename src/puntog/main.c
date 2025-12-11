#include "common.h"
#include <body-4x4.png.h>
#include <head-6x6.png.h>
#include <larm-1x3.png.h>
#include <lleg-2x2.png.h>
#include <rarm-1x3.png.h>
#include <rleg-2x2.png.h>
#include <sdk.h>
#include <stdio.h>
#include <string.h>
#include <tiles-16x16.png.h>
// string 7x10
//*
//-----TODO-----
//
//   Menu
//   Spell
//   Book of speels
//   Maps
//
//*
extern TileType maps_tuturial[MAP_ROWS][MAP_COLS];
TileType (*map)[MAP_COLS] = maps_tuturial;
static const char keypad[4][4] = { { 'A', 'B', 'C', 'D' },
				   { 'E', 'F', 'G', 'H' },
				   { 'I', 'J', 'K', 'L' },
				   { 'M', 'N', 'O', 'P' } };

static const char keypad2[4][4] = { { 'Q', 'R', 'S', 'T' },
				    { 'U', 'V', 'W', 'X' },
				    { 'Y', 'Z', ' ', ' ' },
				    { ' ', ' ', ' ', ' ' } };
static int using_second = 0;
static int cur_x = 0;
static int cur_y = 0;
static int name_len = 0;
static char buffer[16] = { 0 };

enum Game_Status {
	COSTUME_MENU = 0,
	NAME_MENU = 1,
	IN_GAME,
	MENU,
	STATS_MENU,
};

enum Player_Status {
	STANDING,
	RUNING,
};

typedef struct Save {
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
	bool set;
	float timespend;
} Save;

typedef struct {
	float x, y;
	float fx, fy;
	int tile_x, tile_y;
	float speed;
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
	bool nameset;
	int ch[15];
	int direction, d2;
	enum Player_Status S;
} Player;

typedef struct {
	int whut;
	int P_start_x;
	int P_start_y;
	bool init;
	float autosafe_timer;
	float timespend;
	int main_chose;
	enum Game_Status S;
} Menu;

static Menu menu;
static Player p;

static void game_costume();
static void game_read_player();
static void game_save_player();
static void world_start();
static void tiles();
static void player_paint(float x, float y, int direction);
static void draw_bordered_rect(int x1, int y1, int x2, int y2, color_t fill_color,
			       color_t border_color);
static void fill_rect(int x1, int y1, int x2, int y2, color_t color)
{
	for (int y = y1; y <= y2; y++) {
		tft_draw_rect(x1, y, x2, y, color);
	}
}

static void game_read_player()
{
	Save save;
	sdk_save_read(0, &save, sizeof(save));

	memcpy(p.name, save.name, sizeof(p.name));
	if (isnan(save.timespend)) {
		save.timespend = 0;
		menu.timespend = 0;
	} else {
		menu.timespend = save.timespend;
	}
	p.nameset = save.set;
	p.head = save.head;
	p.body = save.body;
	p.Larm = save.Larm;
	p.Rarm = save.Rarm;
	p.Lleg = save.Lleg;
	p.Rleg = save.Rleg;
}

static void game_save_player()
{
	Save save = {
		.timespend = menu.timespend,
		.set = 1,
		.head = p.head,
		.body = p.body,
		.Larm = p.Larm,
		.Rarm = p.Rarm,
		.Lleg = p.Lleg,
		.Rleg = p.Rleg,
	};

	memcpy(save.name, p.name, sizeof(save.name));
	sdk_save_write(0, &save, sizeof(save));
}
static void name_menu_input()
{
	if (sdk_inputs_delta.horizontal == -1 && cur_x > 0)
		cur_x--;
	if (sdk_inputs_delta.horizontal == 1 && cur_x < 3)
		cur_x++;
	if (sdk_inputs_delta.vertical == -1 && cur_y > 0)
		cur_y--;
	if (sdk_inputs_delta.vertical == 1 && cur_y < 3)
		cur_y++;

	if (sdk_inputs_delta.select == 1) {
		using_second = !using_second;
		cur_x = 0;
		cur_y = 0;
	}

	char s = (using_second ? keypad2 : keypad)[cur_y][cur_x];

	if (sdk_inputs_delta.a == 1 && name_len < 15) {
		if ((s >= 'A' && s <= 'Z') || s == '-' || s == '_' || s == ' ') {
			buffer[name_len++] = s;
			buffer[name_len] = 0;
			memcpy(p.name, buffer, 16);
		}
	}

	if (sdk_inputs_delta.b == 1 && name_len > 0) {
		buffer[--name_len] = 0;
		memcpy(p.name, buffer, 16);
	}

	if (sdk_inputs_delta.start == 1 && name_len > 0) {
		buffer[name_len] = 0;
		memcpy(p.name, buffer, 16);
		p.nameset = true;
		menu.S = COSTUME_MENU;
		game_save_player();
	}
}
static void game_start_menu()
{
	tft_fill(0);

	const int cell_w = 20;
	const int cell_h = 15;
	const int start_x = 20;
	const int start_y = 15;

	tft_draw_string(60, 2, YELLOW, "NAME");

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int px = start_x + x * cell_w;
			int py = start_y + y * cell_h;

			char c = (using_second ? keypad2 : keypad)[y][x];

			if (x == cur_x && y == cur_y) {
				fill_rect(px, py, px + cell_w - 1, py + cell_h - 1, WHITE);
				tft_draw_string(px + 6, py + 3, 0, "%c", c);
			} else {
				tft_draw_rect(px, py, px + cell_w - 1, py + cell_h - 1, WHITE);
				tft_draw_string(px + 6, py + 3, WHITE, "%c", c);
			}
		}
	}

	//tft_draw_rect(10, 78, 150, 92, WHITE);
	tft_draw_string(12, 80, WHITE, "%s", p.name);

	int name_width = strlen(p.name) * 7;
	if (name_width < 138) {
		tft_draw_rect(15 + name_width, 83, 15 + name_width + 6, 89, CYAN);
	}

	tft_draw_string(10, 91, CYAN, "A-Set");
	tft_draw_string(10, 101, CYAN, "B-Rem");
	tft_draw_string(10, 112, CYAN, "START");

	if (using_second) {
		tft_draw_string(90, 93, GREEN, "PAGE 2");
	} else {
		tft_draw_string(90, 93, BLUE, "PAGE 1");
	}

	tft_draw_string(90, 105, YELLOW, "SEL-Page");
}

void game_start(void)
{
	game_read_player();
	if (p.nameset == 1) {
		menu.S = IN_GAME;
		menu.autosafe_timer = 0;
		world_start();
		return;
	}

	menu.S = NAME_MENU;
}

static void draw_bordered_rect(int x1, int y1, int x2, int y2, color_t fill_color,
			       color_t border_color)
{
	fill_rect(x1, y1, x2, y2, fill_color);
	tft_draw_rect(x1, y1, x2, y1, border_color);
	tft_draw_rect(x1, y2, x2, y2, border_color);
	tft_draw_rect(x1, y1, x1, y2, border_color);
	tft_draw_rect(x2, y1, x2, y2, border_color);
}

static void world_start(void)
{
	p.x = 20;
	p.y = 15;
	p.speed = 50;
	p.d2 = 0;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	menu.autosafe_timer += dt;
	if (menu.autosafe_timer > AUTOSAFE_TIME) {
		menu.timespend += menu.autosafe_timer;
		game_save_player();
		menu.autosafe_timer = 0;
	}

	switch (menu.S) {
	case NAME_MENU:
		name_menu_input();
		return;
		break;
	case STATS_MENU:
		if (sdk_inputs_delta.select == 1) {
			menu.S = MENU;
		}
		break;
	case COSTUME_MENU:
		if (sdk_inputs_delta.start == 1) {
			menu.S = IN_GAME;
			world_start();
		}
		if (sdk_inputs_delta.vertical == 1) {
			menu.whut = (menu.whut + 1) % 6;
		}
		if (sdk_inputs_delta.vertical == -1) {
			menu.whut = (menu.whut - 1 + 6) % 6;
		}
		if (sdk_inputs_delta.b == 1) {
			switch (menu.whut) {
			case 0:
				p.head = (p.head + 1) % PLAYER_SPRITES;
				break;
			case 1:
				p.body = (p.body + 1) % PLAYER_SPRITES;
				break;
			case 2:
				p.Rarm = (p.Rarm + 1) % PLAYER_SPRITES;
				break;
			case 3:
				p.Larm = (p.Larm + 1) % PLAYER_SPRITES;
				break;
			case 4:
				p.Rleg = (p.Rleg + 1) % PLAYER_SPRITES;
				break;
			case 5:
				p.Lleg = (p.Lleg + 1) % PLAYER_SPRITES;
				break;
			}
		}
		return;
		break;

	case MENU:
		if (sdk_inputs_delta.vertical == -1 && menu.main_chose > 0) {
			menu.main_chose--;
		}
		if (sdk_inputs_delta.vertical == 1 && menu.main_chose < 3) {
			menu.main_chose++;
		}
		if (sdk_inputs_delta.start) {
			switch (menu.main_chose) {
			case 0:
				menu.S = STATS_MENU;
				break;
			case 1:
				menu.S = NAME_MENU;
				break;
			case 2:
				break;
			case 3:
				break;
			default:
				break;
			}
		}

		if (sdk_inputs_delta.select == 1) {
			menu.S = IN_GAME;
		}
		break;
	default:
		break;
	}

	if (sdk_inputs_delta.select == 1) {
		menu.S = MENU;
		return;
	}

	if (sdk_inputs.joy_x > 500 || sdk_inputs.joy_x < -500) {
		p.fx = p.speed * sdk_inputs.joy_x / 2048;
		p.fy = 0;
	} else if (sdk_inputs.joy_y > 500 || sdk_inputs.joy_y < -500) {
		p.fy = p.speed * sdk_inputs.joy_y / 2048;
		p.fx = 0;
	} else {
		p.fx = 0;
		p.fy = 0;
	}

	if (p.fy > 0) {
		p.direction = 0;
	} else if (p.fy < 0) {
		p.direction = 3;
	} else if (p.fx > 0) {
		p.direction = 2;
	} else if (p.fx < 0) {
		p.direction = 1;
	}

	p.x += p.fx * dt;
	p.y += p.fy * dt;
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);

	switch (menu.S) {
	case NAME_MENU:
		tft_set_origin(0, 0);
		game_start_menu();
		break;
	case COSTUME_MENU:
		tft_set_origin(0, 0);
		game_costume();
		game_save_player();
		break;
	case IN_GAME:
		tft_set_origin(p.x - TFT_WIDTH / 2.0 + PLAYER_WIDTH / 2.0,
			       p.y - TFT_HEIGHT / 2.0 + PLAYER_HEIGHT / 2.0);
		tiles();
		player_paint(p.x, p.y, p.direction);
		break;
	case MENU:
		tft_set_origin(0, 0);
		int y_pos = 50 + menu.main_chose * 15;
		tft_draw_string(30, y_pos, YELLOW, ">");
		tft_draw_string(65, 5, YELLOW, "MENU");

		tft_draw_string(40, 50, WHITE, "STATS");
		tft_draw_string(40, 65, WHITE, "NEW CHARACTER");
		tft_draw_string(40, 65, WHITE, "");

		break;
	case STATS_MENU:
		tft_set_origin(0, 0);
		tft_draw_string(10, 10, YELLOW, "Time spend %-.2f H", menu.timespend / 60 / 60);
		//printf("time spend %f\n", menu.timespend);
		break;
	default:
		break;
	}
}

static void tiles()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (map[y][x]) {
				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_16x16_png,
					      map[y][x] - 1);
			}
		}
	}
}

static void player_paint(float x, float y, int direction)
{
	// Clamp values to valid range
	p.head = (p.head + PLAYER_SPRITES) % PLAYER_SPRITES;
	p.body = (p.body + PLAYER_SPRITES) % PLAYER_SPRITES;
	p.Larm = (p.Larm + PLAYER_SPRITES) % PLAYER_SPRITES;
	p.Rarm = (p.Rarm + PLAYER_SPRITES) % PLAYER_SPRITES;
	p.Lleg = (p.Lleg + PLAYER_SPRITES) % PLAYER_SPRITES;
	p.Rleg = (p.Rleg + PLAYER_SPRITES) % PLAYER_SPRITES;

	// Draw player parts in correct order (back to front)
	// Legs (behind body)
	sdk_draw_tile(x - 1, y - 1, &ts_lleg_2x2_png, p.Lleg * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 1, y - 1, &ts_rleg_2x2_png, p.Rleg * PLAYER_DIRECTION + direction);

	// Body (covers top of legs)
	sdk_draw_tile(x - 1, y - 5, &ts_body_4x4_png, p.body * PLAYER_DIRECTION + direction);

	// Head (on top of body)
	sdk_draw_tile(x - 2, y - 11, &ts_head_6x6_png, p.head * PLAYER_DIRECTION + direction);

	// Arms (on top of body)
	sdk_draw_tile(x - 2, y - 5, &ts_rarm_1x3_png, p.Rarm * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 3, y - 5, &ts_larm_1x3_png, p.Larm * PLAYER_DIRECTION + direction);
}

static void game_costume()
{
	tft_set_origin(0, 0);

	draw_bordered_rect(100, 50, 170, 130, DARK_GRAY, WHITE);
	player_paint(135, 90, p.d2);

	draw_bordered_rect(5, 5, 95, 175, DARK_BLUE, BLUE);

	int y_pos = 10 + menu.whut * 15;
	tft_draw_string(5, y_pos, YELLOW, ">");

	tft_draw_string(15, 10, WHITE, "Head  %i", p.head);
	tft_draw_string(15, 25, WHITE, "Body  %i", p.body);
	tft_draw_string(15, 40, WHITE, "R Arm %i", p.Rarm);
	tft_draw_string(15, 55, WHITE, "L Arm %i", p.Larm);
	tft_draw_string(15, 70, WHITE, "R Leg %i", p.Rleg);
	tft_draw_string(15, 85, WHITE, "L Leg %i", p.Lleg);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = false,
		.fps_color = rgb_to_rgb565(255, 255, 255),
	};

	sdk_main(&config);
}
