#include "common.h"
#include <head-6x6.png.h>
#include <lleg-2x2.png.h>
#include <rleg-2x2.png.h>
#include <body-4x4.png.h>
#include <rarm-1x3.png.h>
#include <larm-1x3.png.h>
#include <sdk.h>
#include <stdio.h>
#include <string.h>
#include <tiles-16x16.png.h>
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

typedef struct Save {
	int head, body, Larm, Rarm, Lleg, Rleg;
} Save;

typedef struct {
	float x, y;
	float fx, fy;
	int tile_x, tile_y;
	float speed;
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
	int ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10, ch11, ch12, ch13, ch14, ch15;
	int direction, d2;
} Player;

typedef struct {
	int num1, num2, num3, num4, num5, num6, num7, num8, num9, num0;
	int ch;
} key;
typedef struct {
	bool costume;
	int whut;
	int P_start_x;
	int P_start_y;
	bool start;
	bool init;
} Menu;

static Menu menu;
static Player p;
static key k;

static void game_costume();
static void game_read_costume();
static void game_save_costume();
static void game_start_menu();
static void world_start();
static void tiles();
static void player_paint(float x, float y, int direction);

static void game_read_costume()
{
	Save save;
	sdk_save_read(0, &save, sizeof(save));

	p.head = save.head;
	p.body = save.body;
	p.Larm = save.Larm;
	p.Rarm = save.Rarm;
	p.Lleg = save.Lleg;
	p.Rleg = save.Rleg;
}

static void game_save_costume()
{
	Save save = {
		.head = p.head,
		.body = p.body,
		.Larm = p.Larm,
		.Rarm = p.Rarm,
		.Lleg = p.Lleg,
		.Rleg = p.Rleg,
	};

	sdk_save_write(0, &save, sizeof(save));
}

void game_start(void)
{
	k.ch = 0;
	menu.start = true;
}
static void world_start(void)
{
	p.x = 20;
	p.y = 15;
	p.speed = 50;
	p.d2 = 0;

	p.head = 0;
	p.body = 0;
	p.Larm = 0;
	p.Rarm = 0;
	p.Lleg = 0;
	p.Lleg = 0;
	game_read_costume();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	if (menu.start) {
		if (sdk_inputs_delta.a == 1) {
			menu.P_start_y += 1;
		}
		if (sdk_inputs_delta.b == 1) {
			menu.P_start_x += 1;
		}
		if (sdk_inputs_delta.x == 1) {
			menu.P_start_x -= 1;
		}
		if (sdk_inputs_delta.y == 1) {
			menu.P_start_y -= 1;
		}
		if (sdk_inputs_delta.aux[1]) {
		}

		if (sdk_inputs_delta.vol_sw) {
			world_start();
			menu.costume = true;
			menu.start = false;
		}
		return;
	}

	if (sdk_inputs_delta.start == 1 && menu.costume == false) {
		menu.costume = true;
		return;
	}
	if (menu.costume) {
		if (sdk_inputs_delta.start == 1) {
			menu.costume = false;
		}
		if (sdk_inputs_delta.a == 1) {
			menu.whut += 1;
			if (menu.whut > 5)
				menu.whut = 0;
		}
		if (sdk_inputs_delta.y == 1) {
			menu.whut -= 1;
			if (menu.whut < 0)
				menu.whut = 5;
		}
		if (sdk_inputs_delta.b == 1) {
			if (menu.whut == 0)
				p.head += 1;
			if (menu.whut == 1)
				p.body += 1;
			if (menu.whut == 2)
				p.Rarm += 1;
			if (menu.whut == 3)
				p.Larm += 1;
			if (menu.whut == 4)
				p.Rleg += 1;
			if (menu.whut == 5)
				p.Lleg += 1;
		}
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
	if (menu.start) {
		game_start_menu();
		return;
	}
	if (menu.costume) {
		game_costume();
		game_save_costume();
		return;
	}
	tft_set_origin(p.x - TFT_WIDTH / 2.0 + PLAYER_WIDTH / 2.0,
		       p.y - TFT_HEIGHT / 2.0 + PLAYER_HEIGHT / 2.0);
	tiles();
	player_paint(p.x, p.y, p.direction);
}

static void tiles()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (map[y][x])
				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_16x16_png,
					      map[y][x] - 1);
		}
	}
}
static void draw_rect_ins(int x1, int y1, int x2, int y2, color_t color)
{
	tft_draw_rect(x1, y1, x2, y1, color);
	tft_draw_rect(x1, y2, x2, y2, color);
	tft_draw_rect(x1, y1, x1, y2, color);
	tft_draw_rect(x2, y1, x2, y2, color);
}

static void game_start_menu()
{
	if (menu.P_start_x > 3 || menu.P_start_x < 0) {
		menu.P_start_x = 0;
	}

	if (menu.P_start_y > 2 || menu.P_start_y < 0) {
		menu.P_start_y = 0;
	}
	if (menu.P_start_x == 0 && menu.P_start_y == 0) {
		menu.P_start_x = 1;
	}
	if (menu.P_start_x == 0 && menu.P_start_y == 2) {
		menu.P_start_x = 1;
	}

	draw_rect_ins(menu.P_start_x * 40 - 2, menu.P_start_y * 30 + 10, menu.P_start_x * 40 + 7,
		      menu.P_start_y * 30 + 23, WHITE);

	char name[] = { p.ch1, p.ch2,  p.ch3,  p.ch4,  p.ch5,  p.ch6,  p.ch7,  p.ch8,
			p.ch9, p.ch10, p.ch11, p.ch12, p.ch13, p.ch14, p.ch15, 0 };
	strcpy(p.name, name);

	tft_draw_string(70, 100, WHITE, "%s\n", p.name);

	tft_draw_string(10, 100, WHITE, "-Start-");
	tft_draw_string(40, 10, WHITE, "1");
	tft_draw_string(80, 10, WHITE, "2");
	tft_draw_string(120, 10, WHITE, "3");
	tft_draw_string(40, 40, WHITE, "4");
	tft_draw_string(80, 40, WHITE, "5");
	tft_draw_string(120, 40, WHITE, "6");
	tft_draw_string(40, 70, WHITE, "7");
	tft_draw_string(80, 70, WHITE, "8");
	tft_draw_string(120, 70, WHITE, "9");
	tft_draw_string(0, 40, WHITE, "0");
}
static void player_paint(float x, float y, int direction)
{
	if (p.Lleg > PLAYER_SPRITES - 1)
		p.Lleg = 0;

	if (p.Rleg > PLAYER_SPRITES - 1)
		p.Rleg = 0;

	if (p.body > PLAYER_SPRITES - 1)
		p.body = 0;

	if (p.head > PLAYER_SPRITES - 1)
		p.head = 0;

	if (p.Larm > PLAYER_SPRITES - 1)
		p.Larm = 0;

	if (p.Rarm > PLAYER_SPRITES - 1)
		p.Rarm = 0;

	if (p.Lleg < 0)
		p.Lleg = PLAYER_SPRITES;

	if (p.Rleg < 0)
		p.Rleg = PLAYER_SPRITES;

	if (p.body < 0)
		p.body = PLAYER_SPRITES;

	if (p.head < 0)
		p.head = PLAYER_SPRITES;

	if (p.Rarm < 0)
		p.Rarm = PLAYER_SPRITES;

	if (p.Larm < 0)
		p.Larm = PLAYER_SPRITES;

	// x and y its bottom center

	// legs
	sdk_draw_tile(x - 1, y - 1, &ts_lleg_2x2_png, p.Lleg * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 1, y - 1, &ts_rleg_2x2_png, p.Rleg * PLAYER_DIRECTION + direction);

	// body
	sdk_draw_tile(x - 1, y - 5, &ts_body_4x4_png, p.body * PLAYER_DIRECTION + direction);

	// Head
	sdk_draw_tile(x - 2, y - 11, &ts_head_6x6_png, p.head * PLAYER_DIRECTION + direction);

	// right arm
	sdk_draw_tile(x - 2, y - 5, &ts_rarm_1x3_png, p.Rarm * PLAYER_DIRECTION + direction);

	// left arm
	sdk_draw_tile(x + 3, y - 5, &ts_larm_1x3_png, p.Larm * PLAYER_DIRECTION + direction);

	// pixel
	//tft_draw_pixel(x, y, WHITE);
}
static void game_costume()
{
	tft_set_origin(0, 0);
	player_paint(135, 90, p.d2);

	// 120 , 8, 124 , 12

	tft_draw_rect(100, 8 + menu.whut * 11, 104, 12 + menu.whut * 11, WHITE);
	tft_draw_string(5, 05, WHITE, "Head      %i", p.head);

	tft_draw_string(5, 16, WHITE, "Body      %i", p.body);

	tft_draw_string(5, 27, WHITE, "Right arm %i", p.Rarm);
	tft_draw_string(5, 38, WHITE, "Left arm  %i", p.Larm);

	tft_draw_string(5, 49, WHITE, "Right leg %i", p.Rleg);
	tft_draw_string(5, 60, WHITE, "Left leg  %i", p.Lleg);
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
