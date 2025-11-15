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
#include <stdarg.h>
#include <tiles-16x16.png.h>
#include <font-5x5.png.h>

// Multiplayer structures and variables
struct cursor {
	int id;
	int x, y;
	char name[16];
	int head, body, Larm, Rarm, Lleg, Rleg;
	int direction;
};

#define NUM_CURSORS 8
struct cursor cursors[NUM_CURSORS];

color_t colors[NUM_CURSORS] = {
	rgb_to_rgb565(255, 0, 0),     rgb_to_rgb565(0, 0, 255),	    rgb_to_rgb565(255, 255, 0),
	rgb_to_rgb565(255, 0, 255),   rgb_to_rgb565(0, 255, 255),   rgb_to_rgb565(127, 127, 127),
	rgb_to_rgb565(127, 127, 255), rgb_to_rgb565(255, 127, 127),
};

static int lx, ly;
static uint8_t device_id_16;
static int channel = SDK_RF_CHANNEL;

extern TileType maps_tuturial[MAP_ROWS][MAP_COLS];
TileType (*map)[MAP_COLS] = maps_tuturial;

typedef struct Save {
	int head, body, Larm, Rarm, Lleg, Rleg;
	char name[16];
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
static void game_read_player();
static void game_save_player();
static void game_start_menu();
static void world_start();
static void tiles();
static void player_paint(float x, float y, int direction, int head, int body, int Larm, int Rarm,
			 int Lleg, int Rleg);
static void draw_small_string(int x, int y, const char *format, ...);

// Multiplayer functions
void game_reset(void)
{
	device_id_16 = (0x9e3779b97f4a7801 * sdk_device_id) >> 48;

	for (int i = 0; i < NUM_CURSORS; i++)
		cursors[i].id = -1;
}

void game_inbox(sdk_message_t msg)
{
	if (SDK_MSG_RF == msg.type) {
		if (10 != msg.rf.length) {
			printf("game_inbox: invalid RF len=%i\n", msg.rf.length);
			return;
		}

		uint16_t id = (msg.rf.data[0] << 8) | msg.rf.data[1];

		for (int i = 0; i < NUM_CURSORS; i++) {
			if (cursors[i].id < 0 || cursors[i].id == id) {
				cursors[i].id = id;
				cursors[i].x = msg.rf.data[2];
				cursors[i].y = msg.rf.data[3];
				cursors[i].direction = msg.rf.data[4];
				cursors[i].head = msg.rf.data[5];
				cursors[i].body = msg.rf.data[6];
				cursors[i].Larm = msg.rf.data[7];
				cursors[i].Rarm = msg.rf.data[8];
				cursors[i].Lleg = msg.rf.data[9];

				// Create a simple name based on device ID
				snprintf(cursors[i].name, sizeof(cursors[i].name), "P%d", id);
				break;
			}
		}
	}
}

static void tx_cursor(void)
{
	static uint32_t last_tx;
	uint32_t now = time_us_32();

	if (now - last_tx < 50000) // Send every 50ms
		return;

	last_tx = now;

	// Send: device_id(2), x(1), y(1), direction(1), head(1), body(1), Larm(1), Rarm(1), Lleg(1)
	uint8_t msg[] = { device_id_16 >> 8,	device_id_16,	 (uint8_t)p.x,	  (uint8_t)p.y,
			  (uint8_t)p.direction, (uint8_t)p.head, (uint8_t)p.body, (uint8_t)p.Larm,
			  (uint8_t)p.Rarm,	(uint8_t)p.Lleg };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
}

static void game_read_player()
{
	Save save;
	sdk_save_read(0, &save, sizeof(save));

	memcpy(p.name, save.name, sizeof(p.name));

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

void game_start(void)
{
	k.ch = 1;
	menu.start = true;
	game_read_player();
	game_reset(); // Initialize multiplayer
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
	p.Rleg = 0;
}

static void zero_num()
{
	k.num0 = 0;
	k.num1 = 0;
	k.num2 = 0;
	k.num3 = 0;
	k.num4 = 0;
	k.num5 = 0;
	k.num6 = 0;
	k.num7 = 0;
	k.num8 = 0;
	k.num9 = 0;
}

static void game_start_inputs()
{
	if (sdk_inputs_delta.a == 1) {
		menu.P_start_y += 1;
		zero_num();
	}
	if (sdk_inputs_delta.b == 1) {
		menu.P_start_x += 1;
		zero_num();
	}
	if (sdk_inputs_delta.x == 1) {
		menu.P_start_x -= 1;
		zero_num();
	}
	if (sdk_inputs_delta.y == 1) {
		menu.P_start_y -= 1;
		zero_num();
	}

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
	if (sdk_inputs_delta.start == 1) {
		if (menu.P_start_x == 1 && menu.P_start_y == 0) {
			k.num1 += 1;
			if (k.num1 > 1)
				k.num1 = 0;
		}
		if (menu.P_start_x == 2 && menu.P_start_y == 0) {
			k.num2 += 1;
			if (k.num2 > 3)
				k.num2 = 0;
		}
		if (menu.P_start_x == 3 && menu.P_start_y == 0) {
			k.num3 += 1;
			if (k.num3 > 3)
				k.num3 = 0;
		}
		if (menu.P_start_x == 0 && menu.P_start_y == 1) {
			k.num0 += 1;
			if (k.num0 > 1)
				k.num0 = 0;
		}
		if (menu.P_start_x == 1 && menu.P_start_y == 1) {
			k.num4 += 1;
			if (k.num4 > 3)
				k.num4 = 0;
		}
		if (menu.P_start_x == 2 && menu.P_start_y == 1) {
			k.num5 += 1;
			if (k.num5 > 3)
				k.num5 = 0;
		}
		if (menu.P_start_x == 3 && menu.P_start_y == 1) {
			k.num6 += 1;
			if (k.num6 > 3)
				k.num6 = 0;
		}
		if (menu.P_start_x == 1 && menu.P_start_y == 2) {
			k.num7 += 1;
			if (k.num7 > 3)
				k.num7 = 0;
		}
		if (menu.P_start_x == 2 && menu.P_start_y == 2) {
			k.num8 += 1;
			if (k.num8 > 3)
				k.num8 = 0;
		}
		if (menu.P_start_x == 3 && menu.P_start_y == 2) {
			k.num9 += 1;
			if (k.num9 > 3)
				k.num9 = 0;
		}
	}
	if (sdk_inputs_delta.select == 1) {
		world_start();
		menu.costume = true;
		menu.start = false;
	}

	if (sdk_inputs_delta.vol_up == 1) {
		k.ch += 1;
		zero_num();
	}
	if (sdk_inputs_delta.vol_sw == 1) {
		k.ch += 1;
		zero_num();
	}
	if (sdk_inputs_delta.vol_down == 1) {
		k.ch -= 1;
		zero_num();
	}
	if (k.ch > 15) {
		k.ch = 1;
	}
	if (k.ch < 1) {
		k.ch = 15;
	}
	return;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	// Channel switching (from multiplayer example)
	if (sdk_inputs_delta.vertical < 0) {
		channel = clamp(channel + 1, SDK_RF_CHANNEL_MIN, SDK_RF_CHANNEL_MAX);
		sdk_set_rf_channel(channel);
	}

	if (sdk_inputs_delta.vertical > 0) {
		channel = clamp(channel - 1, SDK_RF_CHANNEL_MIN, SDK_RF_CHANNEL_MAX);
		sdk_set_rf_channel(channel);
	}

	if (menu.start) {
		game_start_inputs();
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

	// Movement input
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

	// Send position to other players
	if (p.fx != 0 || p.fy != 0) {
		tx_cursor();
	}
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
		game_save_player();
		return;
	}

	tft_set_origin(p.x - TFT_WIDTH / 2.0 + PLAYER_WIDTH / 2.0,
		       p.y - TFT_HEIGHT / 2.0 + PLAYER_HEIGHT / 2.0);
	tiles();

	// Draw current player
	player_paint(p.x, p.y, p.direction, p.head, p.body, p.Larm, p.Rarm, p.Lleg, p.Rleg);

	// Draw player name above current player
	tft_set_origin(0, 0); // Reset origin for UI elements
	draw_small_string(p.x - 20, p.y - 40, "%s", p.name);

	// Draw other players
	for (int i = 0; i < NUM_CURSORS; i++) {
		if (cursors[i].id < 0 || cursors[i].id == device_id_16)
			continue;

		// Set origin for world rendering of other players
		tft_set_origin(p.x - TFT_WIDTH / 2.0 + PLAYER_WIDTH / 2.0,
			       p.y - TFT_HEIGHT / 2.0 + PLAYER_HEIGHT / 2.0);

		// Draw other player with their customizations
		player_paint(cursors[i].x, cursors[i].y, cursors[i].direction, cursors[i].head,
			     cursors[i].body, cursors[i].Larm, cursors[i].Rarm, cursors[i].Lleg,
			     cursors[i].Lleg); // Using Lleg for both legs

		// Draw other player's name
		tft_set_origin(0, 0);
		draw_small_string(cursors[i].x - 20, cursors[i].y - 40, "%s", cursors[i].name);
	}

	// Draw channel info (from multiplayer example)
	tft_set_origin(0, 0);
	tft_draw_string(0, 0, rgb_to_rgb565(127, 0, 0), "CH:%i", channel);
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
	int current_word = 0;

	if (k.num2 > 0) {
		if (k.num2 == 1) {
			current_word = 65;
		}
		if (k.num2 == 2) {
			current_word = 66;
		}
		if (k.num2 == 3) {
			current_word = 67;
		}
	}
	if (k.num3 > 0) {
		if (k.num3 == 1) {
			current_word = 68;
		}
		if (k.num3 == 2) {
			current_word = 69;
		}
		if (k.num3 == 3) {
			current_word = 70;
		}
	}
	if (k.num4 > 0) {
		if (k.num4 == 1) {
			current_word = 71;
		}
		if (k.num4 == 2) {
			current_word = 72;
		}
		if (k.num4 == 3) {
			current_word = 73;
		}
	}
	if (k.num5 > 0) {
		if (k.num5 == 1) {
			current_word = 74;
		}
		if (k.num5 == 2) {
			current_word = 75;
		}
		if (k.num5 == 3) {
			current_word = 76;
		}
	}
	if (k.num6 > 0) {
		if (k.num6 == 1) {
			current_word = 77;
		}
		if (k.num6 == 2) {
			current_word = 78;
		}
		if (k.num6 == 3) {
			current_word = 79;
		}
	}
	if (k.num7 > 0) {
		if (k.num7 == 1) {
			current_word = 81;
		}
		if (k.num7 == 2) {
			current_word = 82;
		}
		if (k.num7 == 3) {
			current_word = 83;
		}
	}
	if (k.num8 > 0) {
		if (k.num8 == 1) {
			current_word = 84;
		}
		if (k.num8 == 2) {
			current_word = 85;
		}
		if (k.num8 == 3) {
			current_word = 86;
		}
	}
	if (k.num9 > 0) {
		if (k.num9 == 1) {
			current_word = 87;
		}
		if (k.num9 == 2) {
			current_word = 88;
		}
		if (k.num9 == 3) {
			current_word = 89;
		}
	}
	switch (k.ch) {
	case 1:
		p.ch1 = current_word;
		break;
	case 2:
		p.ch2 = current_word;
		break;
	case 3:
		p.ch3 = current_word;
		break;
	case 4:
		p.ch4 = current_word;
		break;
	case 5:
		p.ch5 = current_word;
		break;
	case 6:
		p.ch6 = current_word;
		break;
	case 7:
		p.ch7 = current_word;
		break;
	case 8:
		p.ch8 = current_word;
		break;
	case 9:
		p.ch9 = current_word;
		break;
	case 10:
		p.ch10 = current_word;
		break;
	case 11:
		p.ch11 = current_word;
		break;
	case 12:
		p.ch12 = current_word;
		break;
	case 13:
		p.ch13 = current_word;
		break;
	case 14:
		p.ch14 = current_word;
		break;
	case 15:
		p.ch15 = current_word;
		break;
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

static void player_paint(float x, float y, int direction, int head, int body, int Larm, int Rarm,
			 int Lleg, int Rleg)
{
	// Clamp values to valid ranges
	if (Lleg > PLAYER_SPRITES - 1)
		Lleg = 0;
	if (Rleg > PLAYER_SPRITES - 1)
		Rleg = 0;
	if (body > PLAYER_SPRITES - 1)
		body = 0;
	if (head > PLAYER_SPRITES - 1)
		head = 0;
	if (Larm > PLAYER_SPRITES - 1)
		Larm = 0;
	if (Rarm > PLAYER_SPRITES - 1)
		Rarm = 0;

	if (Lleg < 0)
		Lleg = PLAYER_SPRITES;
	if (Rleg < 0)
		Rleg = PLAYER_SPRITES;
	if (body < 0)
		body = PLAYER_SPRITES;
	if (head < 0)
		head = PLAYER_SPRITES;
	if (Rarm < 0)
		Rarm = PLAYER_SPRITES;
	if (Larm < 0)
		Larm = PLAYER_SPRITES;

	// x and y its bottom center

	// legs
	sdk_draw_tile(x - 1, y - 1, &ts_lleg_2x2_png, Lleg * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 1, y - 1, &ts_rleg_2x2_png, Rleg * PLAYER_DIRECTION + direction);

	// body
	sdk_draw_tile(x - 1, y - 5, &ts_body_4x4_png, body * PLAYER_DIRECTION + direction);

	// Head
	sdk_draw_tile(x - 2, y - 11, &ts_head_6x6_png, head * PLAYER_DIRECTION + direction);

	// right arm
	sdk_draw_tile(x - 2, y - 5, &ts_rarm_1x3_png, Rarm * PLAYER_DIRECTION + direction);

	// left arm
	sdk_draw_tile(x + 3, y - 5, &ts_larm_1x3_png, Larm * PLAYER_DIRECTION + direction);
}

static void game_costume()
{
	tft_set_origin(0, 0);
	player_paint(135, 90, p.d2, p.head, p.body, p.Larm, p.Rarm, p.Lleg, p.Rleg);

	tft_draw_rect(100, 8 + menu.whut * 11, 104, 12 + menu.whut * 11, WHITE);
	tft_draw_string(5, 05, WHITE, "Head      %i", p.head);
	tft_draw_string(5, 16, WHITE, "Body      %i", p.body);
	tft_draw_string(5, 27, WHITE, "Right arm %i", p.Rarm);
	tft_draw_string(5, 38, WHITE, "Left arm  %i", p.Larm);
	tft_draw_string(5, 49, WHITE, "Right leg %i", p.Rleg);
	tft_draw_string(5, 60, WHITE, "Left leg  %i", p.Lleg);
}

static void draw_small_string(int x, int y, const char *format, ...)
{
	char buffer[64];
	va_list args;

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	int current_x = x;
	const char *str = buffer;

	while (*str) {
		char c = *str++;
		int tile_index;

		if (c >= 'a' && c <= 'z') {
			tile_index = c - 'a'; // a=0, b=1, ..., z=25
		} else if (c >= '0' && c <= '9') {
			tile_index = 26 + (c - '0'); // 0=26, 1=27, ..., 9=35
		} else {
			tile_index = 36; // Default for unsupported characters
		}

		if (tile_index != 36)
			sdk_draw_tile(current_x, y, &ts_font_5x5_png, tile_index);
		current_x += 6;
	}
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
