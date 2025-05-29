#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "common.h" // For Point2D, Point3D, bool

// Function to initialize pre-calculated sine/cosine tables
void init_math_tables(void);

// Function to rotate a 2D point around the origin
Point2D rotate_point(Point2D p, float angle);

// Function to clip a 2D line segment that is behind the player's view plane
bool clip_line_behind_player(Point2D *p1, Point2D *p2);

// Function to project a 3D point onto the 2D screen
int project_point(Point3D p, Point2D *screen_p);

#endif // MATH_UTILS_H
