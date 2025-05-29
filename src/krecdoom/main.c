#include "common.h" #include "graphics.h"
#include "volume.h"

#include <pico/stdlib.h> // Pico SDK standard library
#include <sdk.h>	 // Your game SDK functions
#include <stdio.h>	 // Standard I/O for debugging (e.g., printf)
#include <math.h>	 // For fmodf in player movement
#include <tft.h>

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define GRAY rgb_to_rgb565(100, 100, 100)
#define GREEN_FLOOR rgb_to_rgb565(50, 150, 50)
#define RED_CEILING rgb_to_rgb565(150, 50, 50)
#define WALL_COLOR WHITE // Example wall color

float volume = 0.5f;

void game_player_inputs(float dt);
void draw_sector(int sector_idx);
void draw_wall(int wall_idx, float sector_floor_h, float sector_ceil_h, uint16_t floor_color,
	       uint16_t ceil_color);
void draw_vertical_line(int x, int y1, int y2, uint16_t color);

void game_start(void)
{
	sdk_set_output_gain_db(volume);

	init_math_tables();

	g_player.x = 70.0f;
	g_player.y = 110.0f;
	g_player.z = 20.0f;    // Player's eye level / camera height
	g_player.angle = 0.0f; // Looking along positive X (or adjust based on your world setup)
	g_player.look = 0.0f;  // Looking straight ahead (no vertical pitch)

	// --- Define Game World (Example: A simple square room) ---
	// Walls are defined in 2D (x,y) world coordinates
	// IMPORTANT: Define walls in a consistent order (e.g., clockwise or counter-clockwise)
	// for correct rendering and potential portal system (later).

	// Wall 0: (X -50, Y 50) to (X 50, Y 50) - Back wall (relative to initial player position)
	g_walls[g_num_walls++] = (Wall){ { -50, 50 }, { 50, 50 } };
	// Wall 1: (X 50, Y 50) to (X 50, Y -50) - Right wall
	g_walls[g_num_walls++] = (Wall){ { 50, 50 }, { 50, -50 } };
	// Wall 2: (X 50, Y -50) to (X -50, Y -50) - Front wall
	g_walls[g_num_walls++] = (Wall){ { 50, -50 }, { -50, -50 } };
	// Wall 3: (X -50, Y -50) to (X -50, Y 50) - Left wall
	g_walls[g_num_walls++] = (Wall){ { -50, -50 }, { -50, 50 } };

	// Define a single sector for the room
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = 0,	// Starts with wall 0
			  .num_walls = 4,	// Contains 4 walls
			  .floor_height = 0.0f, // Floor at world Z=0
			  .ceil_height = 64.0f, // Ceiling at world Z=64 (example height)
			  .floor_color = GREEN_FLOOR,
			  .ceil_color = RED_CEILING };
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
	for (int i = 0; i < g_num_sectors; i++) {
		draw_sector(i);
	}
}

// --- Drawing a Single Sector ---
void draw_sector(int sector_idx)
{
	Sector s = g_sectors[sector_idx];

	// Iterate through all walls belonging to this sector
	for (int i = 0; i < s.num_walls; i++) {
		draw_wall(s.start_wall_idx + i, s.floor_height, s.ceil_height, s.floor_color,
			  s.ceil_color);
	}
}

// --- Drawing a Single Wall ---
void draw_wall(int wall_idx, float sector_floor_h, float sector_ceil_h, uint16_t floor_color,
	       uint16_t ceil_color)
{
	Wall w = g_walls[wall_idx];

	// 1. Translate world coordinates of wall points to be relative to the player's position
	Point2D p1_rel = { w.p1.x - g_player.x, w.p1.y - g_player.y };
	Point2D p2_rel = { w.p2.x - g_player.x, w.p2.y - g_player.y };

	// 2. Rotate these relative points to align with the player's view direction
	// We rotate by the negative of the player's angle because the world rotates around the stationary player.
	Point2D p1_rot = rotate_point(p1_rel, -g_player.angle);
	Point2D p2_rot = rotate_point(p2_rel, -g_player.angle);

	// 3. Clip the line segment (p1_rot, p2_rot) against the player's near view plane (Y > 0.1)
	// This is crucial to prevent "fisheye" distortion and artifacts when walls cross the camera.
	if (!clip_line_behind_player(&p1_rot, &p2_rot)) {
		return; // The entire wall segment is behind the player or invalid after clipping
	}

	// 4. Create 3D points for the floor and ceiling of the wall, relative to the player's eye level (g_player.z)
	Point3D p1_floor_3d_rot = { p1_rot.x, p1_rot.y, sector_floor_h - g_player.z };
	Point3D p1_ceil_3d_rot = { p1_rot.x, p1_rot.y, sector_ceil_h - g_player.z };

	Point3D p2_floor_3d_rot = { p2_rot.x, p2_rot.y, sector_floor_h - g_player.z };
	Point3D p2_ceil_3d_rot = { p2_rot.x, p2_rot.y, sector_ceil_h - g_player.z };

	// 5. Project these 3D points onto the 2D screen plane
	Point2D p1_floor_screen, p1_ceil_screen;
	Point2D p2_floor_screen, p2_ceil_screen;

	// project_point also includes a check for depth, but clip_line_behind_player should mostly handle it
	int p1_project_ok = project_point(p1_floor_3d_rot, &p1_floor_screen);
	project_point(p1_ceil_3d_rot,
		      &p1_ceil_screen); // Ceil point validity usually mirrors floor point's
	int p2_project_ok = project_point(p2_floor_3d_rot, &p2_floor_screen);
	project_point(p2_ceil_3d_rot,
		      &p2_ceil_screen); // Ceil point validity usually mirrors floor point's

	if (!p1_project_ok || !p2_project_ok) {
		return; // Should theoretically not happen if clip_line_behind_player works perfectly.
	}

	// 6. Rasterize the wall: Iterate through the screen X coordinates covered by the projected wall.
	// At each X, draw a vertical line for the wall, and fill portions for the floor and ceiling.

	int x_start_screen = (int)p1_floor_screen.x;
	int x_end_screen = (int)p2_floor_screen.x;

	// Ensure proper iteration direction (x_start_screen <= x_end_screen)
	// And swap corresponding Y-coordinates for correct interpolation
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

	// Clip screen X coordinates to actual screen bounds (0 to SCREEN_WIDTH-1)
	if (x_start_screen < 0)
		x_start_screen = 0;
	if (x_end_screen >= SCREEN_WIDTH)
		x_end_screen = SCREEN_WIDTH - 1;

	// Loop through each X column on the screen that the wall spans
	for (int x = x_start_screen; x <= x_end_screen; x++) {
		// Calculate the interpolation ratio for the current X column (0.0 to 1.0)
		float ratio = 0.0f;
		int delta_x_screen = (int)p2_floor_screen.x - (int)p1_floor_screen.x;
		if (delta_x_screen !=
		    0) { // Avoid division by zero for perfectly vertical projected lines
			ratio = (float)(x - (int)p1_floor_screen.x) / delta_x_screen;
		}

		// Interpolate the screen Y coordinates for the current X column
		int y_floor_wall =
			(int)(p1_floor_screen.y + (p2_floor_screen.y - p1_floor_screen.y) * ratio);
		int y_ceil_wall =
			(int)(p1_ceil_screen.y + (p2_ceil_screen.y - p1_ceil_screen.y) * ratio);

		// Draw the vertical line for the wall itself
		draw_vertical_line(x, y_ceil_wall, y_floor_wall, WALL_COLOR);

		// Draw the floor "scan" for this column (from bottom of wall to bottom of screen)
		draw_vertical_line(x, y_floor_wall, SCREEN_HEIGHT - 1, floor_color);

		// Draw the ceiling "scan" for this column (from top of wall to top of screen)
		draw_vertical_line(x, 0, y_ceil_wall, ceil_color);
	}
}

// --- Main Program Entry Point ---
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
