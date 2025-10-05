#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//*     USE MAUSE in everthing *
//*  MAP_EDITOR
//*
//*   STATUS 0
//*     right click to look at texture
//*   STATUS 1
//*     use right click to tile +1
//*   STATUS 2
//*     use right click to set the tile from memory
//*     use start/enter to set the memory tile
//*   STATUS 3
//*     set tile to 0
//*   Use select to use cheet sheet
//*
//*   To edit what map and where use it edit struct edited_file
#include <arrow.png.h>
#include <font-5x5.png.h>
#include <ui_editor-40x120.png.h>
#include <T_00-4x4.png.h>
#include <T_01-4x4.png.h>
#include <T_02-4x4.png.h>
#include <T_03-4x4.png.h>
#include <T_04-4x4.png.h>
#include <T_05-4x4.png.h>
#include <T_06-4x4.png.h>
#include <T_07-4x4.png.h>
#include <T_08-4x4.png.h>
#include <T_09-4x4.png.h>
#include <T_10-4x4.png.h>
#include <T_11-4x4.png.h>
#include <T_12-4x4.png.h>
#include <T_13-4x4.png.h>
#include <T_14-4x4.png.h>
#include <T_15-4x4.png.h>
#include <T_16-4x4.png.h>
#include <T_17-4x4.png.h>
#include <T_18-4x4.png.h>
#include <T_19-4x4.png.h>

#include "textures/textures/T_00.h"
#include "textures/textures/T_01.h"
#include "textures/textures/T_02.h"
#include "textures/textures/T_03.h"
#include "textures/textures/T_04.h"
#include "textures/textures/T_05.h"
#include "textures/textures/T_06.h"
#include "textures/textures/T_07.h"
#include "textures/textures/T_08.h"
#include "textures/textures/T_09.h"
#include "textures/textures/T_10.h"
#include "textures/textures/T_11.h"
#include "textures/textures/T_12.h"
#include "textures/textures/T_13.h"
#include "textures/textures/T_14.h"
#include "textures/textures/T_15.h"
#include "textures/textures/T_16.h"
#include "textures/textures/T_17.h"
#include "textures/textures/T_18.h"
#include "textures/textures/T_19.h"

#include "../krecdoom/maps.h"

#define TILE_SIZE 4

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define RED rgb_to_rgb565(255, 0, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define GRAY rgb_to_rgb565(128, 128, 128)
#define DARK_GRAY rgb_to_rgb565(64, 64, 64)

#define BORDER_COLOR WHITE
static void textures_show();

const char *texture_names[] = {
	T_00, T_01, T_02, T_03, T_04, T_05, T_06, T_07, T_08, T_09,
	T_10, T_11, T_12, T_13, T_14, T_15, T_16, T_17, T_18,
};

const int texture_heights[] = {
	T_00_HEIGHT, T_01_HEIGHT, T_02_HEIGHT, T_03_HEIGHT, T_04_HEIGHT, T_05_HEIGHT, T_06_HEIGHT,
	T_07_HEIGHT, T_08_HEIGHT, T_09_HEIGHT, T_10_HEIGHT, T_11_HEIGHT, T_12_HEIGHT, T_13_HEIGHT,
	T_14_HEIGHT, T_15_HEIGHT, T_16_HEIGHT, T_17_HEIGHT, T_18_HEIGHT,
};

const int texture_widths[] = {
	T_00_WIDTH, T_01_WIDTH, T_02_WIDTH, T_03_WIDTH, T_04_WIDTH, T_05_WIDTH, T_06_WIDTH,
	T_07_WIDTH, T_08_WIDTH, T_09_WIDTH, T_10_WIDTH, T_11_WIDTH, T_12_WIDTH, T_13_WIDTH,
	T_14_WIDTH, T_15_WIDTH, T_16_WIDTH, T_17_WIDTH, T_18_WIDTH,
};

typedef enum {
	EDITOR_MAP,
	EDITOR_TILE,
	EDITOR_MENU,
	EDITOR_FILE_SELECT,
	EDITOR_SAVE,
	EDITOR_RESET,
	EDITOR_INFO
} EDITOR_STATE;
typedef enum { FIRST, SECOND, THIRD, NONE } EDITOR_STATUS;

typedef struct {
	sdk_sprite_t a;
	sdk_sprite_t ui;
} sprites;

typedef struct {
	float timer;
} Saves;

typedef struct {
	bool tiles;
	int tile_sel;
} Menu;

typedef struct {
	char *name;
	char *file_path;
} Edited_File;

typedef struct {
	float total_time;
	float del;
	float auto_timer;
	float timer;
} Save;
typedef struct {
	bool yes;
	int x, y;
} TM;
typedef struct {
	int w, h;		   //texture width/height
	const unsigned char *name; //texture name
} TexureMaps;

static Edited_File edited_file;
static sprites S;
static TM tm;
static Save save;
static Menu M;
static EDITOR_STATE editor_state = EDITOR_MENU;
static EDITOR_STATUS editor_status = NONE;

static TexureMaps Textures[64];

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
extern const TileType maps_map2[MAP_ROWS][MAP_COLS];
const TileType (*load_map)[MAP_COLS] = maps_map1;

static TileType map[MAP_ROWS][MAP_COLS];

static void tile_ui(int x, int y);
static void tiles();
static void ui_editor();
static void ui_input_editor(int mx, int my);
static void draw_small_string(int x, int y, const char *format, ...);

static void editor_read_save()
{
	Saves saves;
	sdk_save_read(0, &saves, sizeof(saves));
	if (isnan(saves.timer)) {
		saves.timer = 0;
		save.total_time = 0;
	} else {
		save.total_time = saves.timer;
	}
}

static void editor_auto_save()
{
	Saves saves = { .timer = save.total_time };
	sdk_save_write(0, &saves, sizeof(saves));
}
static void saveMap(const char *filepath, const char *mapName, TileType map[MAP_ROWS][MAP_COLS])
{
	// Create directories if they don't exist (simple approach)
	char dir_path[256];
	strcpy(dir_path, filepath);

	char *last_slash = strrchr(dir_path, '/');
	if (last_slash != NULL) {
		*last_slash = '\0'; // Terminate at the last slash to get directory path

		char command[512];
		snprintf(command, sizeof(command), "mkdir -p %s", dir_path);
		system(command);
	}

	FILE *file = fopen(filepath, "w");
	if (!file) {
		perror("Failed to open file");
		return;
	}

	// Write the include directive and array declaration for a .c file
	fprintf(file, "#include \"../maps.h\"\n\n");
	fprintf(file, "TileType %s[MAP_ROWS][MAP_COLS] = {\n", mapName);

	// Write each row of the map
	for (int i = 0; i < MAP_ROWS; i++) {
		fprintf(file, "\t{ ");
		for (int j = 0; j < MAP_COLS; j++) {
			fprintf(file, "%d", map[i][j]);
			if (j < MAP_COLS - 1) {
				fprintf(file, ", ");
			}
		}
		fprintf(file, " }");
		if (i < MAP_ROWS - 1) {
			fprintf(file, ",");
		}
		fprintf(file, "\n");
	}

	fprintf(file, "};\n");
	fclose(file);
	printf("Map saved successfully to %s\n", filepath);
}

static void draw_texture(int x, int y, int t)
{
	tft_draw_rect(x - 1, y - 1, x + Textures[t].w, y + Textures[t].h, WHITE);
	int xt, yt;
	for (yt = 0; yt < Textures[t].h; yt++) {
		for (xt = 0; xt < Textures[t].w; xt++) {
			int pixel = yt * 3 * Textures[t].w + xt * 3;
			int r = Textures[t].name[pixel + 0];
			int g = Textures[t].name[pixel + 1];
			int b = Textures[t].name[pixel + 2];
			tft_draw_pixel(x + xt, y + yt, rgb_to_rgb565(r, g, b));
		}
	}
}
static void textures_load()
{
	for (int i = 0; i < 19; i++) {
		Textures[i].name = texture_names[i];
		Textures[i].h = texture_heights[i];
		Textures[i].w = texture_widths[i];
	}
}

void game_start(void)
{
	textures_load();
	editor_read_save();
	edited_file.name = "maps_map1";
	edited_file.file_path = "../krecdoom/maps/map1.c";

	tm.yes = 0;

	S.a.x = 35;
	S.a.y = 42;
	S.a.ts = &ts_arrow_png;
	S.ui.x = 0;
	S.ui.y = 0;
	S.ui.ts = &ts_ui_editor_40x120_png;

	editor_state = EDITOR_MENU;
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	save.total_time += dt;
	save.auto_timer += dt;

	if (save.auto_timer > 5) {
		save.auto_timer = 0;
		editor_auto_save();
	}

	int mx = (int)round(sdk_inputs.tx * TFT_WIDTH);
	int my = (int)round(sdk_inputs.ty * TFT_HEIGHT);
	switch (editor_state) {
	case EDITOR_RESET:
		save.del += dt;
		if (save.del > 3) {
			for (int row = 0; row < MAP_ROWS; row++) {
				for (int col = 0; col < MAP_COLS; col++) {
					map[row][col] = 0;
				}
			}
			editor_state = EDITOR_MAP;
		}
		break;
	case EDITOR_MENU:
		if (my >= 22 && my <= 54) {
			S.a.x = 35;
			S.a.y = 42;
			if (sdk_inputs.tp > 0.5 || sdk_inputs_delta.start == 1) {
				editor_state = EDITOR_FILE_SELECT;
			}
		}
		if (my >= 55 && my <= 69) {
			S.a.x = 35;
			S.a.y = 57;
			if (sdk_inputs.tp > 0.5 || sdk_inputs_delta.start == 1) {
				editor_state = EDITOR_TILE;
			}
		}
		if (my >= 70 && my <= 84) {
			S.a.x = 35;
			S.a.y = 72;
			if (sdk_inputs.tp > 0.5 || sdk_inputs_delta.start == 1) {
				editor_state = EDITOR_INFO;
			}
		}
		if (my >= 84 && my <= 100) {
			S.a.x = 35;
			S.a.y = 87;
			if (sdk_inputs.tp > 0.5 || sdk_inputs_delta.start == 1) {
				editor_state = EDITOR_RESET;
				save.del = 0;
			}
		}
		break;

	case EDITOR_FILE_SELECT:
		if (sdk_inputs.aux[7]) {
			editor_state = EDITOR_MENU;
		}
		for (int r = 0; r < MAP_ROWS; r++) {
			for (int c = 0; c < MAP_COLS; c++) {
				map[r][c] = load_map[r][c];
			}
		}
		editor_state = EDITOR_MAP;
		break;

	case EDITOR_MAP:
		ui_input_editor(mx, my);
		if (sdk_inputs.aux[7]) {
			editor_state = EDITOR_MENU;
		}
		if (mx >= 40) {
			mx -= 40;
			for (int y = 0; y < MAP_COLS; y++) {
				if (round(my / TILE_SIZE == y)) {
					for (int x = 0; x < MAP_COLS; x++) {
						if (round(mx / TILE_SIZE) == x) {
							switch (editor_status) {
							case FIRST:
								if (sdk_inputs_delta.tp == 1) {
									map[x][y] += 1;
									if (map[x][y] >
									    ENUM_TYPES) {
										map[x][y] = 0;
									}
								}
								break;
							case SECOND:
								if (sdk_inputs_delta.tp == 1) {
									map[x][y] = M.tile_sel;
								}
								if (sdk_inputs_delta.start == 1) {
									M.tile_sel = map[x][y];
								}
								break;
							case NONE:
								if (sdk_inputs_delta.tp == 1) {
									tm.x = x;
									tm.y = y;
									if (tm.yes == 1) {
										tm.yes = 0;
									} else {
										tm.yes = 1;
									}
								}
								break;
							case THIRD:
								if (sdk_inputs_delta.tp == 1) {
									map[x][y] = 0;
								}
								break;
							}
						}
					}
				}
			}
			mx += 40;
		}
		if (mx >= 0 && mx <= 39 && my >= 100 && sdk_inputs_delta.tp == 1) {
			editor_state = EDITOR_SAVE;
			save.timer = 0;
		}
		break;

	case EDITOR_TILE:
		if (sdk_inputs.aux[7]) {
			editor_state = EDITOR_MENU;
		}
		break;
	case EDITOR_INFO:
		if (sdk_inputs.aux[7]) {
			editor_state = EDITOR_MENU;
		}
		break;
	case EDITOR_SAVE:
		if (save.timer < 3) {
			save.timer += dt;
			return;
		} else {
			saveMap("edited_file.path", "edited_file.name", map);
			editor_state = EDITOR_MENU;
		}
		break;
	}
}
void draw_loading_bar(float level)
{
	if (level < 0)
		level = 0;
	if (level > 3)
		level = 3;

	int filled_segments = level * 4;

	int bar_x0 = 20;
	int bar_y0 = 50;
	int bar_width = 120;
	int bar_height = 20;

	int segment_width = bar_width / 12;

	tft_fill(0);
	tft_draw_rect(bar_x0, bar_y0, bar_x0 + bar_width, bar_y0 + 1, WHITE); // top
	tft_draw_rect(bar_x0, bar_y0 + bar_height - 1, bar_x0 + bar_width, bar_y0 + bar_height,
		      WHITE);						       // bottom
	tft_draw_rect(bar_x0, bar_y0, bar_x0 + 1, bar_y0 + bar_height, WHITE); // left
	tft_draw_rect(bar_x0 + bar_width - 1, bar_y0, bar_x0 + bar_width, bar_y0 + bar_height,
		      WHITE); // right

	for (int i = 0; i < 12; i++) {
		int x0 = bar_x0 + i * segment_width + 1;
		int x1 = bar_x0 + (i + 1) * segment_width - 1;
		tft_draw_rect(x0, bar_y0 + 2, x1, bar_y0 + bar_height - 2,
			      (i < filled_segments) ? GREEN : 0);
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);
	switch (editor_state) {
	case EDITOR_MENU:
		tft_draw_string(40, 10, WHITE, "Krecdoom");
		tft_draw_string(60, 22, WHITE, "editor");
		tft_draw_string(40, 40, WHITE, "Map select");
		tft_draw_string(40, 55, WHITE, "Tile editor");
		tft_draw_string(40, 70, WHITE, "Info");
		tft_draw_string(40, 85, WHITE, "Reset Map");
		sdk_draw_sprite(&S.a);
		break;

	case EDITOR_FILE_SELECT:
		break;
	case EDITOR_RESET:
		draw_loading_bar(save.del);
		break;
	case EDITOR_INFO:
		tft_draw_string(5, 10, WHITE, "Use Aux1-4 to");
		tft_draw_string(5, 21, WHITE, "switch states");

		tft_draw_string(5, 43, WHITE, "Total spend %-.2f h", save.total_time / 60 / 60);
		tft_draw_string(5, 32, WHITE, "Use Aux7 to return");
		tft_draw_string(5, 65, WHITE, "Select for");
		tft_draw_string(5, 76, WHITE, "Cheat sheet");

		break;

	case EDITOR_SAVE:
		draw_loading_bar(save.timer);
		break;

	case EDITOR_MAP:
		tiles();
		ui_editor();
		break;

	case EDITOR_TILE:
		tile_ui(5, 5);
		break;
	}
}
static void ui_input_editor(int mx, int my)
{
	if (sdk_inputs_delta.select == 1) {
		if (M.tiles) {
			M.tiles = false;
		} else {
			M.tiles = true;
		}
	}
	if (M.tiles) {
		if (mx >= 50 && mx <= 130 && my >= 20 && my <= 100) {
			if (mx >= 52 && mx <= 69) {
				if (my >= 22 && my <= 29) {
					M.tile_sel = 1;
				}
			}
		}
	}
	if (sdk_inputs_delta.aux[0] == 1 || sdk_inputs_delta.aux[4]) {
		editor_status = NONE;
	}
	if (sdk_inputs_delta.aux[1] == 1) {
		editor_status = FIRST;
	}
	if (sdk_inputs_delta.aux[2] == 1) {
		editor_status = SECOND;
	}
	if (sdk_inputs_delta.aux[3] == 1) {
		editor_status = THIRD;
	}
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
static void ui_editor()
{
	if (M.tiles) {
		tile_ui(50, 20);
	}
	sdk_draw_sprite(&S.ui);
	if (tm.yes == 1) {
		textures_show();
	}

	int num_status;
	switch (editor_status) {
	case NONE:
		num_status = 0;
		break;
	case FIRST:
		num_status = 1;
		break;
	case SECOND:
		num_status = 2;
		break;

	case THIRD:
		num_status = 3;
		break;
	}
	draw_small_string(1, 2, "%i", num_status);
}
static void textures_show()
{
	switch (map[tm.x][tm.y]) {
	case COBLE:
		draw_texture(2, 22, 0);
		break;
	case BUNKER:
		draw_texture(2, 22, 1);
		break;
	case BRICKS:
		draw_texture(2, 22, 2);
		break;
	case STONE:
		draw_texture(2, 22, 4);
		break;
	case MAGMA:
		draw_texture(2, 22, 5);
		break;
	case WATER:
		draw_texture(2, 22, 6);
		break;
	case STONE_HOLE:
		draw_texture(2, 22, 7);
		break;
	case CHEKERD:
		draw_texture(2, 22, 8);
		break;
	case MOSS_STONE:
		draw_texture(2, 22, 12);
		break;
	case WOOD_WALL:
		draw_texture(2, 22, 13);
		break;
	case PRISMARYN:
		draw_texture(2, 22, 14);
		break;
	case CHOLOTATE:
		draw_texture(2, 22, 15);
		break;
	case PATTERN_IN_BROWN:
		draw_texture(2, 22, 17);
		break;
	case SAND_WITH_MOSS:
		draw_texture(2, 22, 18);
		break;
	case IRON:
		draw_texture(2, 22, 19);
		break;
	case TELEPORT:
		draw_small_string(2, 22, "teleport");
		break;
	case UN1:
	case UN2:
	case UN3:
	case UN4:
	case UN5:
	case EMPTY:
		break;
	}
}
static void tile_ui(int x, int y)
{
	tft_draw_rect(x, y, x + 80, y + 80, 0);
	tft_draw_rect(x, y, x, y + 80, BORDER_COLOR);
	tft_draw_rect(x + 80, y + 80, x, y + 80, BORDER_COLOR);
	tft_draw_rect(x + 80, y, x + 80, y + 80, BORDER_COLOR);
	tft_draw_rect(x, y, x + 80, y, BORDER_COLOR);

	sdk_draw_tile(x + 2, y + 2, &ts_t_00_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6, &ts_t_00_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6, &ts_t_00_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2, &ts_t_00_4x4_png, 0);
	draw_small_string(x + 11, y + 5, "coble");

	sdk_draw_tile(x + 2, y + 2 + TILE_SIZE * 2, &ts_t_01_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + TILE_SIZE * 2, &ts_t_01_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + TILE_SIZE * 2, &ts_t_01_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + TILE_SIZE * 2, &ts_t_01_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + TILE_SIZE * 2, "bunker");

	sdk_draw_tile(x + 2, y + 2 + 2 * TILE_SIZE * 3, &ts_t_02_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + 2 * TILE_SIZE * 3, &ts_t_02_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + 2 * TILE_SIZE * 3, &ts_t_02_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + 2 * TILE_SIZE * 3, &ts_t_02_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 3, "bricks");

	sdk_draw_tile(x + 2, y + 2 + 2 * TILE_SIZE * 4, &ts_t_04_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + 2 * TILE_SIZE * 4, &ts_t_04_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + 2 * TILE_SIZE * 4, &ts_t_04_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + 2 * TILE_SIZE * 4, &ts_t_04_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 4, "stone");

	sdk_draw_tile(x + 2, y + 2 + 2 * TILE_SIZE * 5, &ts_t_05_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + 2 * TILE_SIZE * 5, &ts_t_05_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + 2 * TILE_SIZE * 5, &ts_t_05_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + 2 * TILE_SIZE * 5, &ts_t_05_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 5, "magma");

	sdk_draw_tile(x + 2, y + 2 + 2 * TILE_SIZE * 6, &ts_t_06_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + 2 * TILE_SIZE * 6, &ts_t_06_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + 2 * TILE_SIZE * 6, &ts_t_06_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + 2 * TILE_SIZE * 6, &ts_t_06_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 6, "teleport");

	sdk_draw_tile(x + 2, y + 2 + 2 * TILE_SIZE * 7, &ts_t_07_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 6 + 2 * TILE_SIZE * 7, &ts_t_07_4x4_png, 0);
	sdk_draw_tile(x + 2, y + 6 + 2 * TILE_SIZE * 7, &ts_t_07_4x4_png, 0);
	sdk_draw_tile(x + 6, y + 2 + 2 * TILE_SIZE * 7, &ts_t_07_4x4_png, 0);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 7, "stone_hole");

	tft_draw_rect(x + 2, y + 2 + 2 * TILE_SIZE * 8, x + 9, y + 9 + 2 * TILE_SIZE * 8, GREEN);
	draw_small_string(x + 11, y + 3 + 2 * TILE_SIZE * 8, "p spawn");
}
static void draw_tile(int x0, int y0, int x1, int y1, int tx, int ty)
{
	switch (map[tx][ty]) {
	case COBLE:
		sdk_draw_tile(x0, y0, &ts_t_00_4x4_png, 0);
		break;
	case BUNKER:
		sdk_draw_tile(x0, y0, &ts_t_01_4x4_png, 0);
		break;
	case TELEPORT:
		sdk_draw_tile(x0, y0, &ts_t_06_4x4_png, 0);
		break;
	case BRICKS:
		sdk_draw_tile(x0, y0, &ts_t_02_4x4_png, 0);
		break;
	case STONE:
		sdk_draw_tile(x0, y0, &ts_t_04_4x4_png, 0);
		break;
	case MAGMA:
		sdk_draw_tile(x0, y0, &ts_t_05_4x4_png, 0);
		break;
	case WATER:
		sdk_draw_tile(x0, y0, &ts_t_06_4x4_png, 0);
		break;
	case STONE_HOLE:
		sdk_draw_tile(x0, y0, &ts_t_07_4x4_png, 0);
		break;
	case CHEKERD:
		sdk_draw_tile(x0, y0, &ts_t_08_4x4_png, 0);
		break;
	case MOSS_STONE:
		sdk_draw_tile(x0, y0, &ts_t_12_4x4_png, 0);
		break;
	case WOOD_WALL:
		sdk_draw_tile(x0, y0, &ts_t_13_4x4_png, 0);
		break;
	case PRISMARYN:
		sdk_draw_tile(x0, y0, &ts_t_14_4x4_png, 0);
		break;
	case CHOLOTATE:
		sdk_draw_tile(x0, y0, &ts_t_15_4x4_png, 0);
		break;
	case PATTERN_IN_BROWN:
		sdk_draw_tile(x0, y0, &ts_t_17_4x4_png, 0);
		break;
	case SAND_WITH_MOSS:
		sdk_draw_tile(x0, y0, &ts_t_18_4x4_png, 0);
		break;
	case IRON:
		sdk_draw_tile(x0, y0, &ts_t_19_4x4_png, 0);
		break;
	case UN1:
	case UN2:
	case UN3:
	case UN4:
	case UN5:
	case EMPTY:
		break;
	}
}
static void tiles()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (x == 1 && y == 1) {
				tft_draw_rect(x * TILE_SIZE + 40, y * TILE_SIZE,
					      x * TILE_SIZE + 3 + 40, y * TILE_SIZE + 3, GREEN);
			} else {
				draw_tile(x * TILE_SIZE + 40, y * TILE_SIZE,
					  x * TILE_SIZE + TILE_SIZE + 39,
					  y * TILE_SIZE + TILE_SIZE - 1, x, y);
			}
		}
	}
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
