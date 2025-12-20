#pragma once
#include <tft.h>

#define LOOKING_POWER 80.f
#define WALKING_POWER 140.f
#define FOV 200

#define BLUE rgb_to_rgb565(0, 0, 255)
#define YELLOW rgb_to_rgb565(160, 160, 0)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define TFT_WIDTH2 TFT_WIDTH / 2
#define TFT_HEIGHT2 TFT_HEIGHT / 2

typedef struct {
	int Text;
	int Sect;
	int Wall;
} NUM;
typedef struct {
	int w, h;		   //texture width/height
	const unsigned char *name; //texture name
} TexureMaps;

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
	float cos[360];
	float sin[360];
} math;

typedef struct {
	int x, y, z;
	int a;
	int l;
} player;

extern player P;
extern math M;
extern NUM Num;
extern walls W[256];
extern sectors S[128];
extern TexureMaps Textures[64];
