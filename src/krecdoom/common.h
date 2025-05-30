#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <math.h>

#include "graphics.h"
#include "colors.h"

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120

#define MOVE_SPEED 0.5f
#define ROTATE_SPEED 1.5f
#define FLY_SPEED_FACTOR 2.0f
#define FOV_DEGREES 90.0f
#define PROJECTION_SCALE 80
#define NEAR_PLANE 0.1f

#define ANGLE_MAX 360
extern float g_cos_table[ANGLE_MAX];
extern float g_sin_table[ANGLE_MAX];

// --- Structures ---
typedef struct {
	float x, y;
} Point2D;

typedef struct {
	float x, y, z;
} Point3D;

typedef struct {
	Point2D p1, p2; // Start and end points of the wall in 2D world space
} Wall;

typedef struct {
	int start_wall_idx; // Index into g_walls array
	int num_walls;	    // Number of walls belonging to this sector
	float floor_height;
	float ceil_height;
	uint16_t floor_color;
	uint16_t ceil_color;
} Sector;

typedef struct {
	float x, y, z;	// Position in 3D world space
	float angle;	// Horizontal viewing angle (0-359 degrees)
	float look;	// Vertical look angle (pitch)
	bool is_flying; // If player is in flying mode (no gravity)
} Player;

extern Player g_player;

extern Wall g_walls[500]; // Should be extern
extern int g_num_walls;
extern Sector g_sectors[50]; // Should be extern
extern int g_num_sectors;

#endif // COMMON_H
