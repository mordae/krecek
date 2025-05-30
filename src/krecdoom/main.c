#include "common.h"
#include "graphics.h"
#include "volume.h"
#include "maps.h"
#include "math_utils.h"
#include "colors.h"

#include <pico/stdlib.h> // Pico SDK standard library
#include <sdk.h>	 // Your game SDK functions
#include <stdio.h>	 // Standard I/O for debugging (e.g., printf)
#include <math.h>	 // For fmodf in player movement
#include <tft.h>

float volume = 0.5f;

void game_player_inputs(float dt);
void draw_sector(int sector_idx);
void draw_wall(int wall_idx, float sector_floor_h, float sector_ceil_h, uint16_t floor_color,
	       uint16_t ceil_color);
void draw_vertical_line(int x, int y1, int y2, uint16_t color);
Player g_player = { .x = 1.5f,
		    .y = 1.5f,
		    .z = 0.5f,
		    .angle = 0.0f,
		    .look = 0.0f,
		    .is_flying = false };

Wall g_walls[500];
int g_num_walls = 0;

Sector g_sectors[50];
int g_num_sectors = 0;

float g_cos_table[ANGLE_MAX];
float g_sin_table[ANGLE_MAX];

void game_start(void)
{
	sdk_set_output_gain_db(volume);

	init_math_tables();

	g_player.x = 0.0f;
	g_player.y = -40.0f;
	g_player.z = 20.0f;
	g_player.angle = 90.0f;
	g_player.look = 0.0f;

	init_game_map();
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_player_inputs(dt);
	game_handle_audio(dt, volume);
}

void game_paint(unsigned dt_usec)
{
	tft_fill(0);
	for (int i = 0; i < g_num_sectors; i++) {
		draw_sector(i);
	}

	tft_draw_string(0, 0, WHITE, "%-.2f", g_player.angle);
	tft_draw_string(0, 10, WHITE, "%-.2f", g_player.look);
}

void draw_sector(int sector_idx)
{
	Sector s = g_sectors[sector_idx];

	for (int i = 0; i < s.num_walls; i++) {
		draw_wall(s.start_wall_idx + i, s.floor_height, s.ceil_height, s.floor_color,
			  s.ceil_color);
	}
}

void draw_wall(int wall_idx, float sector_floor_h, float sector_ceil_h, uint16_t floor_color,
	       uint16_t ceil_color)
{
	Wall w = g_walls[wall_idx];

	Point2D p1_rel = { w.p1.x - g_player.x, w.p1.y - g_player.y };
	Point2D p2_rel = { w.p2.x - g_player.x, w.p2.y - g_player.y };

	Point2D p1_rot = rotate_point(p1_rel, -g_player.angle);
	Point2D p2_rot = rotate_point(p2_rel, -g_player.angle);

	if (!clip_line_behind_player(&p1_rot, &p2_rot)) {
		return;
	}

	Point3D p1_floor_3d_rot = { p1_rot.x, p1_rot.y, sector_floor_h - g_player.z };
	Point3D p1_ceil_3d_rot = { p1_rot.x, p1_rot.y, sector_ceil_h - g_player.z };

	Point3D p2_floor_3d_rot = { p2_rot.x, p2_rot.y, sector_floor_h - g_player.z };
	Point3D p2_ceil_3d_rot = { p2_rot.x, p2_rot.y, sector_ceil_h - g_player.z };

	Point2D p1_floor_screen, p1_ceil_screen;
	Point2D p2_floor_screen, p2_ceil_screen;

	int p1_project_ok = project_point(p1_floor_3d_rot, &p1_floor_screen);
	project_point(p1_ceil_3d_rot, &p1_ceil_screen);
	int p2_project_ok = project_point(p2_floor_3d_rot, &p2_floor_screen);
	project_point(p2_ceil_3d_rot, &p2_ceil_screen);

	if (!p1_project_ok || !p2_project_ok) {
		return;
	}

	int x_start_screen = (int)p1_floor_screen.x;
	int x_end_screen = (int)p2_floor_screen.x;

	if (x_start_screen > x_end_screen) {
		int temp_x = x_start_screen;
		x_start_screen = x_end_screen;
		x_end_screen = temp_x;
		Point2D temp_p = p1_floor_screen;
		p1_floor_screen = p2_floor_screen;
		p2_floor_screen = temp_p;
		temp_p = p1_ceil_screen;
		p1_ceil_screen = p2_ceil_screen;
		p2_ceil_screen = temp_p;
	}

	if (x_start_screen < 0)
		x_start_screen = 0;
	if (x_end_screen >= SCREEN_WIDTH)
		x_end_screen = SCREEN_WIDTH - 1;

	for (int x = x_start_screen; x <= x_end_screen; x++) {
		float ratio = 0.0f;
		int delta_x_screen = (int)p2_floor_screen.x - (int)p1_floor_screen.x;
		if (delta_x_screen != 0) {
			ratio = (float)(x - (int)p1_floor_screen.x) / delta_x_screen;
		}

		int y_floor_wall =
			(int)(p1_floor_screen.y + (p2_floor_screen.y - p1_floor_screen.y) * ratio);
		int y_ceil_wall =
			(int)(p1_ceil_screen.y + (p2_ceil_screen.y - p1_ceil_screen.y) * ratio);

		draw_vertical_line(x, y_ceil_wall, y_floor_wall, WALL_COLOR);
		draw_vertical_line(x, y_floor_wall, SCREEN_HEIGHT - 1, floor_color);
		draw_vertical_line(x, 0, y_ceil_wall, ceil_color);
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
