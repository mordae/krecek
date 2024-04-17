#include "sdk/game.h"
#include "sdk/input.h"
#include "sys/_types.h"
#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>

#include <sdk.h>
#include <tft.h>

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

#define SPACESIZE 5
#define MOVEWAIT 200000

static int score = 0;
static int snakeheadx = 5;
static int snakeheady = 5;
static int snakedeltax = 0;
static int snakedeltay = 0;

static int since_last_move = 0;

void game_reset(void)
{
	score = 0;
}

void game_start(void)
{
	sdk_set_output_gain_db(6);
}

void game_audio(int nsamples)
{
}

void game_input(unsigned dt_usec)
{
	if (sdk_inputs_delta.b > 0) {
		snakedeltax = 1;
		snakedeltay = 0;
	} 
	if (sdk_inputs_delta.x > 0) {
		snakedeltax = -1;
		snakedeltay = 0;
	}
	if (sdk_inputs_delta.a > 0) {
		snakedeltay = 1;
		snakedeltax = 0;
	}
	if (sdk_inputs_delta.y > 0) {
		snakedeltay = -1;
		snakedeltax = 0;
	}

	if (since_last_move > MOVEWAIT) {
		since_last_move -= MOVEWAIT;
		snakeheadx += snakedeltax;
		snakeheady += snakedeltay;
	}

	since_last_move += dt_usec;
	
	if (snakeheadx < 0)
		snakeheadx = 0;
	if (snakeheadx > 31)
		snakeheadx = 31;
	if (snakeheady < 0)
		snakeheady = 0;
	if (snakeheady > 23)
		snakeheady = 23;
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);
	char buf[64];

	snprintf(buf, sizeof buf, "%i", score);
	tft_draw_string(0, 0, RED, buf);


	tft_draw_rect(SPACESIZE * snakeheadx,
	              SPACESIZE * snakeheady,
	              SPACESIZE * snakeheadx + SPACESIZE - 1,
	              SPACESIZE * snakeheady + SPACESIZE - 1,
	              GREEN);
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
