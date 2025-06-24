#include "common.h"
#include <head-6x6.png.h>
#include <lleg-2x2.png.h>
#include <rleg-2x2.png.h>
#include <body-4x4.png.h>
#include <rarm-1x3.png.h>
#include <larm-1x3.png.h>
#include <sdk.h>
#include <stdio.h>
#include <tiles-16x16.png.h>

extern TileType maps_tuturial[MAP_ROWS][MAP_COLS];

TileType (*map)[MAP_COLS] = maps_tuturial;

typedef struct {
	float x, y;
	float fx, fy;
	int tile_x, tile_y;
	float speed;
	int head, body, Larm, Rarm, Lleg, Rleg;
	int direction, d2;
} Player;

typedef struct {
	bool costume;
	int whut;
} Menu;

static Menu menu;
static Player p;

static void game_costume();
static void tiles();
static void player_paint(float x, float y, int direction);

void game_start(void)
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
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

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
	if (menu.costume) {
		game_costume();
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
static void player_paint(float x, float y, int direction)
{
	if (p.Lleg > PLAYER_SPRITES - 1)
		p.Lleg = 0;

	if (p.Rleg > PLAYER_SPRITES - 1)
		p.Lleg = 0;

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
	sdk_draw_tile(x - 1, y - 1, &ts_lleg_2x2_png, p.Lleg * 4 + direction);
	sdk_draw_tile(x + 1, y - 1, &ts_rleg_2x2_png, p.Rleg * 4 + direction);

	// body
	sdk_draw_tile(x - 1, y - 5, &ts_body_4x4_png, p.body * 4 + direction);

	// Head
	sdk_draw_tile(x - 2, y - 11, &ts_head_6x6_png, p.head * 4 + direction);

	// right arm
	sdk_draw_tile(x - 2, y - 5, &ts_rarm_1x3_png, p.Rarm * 4 + direction);

	// left arm
	sdk_draw_tile(x + 3, y - 5, &ts_larm_1x3_png, p.Larm * 4 + direction);

	// pixel
	//tft_draw_pixel(x, y, WHITE);
}
static void game_costume()
{
	tft_set_origin(0, 0);
	player_paint(135, 90, p.d2);

	// 120 , 8, 124 , 12

	tft_draw_rect(100, 8 + menu.whut * 11, 104, 12 + menu.whut * 11, WHITE);

	tft_draw_string(5, 5, WHITE, "Head      %i", p.head);

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
