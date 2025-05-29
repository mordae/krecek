#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>
#include "graphics.h"
#define MY_BLUE rgb_to_rgb565(0, 0, 255)
#define MY_RED rgb_to_rgb565(255, 0, 0)
#define MY_YELLOW rgb_to_rgb565(255, 255, 0)
#define MY_GREEN rgb_to_rgb565(0, 255, 0)
#define MY_DARK_TEAL rgb_to_rgb565(50, 100, 100)
#define MY_WHITE rgb_to_rgb565(255, 255, 255)
#define MY_LIGHT_GRAY rgb_to_rgb565(180, 180, 180)

#define DEFAULT_FLOOR_COLOR MY_GREEN
#define DEFAULT_CEILING_COLOR MY_RED
#define DEFAULT_WALL_COLOR MY_LIGHT_GRAY
#define MY_LIGHT_GRAY rgb_to_rgb565(180, 180, 180)
#define WALL_COLOR MY_LIGHT_GRAY

#endif
