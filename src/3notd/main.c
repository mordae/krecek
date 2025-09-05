#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>

#include "pixel.h"

//    TODO    //
//    3D      //
//    texutes //
//    editor  //

#define LOOKING_POWER 80
#define WALKING_POWER 140

#define BLUE rgb_to_rgb565(0, 0, 255)
#define YELLOW rgb_to_rgb565(160, 160, 0)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define TFT_WIDTH2 TFT_WIDTH / 2
#define TFT_HEIGHT2 TFT_HEIGHT / 2

#include "assets/T_00.h"
#include "assets/T_01.h"
#include "assets/T_02.h"
#include "assets/T_03.h"
#include "assets/T_04.h"
#include "assets/T_05.h"
#include "assets/T_06.h"
#include "assets/T_07.h"
#include "assets/T_08.h"
#include "assets/T_09.h"
#include "assets/T_10.h"
#include "assets/T_11.h"
#include "assets/T_12.h"
#include "assets/T_13.h"
#include "assets/T_14.h"
#include "assets/T_15.h"
#include "assets/T_16.h"
#include "assets/T_17.h"
#include "assets/T_18.h"
#include "assets/T_19.h"
int numText = 19; //number of assets
int numSect = 0;  //number of sectors
int numWall = 0;  //number of walls

//------------------------------------------------------------------------------

typedef struct {
	float cos[360];
	float sin[360];
} math;

typedef struct {
	int x, y, z;
	int a;
	int l;
} player;
typedef struct {
	int x1, y1; //bottom line point 1
	int x2, y2; //bottom line point 2
	int c;
	int wt, u, v; //wall texture and u/v tile
	int shade;    //shade of the wall
} walls;

typedef struct {
	int ws, we; //wall number start and end
	int z1, z2; //height of bottom and top
	int d;
	int c1, c2;
	int st, ss;	     //surface texture, surface scale
	int surf[TFT_WIDTH]; //to hold points for surfaces
	int surface;
} sectors;

typedef struct {
	int w, h;		   //texture width/height
	const unsigned char *name; //texture name
} TexureMaps;
TexureMaps Textures[64];
static walls W[256];
static sectors S[128];
static player P;
static math M;

void load()
{
	FILE *fp = fopen("level.h", "r");
	if (fp == NULL) {
		printf("Error opening level.h");
		return;
	}
	int s, w;

	fscanf(fp, "%i", &numSect);   //number of sectors
	for (s = 0; s < numSect; s++) //load all sectors
	{
		fscanf(fp, "%i", &S[s].ws);
		fscanf(fp, "%i", &S[s].we);
		fscanf(fp, "%i", &S[s].z1);
		fscanf(fp, "%i", &S[s].z2);
		fscanf(fp, "%i", &S[s].st);
		fscanf(fp, "%i", &S[s].ss);
	}
	fscanf(fp, "%i", &numWall);   //number of walls
	for (s = 0; s < numWall; s++) //load all walls
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

	//starter player
	P.x = 70;
	P.y = -110;
	P.z = 20;
	P.a = 0;
	P.l = 0;
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
	if (sdk_inputs.joy_x > 500 && sdk_inputs.start == 0) {
		printf("looking left\n");
		P.a += LOOKING_POWER * dt;
		if (P.a > 359) {
			P.a -= 360;
		}
	}
	if (sdk_inputs.joy_x < -500 && sdk_inputs.start == 0) {
		printf("looking right\n");
		P.a -= LOOKING_POWER * dt;
		if (P.a < 0) {
			P.a += 360;
		}
	}
	//moving

	if (sdk_inputs.y && sdk_inputs.start == 0) {
		printf("moving forward\n");
		P.x += dx;
		P.y += dy;
	}
	if (sdk_inputs.joy_y < -500 && sdk_inputs.start) {
		printf("moving forward\n");
		P.x += dx;
		P.y += dy;
	}
	if (sdk_inputs.joy_y > 500 && sdk_inputs.start) {
		printf("moving back\n");
		P.x -= dx;
		P.y -= dy;
	}

	//strafe left, right
	if (sdk_inputs.joy_x > 500 && sdk_inputs.start) {
		printf("strafe left\n");
		P.x += dy;
		P.y -= dx;
	}
	if (sdk_inputs.joy_x < -500 && sdk_inputs.start) {
		printf("strafe right\n");
		P.x -= dy;
		P.y += dx;
	}

	//move up,down
	if (sdk_inputs.joy_y < -500 && sdk_inputs.start == 0) {
		printf("move up\n");
		P.z -= LOOKING_POWER * dt;
	}
	if (sdk_inputs.joy_y > 500 && sdk_inputs.start == 0) {
		printf("move down\n");
		P.z += LOOKING_POWER * dt;
	}
}

static void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2) //clip line
{
	float da = *y1; //distance plane -> point a
	float db = y2;	//distance plane -> point b
	float d = da - db;
	if (d == 0) {
		d = 1;
	}
	float s = da / (da - db); //intersection factor (between 0 and 1)
	*x1 = *x1 + s * (x2 - (*x1));
	*y1 = *y1 + s * (y2 - (*y1));
	if (*y1 == 0) {
		*y1 = 1;
	} //prevent divide by zero
	*z1 = *z1 + s * (z2 - (*z1));
}

static void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack)
{
	int x, y;
	//Hold diffrent
	int dyb = b2 - b1;
	int dyt = t2 - t1;
	int dx = x2 - x1;
	if (dx == 0) {
		dx = 1;
	}
	int xs = x1;
	//CLIP X
	if (x1 < 0) {
		x1 = 0;
	}
	if (x2 < 0) {
		x2 = 0;
	}
	if (x1 > TFT_WIDTH) {
		x1 = TFT_WIDTH;
	}
	if (x2 > TFT_WIDTH) {
		x2 = TFT_WIDTH;
	}

	for (x = x1; x < x2; x++) {
		int y1 = dyb * (x - xs + 0.5) / dx + b1;
		int y2 = dyt * (x - xs + 0.5) / dx + t1;

		//CLIP Y
		if (y1 < 0) {
			y1 = 0;
		}
		if (y2 < 0) {
			y2 = 0;
		}
		if (y1 > TFT_HEIGHT) {
			y1 = TFT_HEIGHT;
		}
		if (y2 > TFT_HEIGHT) {
			y2 = TFT_HEIGHT;
		}
		//draw front wall
		if (frontBack == 0) {
			if (S[s].surface == 1) {
				S[s].surf[x] = y1;
			}
			if (S[s].surface == 2) {
				S[s].surf[x] = y2;
			}
			for (y = y1; y < y2; y++) {
				drawpixel(x, y, 0);
			} //normal wall
		}
		if (frontBack == 1) {
			if (S[s].surface == 1) {
				y2 = S[s].surf[x];
			}
			if (S[s].surface == 2) {
				y1 = S[s].surf[x];
			}
			for (y = y1; y < y2; y++) {
				drawpixel(x, y, 2);
			} //normal wall
		}
	}
}
static int dist(int x1, int y1, int x2, int y2)
{
	int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return distance;
}
static void draw_3d(void)
{
	int x, s, w, frontBack, cycles, wx[4], wy[4], wz[4];
	float CS = M.cos[P.a], SN = M.sin[P.a];
	//oreder sectors
	for (s = 0; s < numSect - 1; s++) {
		for (w = 0; w < numSect - s - 1; w++) {
			if (S[w].d < S[w + 1].d) {
				sectors st = S[w];
				S[w] = S[w + 1];
				S[w + 1] = st;
			}
		}
	}

	//draw sectors
	for (s = 0; s < numSect; s++) {
		S[s].d = 0;
		if (P.z < S[s].z1) {
			S[s].surface = 1;
			cycles = 2;
			for (x = 0; x < TFT_WIDTH; x++) {
				S[s].surf[x] = TFT_HEIGHT;
			}
		} else if (P.z > S[s].z2) {
			S[s].surface = 2;
			cycles = 2;
			for (x = 0; x < TFT_WIDTH; x++) {
				S[s].surf[x] = 0;
			}
		} else {
			S[s].surface = 0;
			cycles = 1;
		}
		for (frontBack = 0; frontBack < cycles; frontBack++) {
			for (w = S[s].ws; w < S[s].we; w++) {
				//offset
				int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
				int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;

				//swap
				if (frontBack == 1) {
					int swp = x1;
					x1 = x2;
					x2 = swp;
					swp = y1;
					y1 = y2;
					y2 = swp;
				}

				wx[0] = x1 * CS - y1 * SN;
				wx[1] = x2 * CS - y2 * SN;
				wx[2] = wx[0];
				wx[3] = wx[1];

				wy[0] = y1 * CS + x1 * SN;
				wy[1] = y2 * CS + x2 * SN;
				wy[2] = wy[0];
				wy[3] = wy[1];
				S[s].d += dist(0, 0, (wx[0] + wx[1]) / 2, (wy[0] + wy[1]) / 2);

				wz[0] = S[s].z1 - P.z + ((P.l * wy[0]) / 32.0);
				wz[1] = S[s].z1 - P.z + ((P.l * wy[1]) / 32.0);
				wz[2] = S[s].z2 - P.z + ((P.l * wy[0]) / 32.0);
				wz[3] = S[s].z2 - P.z + ((P.l * wy[1]) / 32.0);

				if (wy[0] < 1 && wy[1] < 1) {
					continue;
				}

				if (wy[0] < 1) {
					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1],
							 wz[1]); //bottom line
					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3],
							 wz[3]); //top line
				}

				if (wy[1] < 1) {
					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0],
							 wz[0]); //bottom line
					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2],
							 wz[2]); //top line
				}

				wx[0] = wx[0] * 200 / wy[0] + TFT_WIDTH2;
				wy[0] = wz[0] * 200 / wy[0] + TFT_HEIGHT2;
				wx[1] = wx[1] * 200 / wy[1] + TFT_WIDTH2;
				wy[1] = wz[1] * 200 / wy[1] + TFT_HEIGHT2;

				wx[2] = wx[2] * 200 / wy[2] + TFT_WIDTH2;
				wy[2] = wz[2] * 200 / wy[2] + TFT_HEIGHT2;
				wx[3] = wx[3] * 200 / wy[3] + TFT_WIDTH2;
				wy[3] = wz[3] * 200 / wy[3] + TFT_HEIGHT2;

				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], s, w, frontBack);
			}
			int numWallsInSector = S[s].we - S[s].ws;
		}
	}
}
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);
	draw_3d();
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
