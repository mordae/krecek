#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>

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

#define numSect 4
#define numWall 16

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
	int ws, we;
	int z1, z2;
	int d;
} sectors;
typedef struct {
	int x1, x2;
	int y1, y2;
	uint16_t c;
} walls;

static walls W[30];
static sectors S[30];
static player P;
static math M;
static int loadSectors[] = {
	//wall start,end,z1 H,z12 H
	0,  4,	0, 40, //sector 1
	4,  8,	0, 40, //sector 2
	8,  12, 0, 40, //sector 3
	12, 16, 0, 40, //sector 4
};
static int loadWalls[] = {
	//x1,y1, x2,y2, color
	0,  0,	32, 0,	rgb_to_rgb565(255, 255, 0), 32, 0,  32, 32, rgb_to_rgb565(160, 160, 0),
	32, 32, 0,  32, rgb_to_rgb565(255, 255, 0), 0,	32, 0,	0,  rgb_to_rgb565(160, 160, 0),

	64, 0,	96, 0,	rgb_to_rgb565(255, 247, 0), 96, 0,  96, 32, rgb_to_rgb565(128, 124, 0),
	96, 32, 64, 32, rgb_to_rgb565(255, 247, 0), 64, 32, 64, 0,  rgb_to_rgb565(128, 124, 0),

	64, 64, 96, 64, rgb_to_rgb565(255, 223, 0), 96, 64, 96, 96, rgb_to_rgb565(128, 112, 0),
	96, 96, 64, 96, rgb_to_rgb565(255, 223, 0), 64, 96, 64, 64, rgb_to_rgb565(128, 112, 0),

	0,  64, 32, 64, rgb_to_rgb565(204, 153, 0), 32, 64, 32, 96, rgb_to_rgb565(102, 77, 0),
	32, 96, 0,  96, rgb_to_rgb565(204, 153, 0), 0,	96, 0,	64, rgb_to_rgb565(102, 77, 0),
};
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
	//load sectors
	int s, w, v1 = 0, v2 = 0;
	for (s = 0; s < numSect; s++) {
		S[s].ws = loadSectors[v1 + 0];
		S[s].we = loadSectors[v1 + 1];
		S[s].z1 = loadSectors[v1 + 2];
		S[s].z2 = loadSectors[v1 + 3] - loadSectors[v1 + 2];
		v1 += 4;
		for (w = S[s].ws; w < S[s].we; w++) {
			W[w].x1 = loadWalls[v2 + 0];
			W[w].y1 = loadWalls[v2 + 1];
			W[w].x2 = loadWalls[v2 + 2];
			W[w].y2 = loadWalls[v2 + 3];
			W[w].c = loadWalls[v2 + 4];
			v2 += 5;
		}
	}
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

void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2) //clip line
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

static void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, uint16_t c)
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
	if (x1 < 1) {
		x1 = 0;
	}
	if (x2 < 1) {
		x2 = 0;
	}
	if (x1 > TFT_WIDTH - 1) {
		x1 = TFT_WIDTH - 0;
	}
	if (x2 > TFT_WIDTH - 1) {
		x2 = TFT_WIDTH - 0;
	}

	for (x = x1; x < x2; x++) {
		int y1 = dyb * (x - xs + 0.5) / dx + b1;
		int y2 = dyt * (x - xs + 0.5) / dx + t1;

		//CLIP Y
		if (y1 < 1) {
			y1 = 0;
		}
		if (y2 < 1) {
			y2 = 0;
		}
		if (y1 > TFT_HEIGHT - 1) {
			y1 = TFT_HEIGHT - 0;
		}
		if (y2 > TFT_HEIGHT - 1) {
			y2 = TFT_HEIGHT - 0;
		}

		for (y = y1; y < y2; y++) {
			tft_draw_pixel(x, y, c);
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
	int s, w, loop, wx[4], wy[4], wz[4];
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
		for (loop = 0; loop < 2; loop++) {
			for (w = S[s].ws; w < S[s].we; w++) {
				//offset
				int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
				int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;

				//swap
				if (loop == 0) {
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
				wz[2] = wz[0] + S[s].z2;
				wz[3] = wz[1] + S[s].z2;

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

				wx[0] = wx[0] * 150 / wy[0] + TFT_WIDTH2;
				wy[0] = wz[0] * 150 / wy[0] + TFT_HEIGHT2;
				wx[1] = wx[1] * 150 / wy[1] + TFT_WIDTH2;
				wy[1] = wz[1] * 150 / wy[1] + TFT_HEIGHT2;

				wx[2] = wx[2] * 150 / wy[2] + TFT_WIDTH2;
				wy[2] = wz[2] * 150 / wy[2] + TFT_HEIGHT2;
				wx[3] = wx[3] * 150 / wy[3] + TFT_WIDTH2;
				wy[3] = wz[3] * 150 / wy[3] + TFT_HEIGHT2;

				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], W[w].c);
			}
			int numWallsInSector = S[s].we - S[s].ws;
			if (numWallsInSector > 0) {
				S[s].d /= numWallsInSector;
			}
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
