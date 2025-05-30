#include "common.h"
#include <math.h>

void init_math_tables(void)
{
	for (int i = 0; i < ANGLE_MAX; i++) {
		float radians = (float)i * M_PI / 180.0f;
		g_sin_table[i] = sinf(radians);
		g_cos_table[i] = cosf(radians);
	}
}

Point2D rotate_point(Point2D p_world, float angle_degrees)
{
	Point2D rotated_p;

	// Ensure angle_degrees is within 0-359 for lookup table index
	int angle_idx = (int)fmodf(angle_degrees, ANGLE_MAX);
	if (angle_idx < 0)
		angle_idx += ANGLE_MAX; // Ensure positive index for negative angles

	float cos_a = g_cos_table[angle_idx];
	float sin_a = g_sin_table[angle_idx];

	rotated_p.x = p_world.x * cos_a - p_world.y * sin_a;
	rotated_p.y = p_world.x * sin_a + p_world.y * cos_a;
	return rotated_p;
}

int clip_line_behind_player(Point2D *p1_rot, Point2D *p2_rot)
{
	const float NEAR_PLANE_Y = 0.1f; // Small epsilon to avoid division by zero and artifacts

	// Check if both points are behind the near plane
	if (p1_rot->y <= NEAR_PLANE_Y && p2_rot->y <= NEAR_PLANE_Y) {
		return 0; // Both points are behind, line is invisible
	}

	// Check if one point is behind and the other is in front
	if (p1_rot->y <= NEAR_PLANE_Y) {
		// p1 is behind, p2 is in front. Clip p1.
		// Calculate the interpolation factor 't' where the line intersects the near plane
		float t = (NEAR_PLANE_Y - p1_rot->y) / (p2_rot->y - p1_rot->y);
		p1_rot->x = p1_rot->x + t * (p2_rot->x - p1_rot->x);
		p1_rot->y = NEAR_PLANE_Y; // Clamp to near plane
	} else if (p2_rot->y <= NEAR_PLANE_Y) {
		// p2 is behind, p1 is in front. Clip p2.
		float t = (NEAR_PLANE_Y - p2_rot->y) / (p1_rot->y - p2_rot->y);
		p2_rot->x = p2_rot->x + t * (p1_rot->x - p2_rot->x);
		p2_rot->y = NEAR_PLANE_Y; // Clamp to near plane
	}

	return 1; // Line is still visible after clipping (potentially modified)
}

int project_point(Point3D p_rel_rotated, Point2D *p_screen)
{
	float depth_y =
		p_rel_rotated
			.y; // This is the 'depth' (distance forward) in the player's local space.

	if (depth_y <= 0.1f)
		return 0; // Use a small epsilon to avoid division by zero / artifacts

	// Calculate vertical offset due to player's vertical look angle (pitch)
	float look_angle_radians = g_player.look * (M_PI / 180.0f);

	// Vertical perspective effect: points further away appear closer to the horizon
	// Adjusting based on player's Z position relative to world origin and look angle
	float vertical_offset_center = (float)SCREEN_HEIGHT / 2.0f;
	float vertical_offset_pitch =
		tanf(look_angle_radians) * PROJECTION_SCALE; // Shift due to pitch
	float vertical_offset_player_z =
		(p_rel_rotated.z / depth_y) * PROJECTION_SCALE; // Vertical perspective scaling

	// Screen X: (relative_x / depth_y) * PROJECTION_SCALE + center_x
	p_screen->x = (p_rel_rotated.x * PROJECTION_SCALE / depth_y) + (float)SCREEN_WIDTH / 2.0f;

	// Screen Y: (center_y - vertical_perspective_component + pitch_shift)
	// Note: Screen Y usually increases downwards (top-left is 0,0).
	// World Z usually increases upwards. So, a higher world Z maps to a smaller screen Y.
	p_screen->y = vertical_offset_center - vertical_offset_player_z + vertical_offset_pitch;

	return 1; // Successfully projected
}
