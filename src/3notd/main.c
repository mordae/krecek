#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>

#include "pixel.h"

//    TODO    //
//    Menu    //
//    editor  //

//--------------------------------------
//             3NOTD
// includes DUKE NUKE 3D Grafics
//
//--------------------------------------
#define LOOKING_POWER 80.f
#define WALKING_POWER 140.f
#define FOV 200

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
static void textures_load();

static void load()
{
	FILE *fp = fopen("level.h", "r");
	if (fp == NULL) {
		printf("OH NOO AN ERROR level.h");
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
// unsed
static void testTextures(int t)
{
	int x, y;
	t = 0;
	for (y = 0; y < Textures[t].h; y++) {
		for (x = 0; x < Textures[t].w; x++) {
			int pixel = y * 3 * Textures[t].w + x * 3;
			int r = Textures[t].name[pixel + 0];
			int g = Textures[t].name[pixel + 1];
			int b = Textures[t].name[pixel + 2];
			drawpixel(x, y, r, g, b);
		}
	}
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
static void floors()
{
	int x, y;
	int xo = TFT_WIDTH2;
	int yo = TFT_HEIGHT2;
	float lookUpDown = -P.l * 4;
	//if (lookUpDown > TFT_HEIGHT) {
	//	lookUpDown = TFT_HEIGHT;
	//}
	//printf("Looking %f\n" , lookUpDown);
	float moveUpDown = P.z / 16.f;
	if (moveUpDown == 0) {
		moveUpDown = 0.001;
	}

	int ys = yo, ye = -lookUpDown;

	if (moveUpDown < 0) {
		ys = -lookUpDown;
		ye = yo + lookUpDown;
	}

	//for (y = -lookUpDown; y < yo; y++)

	for (y = ye; y < ys; y++) {
		for (x = -xo; x < xo; x++) {
			float z = y + lookUpDown;
			if (z == 0) {
				z = 0.0001;
			}
			float fx = x / z * moveUpDown;
			float fy = FOV / z * moveUpDown;

			float ry = fx * M.sin[P.a] - fy * M.cos[P.a] + (-P.y / 30.0);
			float rx = fx * M.cos[P.a] + fy * M.sin[P.a] + (P.x / 30.0);

			if (rx < 0) {
				rx = -rx + 1;
			}
			if (ry < 0) {
				ry = -ry + 1;
			}

			if (rx < 0 || ry < 0 || rx > 5 || ry > 5) {
				continue;
			}
			if ((int)rx % 2 == (int)ry % 2) {
				drawpixel(x + xo, y + yo, 255, 0, 0);
			} else {
				drawpixel(x + xo, y + yo, 0, 255, 0);
			}
		}
	}
}
static void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack)
{
	// wall texture
	int wt = W[w].wt;

	float ht = 0, ht_step = (float)Textures[wt].w * W[w].u / (float)(x2 - x1);

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
		ht -= ht_step * x1;
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

	//draw x ver. line
	for (x = x1; x < x2; x++) {
		int y1 = dyb * (x - xs + 0.5) / dx + b1;
		int y2 = dyt * (x - xs + 0.5) / dx + t1;
		if ((y2 - y1) == 0) {
			continue; // Skip
		}

		float vt = 0, vt_step = (float)Textures[wt].h * W[w].v / (float)(y2 - y1);
		int clipped_y = 0;
		if (y1 < 0) {
			clipped_y = 0 - y1;
			vt += clipped_y * vt_step;
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
				int tex_x = (int)ht % Textures[wt].w;
				int tex_y = (int)vt % Textures[wt].h;
				int pixel = tex_y * 3 * Textures[wt].w + tex_x * 3;
				int r = Textures[wt].name[pixel + 0] - (W[w].shade / 4);
				if (r < 0) {
					r = 0;
				}
				int g = Textures[wt].name[pixel + 1] - (W[w].shade / 4);
				if (g < 0) {
					g = 0;
				}
				int b = Textures[wt].name[pixel + 2] - (W[w].shade / 4);
				if (b < 0) {
					b = 0;
				}
				drawpixel(x, y, r, g, b);
				vt += vt_step;
			}
			ht += ht_step;
		}
		if (frontBack == 1) {
			if (S[s].surface == 1) {
				y2 = S[s].surf[x];
			}
			if (S[s].surface == 2) {
				y1 = S[s].surf[x];
			}
			for (y = y1; y < y2; y++) {
				drawpixel(x, y, 255, 0, 0);
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

				wx[0] = wx[0] * FOV / wy[0] + TFT_WIDTH2;
				wy[0] = wz[0] * FOV / wy[0] + TFT_HEIGHT2;
				wx[1] = wx[1] * FOV / wy[1] + TFT_WIDTH2;
				wy[1] = wz[1] * FOV / wy[1] + TFT_HEIGHT2;

				wx[2] = wx[2] * FOV / wy[2] + TFT_WIDTH2;
				wy[2] = wz[2] * FOV / wy[2] + TFT_HEIGHT2;
				wx[3] = wx[3] * FOV / wy[3] + TFT_WIDTH2;
				wy[3] = wz[3] * FOV / wy[3] + TFT_HEIGHT2;

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
	//draw_3d();
	floors();
	//testTextures(4);
}
static void textures_load()
{
	Textures[0].name = T_00;
	Textures[0].h = T_00_HEIGHT;
	Textures[0].w = T_00_WIDTH;
	Textures[1].name = T_01;
	Textures[1].h = T_01_HEIGHT;
	Textures[1].w = T_01_WIDTH;
	Textures[2].name = T_02;
	Textures[2].h = T_02_HEIGHT;
	Textures[2].w = T_02_WIDTH;
	Textures[3].name = T_03;
	Textures[3].h = T_03_HEIGHT;
	Textures[3].w = T_03_WIDTH;
	Textures[4].name = T_04;
	Textures[4].h = T_04_HEIGHT;
	Textures[4].w = T_04_WIDTH;
	Textures[5].name = T_05;
	Textures[5].h = T_05_HEIGHT;
	Textures[5].w = T_05_WIDTH;
	Textures[6].name = T_06;
	Textures[6].h = T_06_HEIGHT;
	Textures[6].w = T_06_WIDTH;
	Textures[7].name = T_07;
	Textures[7].h = T_07_HEIGHT;
	Textures[7].w = T_07_WIDTH;
	Textures[8].name = T_08;
	Textures[8].h = T_08_HEIGHT;
	Textures[8].w = T_08_WIDTH;
	Textures[9].name = T_09;
	Textures[9].h = T_09_HEIGHT;
	Textures[9].w = T_09_WIDTH;
	Textures[10].name = T_10;
	Textures[10].h = T_10_HEIGHT;
	Textures[10].w = T_10_WIDTH;
	Textures[11].name = T_11;
	Textures[11].h = T_11_HEIGHT;
	Textures[11].w = T_11_WIDTH;
	Textures[12].name = T_12;
	Textures[12].h = T_12_HEIGHT;
	Textures[12].w = T_12_WIDTH;
	Textures[13].name = T_13;
	Textures[13].h = T_13_HEIGHT;
	Textures[13].w = T_13_WIDTH;
	Textures[14].name = T_14;
	Textures[14].h = T_14_HEIGHT;
	Textures[14].w = T_14_WIDTH;
	Textures[15].name = T_15;
	Textures[15].h = T_15_HEIGHT;
	Textures[15].w = T_15_WIDTH;
	Textures[16].name = T_16;
	Textures[16].h = T_16_HEIGHT;
	Textures[16].w = T_16_WIDTH;
	Textures[17].name = T_17;
	Textures[17].h = T_17_HEIGHT;
	Textures[17].w = T_17_WIDTH;
	Textures[18].name = T_18;
	Textures[18].h = T_18_HEIGHT;
	Textures[18].w = T_18_WIDTH;
	Textures[19].name = T_19;
	Textures[19].h = T_19_HEIGHT;
	Textures[19].w = T_19_WIDTH;
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
