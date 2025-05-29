#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "common.h"

void draw_vertical_line(int x, int y1, int y2, uint16_t color);
void draw_horizontal_line(int x1, int x2, int y, uint16_t color);
void draw_line(int x1, int y1, int x2, int y2, uint16_t color);
void clear_screen(uint16_t color);
#endif
