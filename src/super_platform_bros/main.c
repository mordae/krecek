#include <sdk.h>
#include <stdio.h>

#include <Petr_16x32.png.h>
#include <TileSetGame_16x16.png.h>

#define TILE_SIZE 16
#define MAP_SIZE 16

#define GREEN rgb_to_rgb565(0, 255, 0)
#define BROWN rgb_to_rgb565(165, 42, 42)

#define GRAVITY 3
#define MAX_V_SPEED 0.0001f

typedef struct {
	float speed;
	float max_speed;
	float vx, vy;
	sdk_sprite_t s;
} Petr;

typedef struct {
	float x, y;
} Camera;

typedef enum {
	EMPTY = 0,
	GRASS = 1,
	DIRT,
	GRASS_L_C,
	GRASS_P_C,
	GRASS_L,
	GRASS_P,
} TileType_world;

static Petr Pe;
static Camera C;

static void map_tile();

bool is_solid_ground(float x, float y, TileType_world map[MAP_SIZE][MAP_SIZE]);

TileType_world game_map[MAP_SIZE][MAP_SIZE] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 },
	{ 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
	{ 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
};

void game_start(void)
{
	Pe.s.x = 0;
	Pe.s.y = 177;

	Pe.speed = 30.5f;
	Pe.max_speed = 2.5f;

	C.y = 136;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (Pe.vx > Pe.max_speed) {
		Pe.vx = Pe.max_speed;
	}
	if (Pe.vx < -Pe.max_speed) {
		Pe.vx = -Pe.max_speed;
	}

	if (sdk_inputs.joy_x > 500) {
		Pe.vx += Pe.speed * dt;
	} else if (sdk_inputs.joy_x < -500) {
		Pe.vx += -Pe.speed * dt;
	} else {
		Pe.vx = 0;
	}
	Pe.vy += GRAVITY * dt;

	if (Pe.vy >= MAX_V_SPEED)
		Pe.vy = MAX_V_SPEED;

	Pe.s.x += Pe.vx;

	Pe.s.y += Pe.vy;

	C.x = Pe.s.x + -TFT_WIDTH * 0.5f;
	C.y = Pe.s.y + -TFT_HEIGHT * 0.5f;

	if (is_solid_ground(Pe.s.x, Pe.s.y, game_map)) {
		int Tile_Y = round(Pe.s.y) / TILE_SIZE + 1;
		Pe.s.y = Tile_Y * TILE_SIZE;
	}
}

bool is_solid_ground(float x, float y, TileType_world map[MAP_SIZE][MAP_SIZE])
{
	int Tile_X = round(x) / TILE_SIZE;
	int Tile_Y = round(y) / TILE_SIZE - 1;
	printf("Tile_x = %i\n", Tile_X);
	printf("Tile_y = %i\n", Tile_Y);

	switch (map[Tile_X][Tile_Y]) {
	case EMPTY:
	case GRASS:
	case DIRT:
	case GRASS_L_C:
	case GRASS_P_C:
	case GRASS_L:
	case GRASS_P:
		return true;
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(rgb_to_rgb565(173, 216, 230)); //reset screen every frame to 0-black

	tft_set_origin(C.x, C.y);

	map_tile();

	Pe.s.ts = &ts_petr_16x32_png;

	sdk_draw_sprite(&Pe.s);
}
static void map_tile()
{
	for (int y = 0; y < MAP_SIZE; y++) {
		for (int x = 0; x < MAP_SIZE; x++) {
			if (game_map[y][x]) {
				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE,
					      &ts_tilesetgame_16x16_png, game_map[y][x] - 1);
			}
		}
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
