#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>
#include "common.h"

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
NUM Num;
walls W[256];
sectors S[128];
TexureMaps Textures[64];

extern void textures_load();
extern void drawpixel(int x, int y, int r, int g, int b);
extern void draw_3d();

static void load_sectors()
{
	FILE *fp = fopen("level.h", "r");
	if (fp == NULL) {
		printf("OH NOO AN ERROR level.h\n");
		return;
	}
	int s;
	//int w;

	fscanf(fp, "%i", &Num.Sect);   //number of sectors
	for (s = 0; s < Num.Sect; s++) //load all sectors
	{
		fscanf(fp, "%i", &S[s].ws);
		fscanf(fp, "%i", &S[s].we);
		fscanf(fp, "%i", &S[s].z1);
		fscanf(fp, "%i", &S[s].z2);
		fscanf(fp, "%i", &S[s].st);
		fscanf(fp, "%i", &S[s].ss);
	}
	fscanf(fp, "%i", &Num.Wall);   //number of walls
	for (s = 0; s < Num.Wall; s++) //load all walls
	{
		fscanf(fp, "%i", &W[s].x1);
		fscanf(fp, "%i", &W[s].y1);
		fscanf(fp, "%i", &W[s].x2);
		fscanf(fp, "%i", &W[s].y2);
		fscanf(fp, "%i", &W[s].wt);
		fscanf(fp, "%i", &W[s].u);
		fscanf(fp, "%i", &W[s].v);
		fscanf(fp, "%i", &W[s].shade);
	}
	fclose(fp);
}
static void load()
{
	FILE *fp = fopen("level.h", "r");
	if (fp == NULL) {
		printf("OH NOO AN ERROR level.h\n");
		return;
	}
	int s;
	//int w;

	fscanf(fp, "%i", &Num.Sect);   //number of sectors
	for (s = 0; s < Num.Sect; s++) //load all sectors
	{
		fscanf(fp, "%i", &S[s].ws);
		fscanf(fp, "%i", &S[s].we);
		fscanf(fp, "%i", &S[s].z1);
		fscanf(fp, "%i", &S[s].z2);
		fscanf(fp, "%i", &S[s].st);
		fscanf(fp, "%i", &S[s].ss);
	}
	fscanf(fp, "%i", &Num.Wall);   //number of walls
	for (s = 0; s < Num.Wall; s++) //load all walls
	{
		fscanf(fp, "%i", &W[s].x1);
		fscanf(fp, "%i", &W[s].y1);
		fscanf(fp, "%i", &W[s].x2);
		fscanf(fp, "%i", &W[s].y2);
		fscanf(fp, "%i", &W[s].wt);
		fscanf(fp, "%i", &W[s].u);
		fscanf(fp, "%i", &W[s].v);
		fscanf(fp, "%i", &W[s].shade);
	}
	fscanf(fp, "%i %i %i %i %i", &P.x, &P.y, &P.z, &P.a,
	       &P.l); //player position, angle, look direction
	fclose(fp);
}

void game_start(void)
{
	// store in degress
	int x;
	for (x = 0; x < 360; x++) {
		M.cos[x] = cos(x / 180.0 * M_PI);
		M.sin[x] = sin(x / 180.0 * M_PI);
	}

	Num.Text = 20;
	//starter player
	P.x = 0;
	P.y = -10;
	P.z = 10;
	P.a = 0;
	P.l = 0;
	textures_load();
	load();
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
		load_sectors();
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
