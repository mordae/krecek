#include "enemy.h"
#include "player.h"
#include "maps.h"
#include <math.h>
#include <tft.h>
#include <stdio.h>
#include <stdlib.h>

Zombik zombik = {
	.x = 4.5f,
	.y = 4.5f,
	.dx = 0.0f,
	.dy = 0.0f,
	.speed = 0.5f,
	.alive = 1,
	.health = 100,
};

void game_zombie_start(void)
{
	do {
		zombik.x = 1.5f + (rand() % (MAP_WIDTH - 2));
		zombik.y = 1.5f + (rand() % (MAP_HEIGHT - 2));
	} while (map[(int)zombik.y][(int)zombik.x] == WALL ||
		 (fabs(zombik.x - player.x) < 2.0f && fabs(zombik.y - player.y) < 2.0f));

	zombik.speed = 0.5f;
	zombik.health = 100;
	zombik.alive = 1;
}
void game_zombie_handle(float dt)
{
	if (!zombik.alive)
		return;

	float dx = player.x - zombik.x;
	float dy = player.y - zombik.y;
	float dist = sqrtf(dx * dx + dy * dy);

	if (dist > 0.1f) {
		dx /= dist;
		dy /= dist;
	}

	float newX = zombik.x + dx * zombik.speed * dt;
	float newY = zombik.y + dy * zombik.speed * dt;

	if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT &&
	    map[(int)newY][(int)newX] != WALL) {
		zombik.x = newX;
		zombik.y = newY;
	}

	printf("Zombie position: (%.2f, %.2f)\n", zombik.x, zombik.y);
}
void game_zombie_paint(float dt)
{
	if (!zombik.alive)
		return;

	float zombieX = zombik.x - player.x;
	float zombieY = zombik.y - player.y;

	float invDet = 1.0f / (cosf(player.angle) * -sinf(player.angle) -
			       sinf(player.angle) * cosf(player.angle));
	float transformX = invDet * (-sinf(player.angle) * zombieX + cosf(player.angle) * zombieY);
	float transformY = invDet * (cosf(player.angle) * zombieX + sinf(player.angle) * zombieY);

	if (transformY <= 0)
		return;
	int zombieScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

	int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));
	int spriteWidth = spriteHeight;

	int drawStartX = -spriteWidth / 2 + zombieScreenX;
	int drawEndX = spriteWidth / 2 + zombieScreenX;
	int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
	int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;

	if (drawStartX < 0)
		drawStartX = 0;
	if (drawEndX >= SCREEN_WIDTH)
		drawEndX = SCREEN_WIDTH - 1;
	if (drawStartY < 0)
		drawStartY = 0;
	if (drawEndY >= SCREEN_HEIGHT)
		drawEndY = SCREEN_HEIGHT - 1;

	printf("Zombie world pos: (%.2f, %.2f) | Screen pos: %d | Size: %dx%d\n", zombik.x,
	       zombik.y, zombieScreenX, spriteWidth, spriteHeight);

	for (int y = drawStartY; y < drawEndY; y++) {
		for (int x = drawStartX; x < drawEndX; x++) {
			if (transformY < z_buffer[x]) { // Depth check
				tft_draw_pixel(x, y, rgb_to_rgb565(0, 255, 0));
			}
		}
	}
}
