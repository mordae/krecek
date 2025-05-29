#ifndef COMMON_H
#define COMMON_H

#include <stdint.h> // For uint16_t (colors)
#include <math.h>   // For M_PI (pi constant) and tanf (tangent function)

// --- Screen and Display Constants ---
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#define FOV_DEGREES 60.0f // Field of View in degrees

// --- Math Lookup Table Constants and Projection Scale ---
#define ANGLE_MAX 360 // For 0-359 degrees in lookup tables
// M_PI is typically defined in <math.h>
#define HALF_FOV_RADIANS (FOV_DEGREES * 0.5f * (M_PI / 180.0f)) // Convert half FOV to radians
// PROJECTION_SCALE is a constant derived from FOV and screen width, used in 3D projection
#define PROJECTION_SCALE ((float)SCREEN_WIDTH / (2.0f * tanf(HALF_FOV_RADIANS)))

// --- Basic 2D/3D Point/Vector Structures ---
typedef struct {
	float x, y;
} Point2D;

typedef struct {
	float x, y, z;
} Point3D;

// --- Player Structure ---
typedef struct {
	float x, y, z; // Player's 3D world position
	float angle;   // Horizontal rotation (yaw) in degrees (0-359)
	float look;    // Vertical rotation (pitch) in degrees (-90 to +90)
} Player;

// --- Wall Structure ---
typedef struct {
	Point2D p1, p2; // 2D points defining the base of the wall in world space (x, y)
	// Additional properties like texture ID or explicit color could be added here
} Wall;

// --- Sector Structure ---
typedef struct {
	int start_wall_idx;   // Index to the first wall in the g_walls array for this sector
	int num_walls;	      // Number of walls that belong to this sector
	float floor_height;   // Z-coordinate of the floor (world Z)
	float ceil_height;    // Z-coordinate of the ceiling (world Z)
	uint16_t floor_color; // Color of the floor (RGB565 format)
	uint16_t ceil_color;  // Color of the ceiling (RGB565 format)
} Sector;

// --- Global Game State Variables (declared as extern, defined in math_utils.c) ---
extern Player g_player;
extern Wall g_walls[];	   // Array to store all wall definitions
extern Sector g_sectors[]; // Array to store all sector definitions
extern int g_num_walls;	   // Current number of walls in use
extern int g_num_sectors;  // Current number of sectors in use

// --- Global Math Lookup Tables (declared as extern, defined in math_utils.c) ---
extern float g_sin_table[ANGLE_MAX];
extern float g_cos_table[ANGLE_MAX];

// --- Math Utility Function Declarations (implementations in math_utils.c) ---
void init_math_tables(void);
Point2D rotate_point(Point2D p_world, float angle_degrees);
// Clips a 2D line segment in player-relative, rotated space against the near plane.
// Returns 1 if visible, 0 if fully clipped. Modifies p1_rot/p2_rot if clipped.
int clip_line_behind_player(Point2D *p1_rot, Point2D *p2_rot);
// Projects a 3D point (player-relative, rotated) to 2D screen coordinates.
// Returns 1 if successfully projected, 0 if point is behind player or invalid.
int project_point(Point3D p_rel_rotated, Point2D *p_screen);

#endif // COMMON_H
