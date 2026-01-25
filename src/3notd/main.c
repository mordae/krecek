#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>
#include "common.h"
#include "level_data.h"

//    TODO    //
//    Menu    //
//    editor  //

//--------------------------------------
//             3NOTD
// includes DUKE NUKE 3D Grafics
//
//--------------------------------------

player P;
math M;
TextureMaps Textures[64];

struct Map current_map;
extern void textures_load();
extern void drawpixel(int x, int y, int r, int g, int b);
extern void draw_3d();
extern void legacy_load_sectors(void);

void game_start(void)
{
	// store in degress
	int x;
	for (x = 0; x < 360; x++) {
		M.cos[x] = cos(x / 180.0 * M_PI);
		M.sin[x] = sin(x / 180.0 * M_PI);
	}

	//starter player
	P.x = 0;
	P.y = -10;
	P.z = 10;
	P.a = 0;
	P.l = 0;
	textures_load();
	//legacy_load_sectors();
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	int dx = (M.sin[P.a] * WALKING_POWER) * dt;
	int dy = (M.cos[P.a] * WALKING_POWER) * dt;

	// Looking left right
	if (sdk_inputs.joy_x > 500 && sdk_inputs.start && sdk_inputs.select == 0) {
		printf("looking right\n");
		P.a += LOOKING_POWER * dt;
		if (P.a > 359) {
			P.a -= 360;
		}
	}
	if (sdk_inputs.joy_x < -500 && sdk_inputs.start && sdk_inputs.select == 0) {
		printf("looking left\n");
		P.a -= LOOKING_POWER * dt;
		if (P.a < 0) {
			P.a += 360;
		}
	}
	//moving

	if (sdk_inputs_delta.aux[1] == 1) {
		legacy_load_sectors();
	}

	if (sdk_inputs.joy_y < -500 && sdk_inputs.start == 0 && sdk_inputs.select == 0) {
		printf("moving forward\n");
		P.x += dx;
		P.y += dy;
	}
	if (sdk_inputs.joy_y > 500 && sdk_inputs.start == 0 && sdk_inputs.select == 0) {
		printf("moving back\n");
		P.x -= dx;
		P.y -= dy;
	}

	//strafe left, right
	if (sdk_inputs.joy_x > 500 && sdk_inputs.start == 0 && sdk_inputs.select == 0) {
		printf("strafe right\n");
		P.x += dy;
		P.y -= dx;
	}
	if (sdk_inputs.joy_x < -500 && sdk_inputs.start == 0 && sdk_inputs.select == 0) {
		printf("strafe left\n");
		P.x -= dy;
		P.y += dx;
	}

	//move up,down
	if (sdk_inputs.joy_y < -500 && sdk_inputs.select && sdk_inputs.start == 0) {
		printf("move up\n");
		P.z -= LOOKING_POWER * dt;
	}
	if (sdk_inputs.joy_y > 500 && sdk_inputs.select && sdk_inputs.start == 0) {
		printf("move down\n");
		P.z += LOOKING_POWER * dt;
	}
	//look up,down
	if (sdk_inputs.joy_y < -500 && sdk_inputs.start && sdk_inputs.select == 0) {
		printf("looking up\n");
		P.l += LOOKING_POWER * dt;
	}
	if (sdk_inputs.joy_y > 500 && sdk_inputs.start && sdk_inputs.select == 0) {
		printf("looking down\n");
		P.l -= LOOKING_POWER * dt;
	}
}
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(rgb_to_rgb565(0, 60, 130));
	draw_3d();
	//testTextures(4);
}
int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = WHITE,
	};
	sdk_main(&config);
}
