#pragma once

#include "common.h"

typedef struct {
	float x, y;
	float speed;
	int alive;
	int health;
	float dx, dy;
} Zombik;

extern Zombik zombik;

#ifndef PLAYER_H
#define PLAYER_H

void game_zombie_handle(float dt);
void game_zombie_paint(float dt);
void game_zombie_start(void);

#endif
