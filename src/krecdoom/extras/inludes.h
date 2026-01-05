#pragma once
#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tft.h>

#include <Chainsaw.png.h>
#include <cover.png.h>
#include <overline.png.h>
#include <pistol.png.h>
#include <shotgun.png.h>

#include "../textures/TTE_00.h"
#include "../textures/TTE_01.h"
#include "../textures/TTO_00.h"
#include "../textures/TTO_01.h"
#include "../textures/TTO_02.h"
#include "../textures/DOORS.h"
#include "../textures/T_00.h"
#include "../textures/T_01.h"
#include "../textures/T_02.h"
#include "../textures/T_03.h"
#include "../textures/T_04.h"
#include "../textures/T_05.h"
#include "../textures/T_06.h"
#include "../textures/T_07.h"
#include "../textures/T_08.h"
#include "../textures/T_09.h"
#include "../textures/T_10.h"
#include "../textures/T_11.h"
#include "../textures/T_12.h"
#include "../textures/T_13.h"
#include "../textures/T_14.h"
#include "../textures/T_15.h"
#include "../textures/T_16.h"
#include "../textures/T_17.h"
#include "../textures/T_18.h"
#include "../textures/T_19.h"

const char *texture_names[] = {
	T_00, T_01, T_02, T_03, T_04, T_05, T_06, T_07, T_08, T_09,
	T_10, T_11, T_12, T_13, T_14, T_15, T_16, T_17, T_18,
};

const int texture_heights[] = {
	T_00_HEIGHT, T_01_HEIGHT, T_02_HEIGHT, T_03_HEIGHT, T_04_HEIGHT, T_05_HEIGHT, T_06_HEIGHT,
	T_07_HEIGHT, T_08_HEIGHT, T_09_HEIGHT, T_10_HEIGHT, T_11_HEIGHT, T_12_HEIGHT, T_13_HEIGHT,
	T_14_HEIGHT, T_15_HEIGHT, T_16_HEIGHT, T_17_HEIGHT, T_18_HEIGHT,
};

const int texture_widths[] = {
	T_00_WIDTH, T_01_WIDTH, T_02_WIDTH, T_03_WIDTH, T_04_WIDTH, T_05_WIDTH, T_06_WIDTH,
	T_07_WIDTH, T_08_WIDTH, T_09_WIDTH, T_10_WIDTH, T_11_WIDTH, T_12_WIDTH, T_13_WIDTH,
	T_14_WIDTH, T_15_WIDTH, T_16_WIDTH, T_17_WIDTH, T_18_WIDTH,
};
