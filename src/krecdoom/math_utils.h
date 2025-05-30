#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "common.h"
void init_math_tables(void);
Point2D rotate_point(Point2D p, float angle);
bool clip_line_behind_player(Point2D *p1, Point2D *p2);
int project_point(Point3D p, Point2D *screen_p);
#endif // MATH_UTILS_H
