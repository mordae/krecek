#include "common.h"
#include "graphics.h"

#include "maps.h"
#include <math.h>

void init_scene_octagonal_structure(void)
{
	// Constants for the central structure
	float oct_radius_base = 50.0f;
	float oct_radius_platform = 40.0f;
	float oct_y_center_offset = 50.0f; // Shift it forward on Y axis
	int num_oct_sides = 8;
	float oct_base_height = 30.0f;
	float oct_platform_height = 5.0f;
	float oct_small_box_height = 10.0f;
	float small_box_size = 10.0f;

	// Define the Red Octagonal Base Sector
	Point2D oct_base_points[num_oct_sides];
	for (int i = 0; i < num_oct_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_oct_sides);
		oct_base_points[i].x = oct_radius_base * g_cos_table[(int)angle % ANGLE_MAX];
		oct_base_points[i].y =
			oct_radius_base * g_sin_table[(int)angle % ANGLE_MAX] + oct_y_center_offset;
	}
	int red_base_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_oct_sides; i++) {
		g_walls[g_num_walls++] =
			(Wall){ oct_base_points[i], oct_base_points[(i + 1) % num_oct_sides] };
	}
	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = red_base_start_wall_idx,
					       .num_walls = num_oct_sides,
					       .floor_height = 0.0f,
					       .ceil_height = oct_base_height,
					       .floor_color = RED,
					       .ceil_color = RED };

	// Define the Yellow Octagonal Platform Sector
	Point2D oct_platform_points[num_oct_sides];
	for (int i = 0; i < num_oct_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_oct_sides);
		oct_platform_points[i].x =
			oct_radius_platform * g_cos_table[(int)angle % ANGLE_MAX];
		oct_platform_points[i].y =
			oct_radius_platform * g_sin_table[(int)angle % ANGLE_MAX] +
			oct_y_center_offset;
	}
	int yellow_platform_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_oct_sides; i++) {
		g_walls[g_num_walls++] = (Wall){ oct_platform_points[i],
						 oct_platform_points[(i + 1) % num_oct_sides] };
	}
	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = yellow_platform_start_wall_idx,
					       .num_walls = num_oct_sides,
					       .floor_height = oct_base_height,
					       .ceil_height = oct_base_height + oct_platform_height,
					       .floor_color = MY_YELLOW,
					       .ceil_color = MY_YELLOW };

	// Define the Small Blue Box on top of the Yellow Platform
	float small_box_z = oct_base_height + oct_platform_height;
	int small_box_start_wall_idx = g_num_walls;
	g_walls[g_num_walls++] =
		(Wall){ { -small_box_size / 2, oct_y_center_offset + small_box_size / 2 },
			{ small_box_size / 2, oct_y_center_offset + small_box_size / 2 } };
	g_walls[g_num_walls++] =
		(Wall){ { small_box_size / 2, oct_y_center_offset + small_box_size / 2 },
			{ small_box_size / 2, oct_y_center_offset - small_box_size / 2 } };
	g_walls[g_num_walls++] =
		(Wall){ { small_box_size / 2, oct_y_center_offset - small_box_size / 2 },
			{ -small_box_size / 2, oct_y_center_offset - small_box_size / 2 } };
	g_walls[g_num_walls++] =
		(Wall){ { -small_box_size / 2, oct_y_center_offset - small_box_size / 2 },
			{ -small_box_size / 2, oct_y_center_offset + small_box_size / 2 } };
	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = small_box_start_wall_idx,
					       .num_walls = 4,
					       .floor_height = small_box_z,
					       .ceil_height = small_box_z + oct_small_box_height,
					       .floor_color = BLUE,
					       .ceil_color = BLUE };
}

void init_scene_steps(void)
{
	float step_width = 30.0f;
	float step_depth = 20.0f;
	float step_height_increment = 10.0f;
	float current_step_floor_h = 0.0f;
	float steps_y_offset =
		50.0f - 50.0f - 10.0f; // Relative to oct_y_center_offset - oct_radius_base - 10.0f

	// Step 1
	int step1_start_wall_idx = g_num_walls;
	g_walls[g_num_walls++] =
		(Wall){ { -step_width / 2, steps_y_offset }, { step_width / 2, steps_y_offset } };
	g_walls[g_num_walls++] = (Wall){ { step_width / 2, steps_y_offset },
					 { step_width / 2, steps_y_offset + step_depth } };
	g_walls[g_num_walls++] = (Wall){ { step_width / 2, steps_y_offset + step_depth },
					 { -step_width / 2, steps_y_offset + step_depth } };
	g_walls[g_num_walls++] = (Wall){ { -step_width / 2, steps_y_offset + step_depth },
					 { -step_width / 2, steps_y_offset } };
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = step1_start_wall_idx,
			  .num_walls = 4,
			  .floor_height = current_step_floor_h,
			  .ceil_height = current_step_floor_h + step_height_increment,
			  .floor_color = MY_GREEN,
			  .ceil_color = MY_GREEN };
	current_step_floor_h += step_height_increment;
	steps_y_offset += step_depth; // Move forward for next step

	// Step 2
	int step2_start_wall_idx = g_num_walls;
	g_walls[g_num_walls++] =
		(Wall){ { -step_width / 2, steps_y_offset }, { step_width / 2, steps_y_offset } };
	g_walls[g_num_walls++] = (Wall){ { step_width / 2, steps_y_offset },
					 { step_width / 2, steps_y_offset + step_depth } };
	g_walls[g_num_walls++] = (Wall){ { step_width / 2, steps_y_offset + step_depth },
					 { -step_width / 2, steps_y_offset + step_depth } };
	g_walls[g_num_walls++] = (Wall){ { -step_width / 2, steps_y_offset + step_depth },
					 { -step_width / 2, steps_y_offset } };
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = step2_start_wall_idx,
			  .num_walls = 4,
			  .floor_height = current_step_floor_h,
			  .ceil_height = current_step_floor_h + step_height_increment,
			  .floor_color = MY_GREEN,
			  .ceil_color = MY_GREEN };
	current_step_floor_h += step_height_increment;
	steps_y_offset += step_depth;

	// Step 3 (Adjust step_width to connect to the octagon if needed)
	float last_step_width = 50.0f * 2 * 0.8; // Approx 80% of octagon width
	float oct_radius_base = 50.0f;		 // Need to redefine or pass as parameter
	float oct_y_center_offset = 50.0f;	 // Need to redefine or pass as parameter

	int step3_start_wall_idx = g_num_walls;
	g_walls[g_num_walls++] = (Wall){ { -last_step_width / 2, steps_y_offset },
					 { last_step_width / 2, steps_y_offset } };
	g_walls[g_num_walls++] = (Wall){ { last_step_width / 2, steps_y_offset },
					 { last_step_width / 2,
					   oct_y_center_offset - oct_radius_base / sqrtf(2.0f) } };
	g_walls[g_num_walls++] = (Wall){
		{ last_step_width / 2, oct_y_center_offset - oct_radius_base / sqrtf(2.0f) },
		{ -last_step_width / 2, oct_y_center_offset - oct_radius_base / sqrtf(2.0f) }
	};
	g_walls[g_num_walls++] = (Wall){ { -last_step_width / 2,
					   oct_y_center_offset - oct_radius_base / sqrtf(2.0f) },
					 { -last_step_width / 2, steps_y_offset } };
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = step3_start_wall_idx,
			  .num_walls = 4,
			  .floor_height = current_step_floor_h,
			  .ceil_height = current_step_floor_h + step_height_increment,
			  .floor_color = MY_GREEN,
			  .ceil_color = MY_GREEN };
}

void init_scene_pillars(void)
{
	float pillar_radius = 12.0f;
	float pillar_height_shaft = 60.0f;
	float pillar_height_top = 10.0f;
	int num_pillar_sides = 12;	   // Use 12 or 16 for better cylinder approximation
	float oct_y_center_offset = 50.0f; // Need to redefine or pass as parameter

	// Left Pillar Shaft
	float left_pillar_x = -100.0f;
	float left_pillar_y = oct_y_center_offset; // Align with central structure
	Point2D left_pillar_base_points[num_pillar_sides];
	for (int i = 0; i < num_pillar_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_pillar_sides);
		left_pillar_base_points[i].x =
			left_pillar_x + pillar_radius * g_cos_table[(int)angle % ANGLE_MAX];
		left_pillar_base_points[i].y =
			left_pillar_y + pillar_radius * g_sin_table[(int)angle % ANGLE_MAX];
	}
	int left_pillar_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_pillar_sides; i++) {
		g_walls[g_num_walls++] =
			(Wall){ left_pillar_base_points[i],
				left_pillar_base_points[(i + 1) % num_pillar_sides] };
	}
	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = left_pillar_start_wall_idx,
					       .num_walls = num_pillar_sides,
					       .floor_height = 0.0f,
					       .ceil_height = pillar_height_shaft,
					       .floor_color = MY_BLUE,
					       .ceil_color = MY_BLUE };

	// Left Pillar Top Part (Blue)
	float left_pillar_top_radius = 15.0f; // Slightly wider top
	Point2D left_pillar_top_points[num_pillar_sides];
	for (int i = 0; i < num_pillar_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_pillar_sides);
		left_pillar_top_points[i].x =
			left_pillar_x +
			left_pillar_top_radius * g_cos_table[(int)angle % ANGLE_MAX];
		left_pillar_top_points[i].y =
			left_pillar_y +
			left_pillar_top_radius * g_sin_table[(int)angle % ANGLE_MAX];
	}
	int left_pillar_top_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_pillar_sides; i++) {
		g_walls[g_num_walls++] =
			(Wall){ left_pillar_top_points[i],
				left_pillar_top_points[(i + 1) % num_pillar_sides] };
	}
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = left_pillar_top_start_wall_idx,
			  .num_walls = num_pillar_sides,
			  .floor_height = pillar_height_shaft,
			  .ceil_height = pillar_height_shaft + pillar_height_top,
			  .floor_color = MY_BLUE,
			  .ceil_color = MY_BLUE };

	// Right Pillar (repeat similar logic for right pillar)
	float right_pillar_x = 100.0f;
	float right_pillar_y = oct_y_center_offset;
	Point2D right_pillar_base_points[num_pillar_sides];
	for (int i = 0; i < num_pillar_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_pillar_sides);
		right_pillar_base_points[i].x =
			right_pillar_x + pillar_radius * g_cos_table[(int)angle % ANGLE_MAX];
		right_pillar_base_points[i].y =
			right_pillar_y + pillar_radius * g_sin_table[(int)angle % ANGLE_MAX];
	}
	int right_pillar_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_pillar_sides; i++) {
		g_walls[g_num_walls++] =
			(Wall){ right_pillar_base_points[i],
				right_pillar_base_points[(i + 1) % num_pillar_sides] };
	}
	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = right_pillar_start_wall_idx,
					       .num_walls = num_pillar_sides,
					       .floor_height = 0.0f,
					       .ceil_height = pillar_height_shaft,
					       .floor_color = MY_BLUE,
					       .ceil_color = MY_BLUE };

	// Right Pillar Top Part (Blue)
	Point2D right_pillar_top_points[num_pillar_sides];
	for (int i = 0; i < num_pillar_sides; i++) {
		float angle = (float)i * (360.0f / (float)num_pillar_sides);
		right_pillar_top_points[i].x =
			right_pillar_x +
			left_pillar_top_radius * g_cos_table[(int)angle % ANGLE_MAX];
		right_pillar_top_points[i].y =
			right_pillar_y +
			left_pillar_top_radius * g_sin_table[(int)angle % ANGLE_MAX];
	}
	int right_pillar_top_start_wall_idx = g_num_walls;
	for (int i = 0; i < num_pillar_sides; i++) {
		g_walls[g_num_walls++] =
			(Wall){ right_pillar_top_points[i],
				right_pillar_top_points[(i + 1) % num_pillar_sides] };
	}
	g_sectors[g_num_sectors++] =
		(Sector){ .start_wall_idx = right_pillar_top_start_wall_idx,
			  .num_walls = num_pillar_sides,
			  .floor_height = pillar_height_shaft,
			  .ceil_height = pillar_height_shaft + pillar_height_top,
			  .floor_color = MY_BLUE,
			  .ceil_color = MY_BLUE };
}

void init_scene_horizontal_beam(void)
{
	float oct_y_center_offset = 50.0f;    // Need to redefine or pass as parameter
	float pillar_height_shaft = 60.0f;    // Need to redefine or pass as parameter
	float pillar_height_top = 10.0f;      // Need to redefine or pass as parameter
	float left_pillar_top_radius = 15.0f; // Need to redefine or pass as parameter
	float left_pillar_x = -100.0f;	      // Need to redefine or pass as parameter
	float right_pillar_x = 100.0f;	      // Need to redefine or pass as parameter

	// Horizontal Red Beam
	float beam_y_center = oct_y_center_offset; // Centered on Y with the main structure
	float beam_x_start = left_pillar_x + left_pillar_top_radius; // From left pillar
	float beam_x_end = right_pillar_x - left_pillar_top_radius;  // To right pillar
	float beam_thickness = 15.0f;				     // Depth of the beam
	float beam_z_base =
		pillar_height_shaft + pillar_height_top - 5.0f; // Just below pillar tops
	float beam_height = 20.0f;				// Vertical height of the beam

	int beam_start_wall_idx = g_num_walls;
	// Define the 4 walls of the rectangular beam
	g_walls[g_num_walls++] =
		(Wall){ { beam_x_start, beam_y_center + beam_thickness / 2 },
			{ beam_x_end, beam_y_center + beam_thickness / 2 } }; // Front wall
	g_walls[g_num_walls++] =
		(Wall){ { beam_x_end, beam_y_center + beam_thickness / 2 },
			{ beam_x_end, beam_y_center - beam_thickness / 2 } }; // Right wall
	g_walls[g_num_walls++] =
		(Wall){ { beam_x_end, beam_y_center - beam_thickness / 2 },
			{ beam_x_start, beam_y_center - beam_thickness / 2 } }; // Back wall
	g_walls[g_num_walls++] =
		(Wall){ { beam_x_start, beam_y_center - beam_thickness / 2 },
			{ beam_x_start, beam_y_center + beam_thickness / 2 } }; // Left wall

	g_sectors[g_num_sectors++] = (Sector){ .start_wall_idx = beam_start_wall_idx,
					       .num_walls = 4,
					       .floor_height = beam_z_base,
					       .ceil_height = beam_z_base + beam_height,
					       .floor_color = MY_RED,
					       .ceil_color = MY_RED };
}

// --- Master Map Initialization Function ---
void init_game_map(void)
{
	// Reset global counters before defining the new map
	g_num_walls = 0;
	g_num_sectors = 0;

	// Call individual scene component functions to build the map
	init_scene_octagonal_structure();
	init_scene_steps();
	init_scene_pillars();
	init_scene_horizontal_beam();
}
