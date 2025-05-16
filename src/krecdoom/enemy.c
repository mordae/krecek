#include "enemy.h"
#include "player.h"
#include "maps.h"
#include <math.h>
#include <tft.h>
#include <stdio.h>

float z_buffer[SCREEN_WIDTH];

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
	zombik.x = 1.5f;
	zombik.y = 3.5f;
	zombik.speed = 0.5f;
	zombik.health = 100;
	zombik.alive = 1;
	zombik.dx = 0;
	zombik.dy = 0;
}

void game_zombie_handle(float dt)
{
	if (!zombik.alive)
		return;

	zombik.dx = player.x - zombik.x;
	zombik.dy = player.y - zombik.y;
	float dist = sqrtf(zombik.dx * zombik.dx + zombik.dy * zombik.dy);

	if (dist > 0.1f && dist < 5.0f) {
		zombik.dx /= dist;
		zombik.dy /= dist;

		int newX = zombik.x + zombik.dx * zombik.speed * dt;
		int newY = zombik.y + zombik.dy * zombik.speed * dt;

		if (maps_map1[(int)newY][(int)newX] != WALL) {
			zombik.x = newX;
			zombik.y = newY;
		}
	}

	if (dist < 0.5f) {
		printf("Zombie attacking player!\n");
	}
	printf("------------------\n");
	printf("zombik-x-%f-------\n", zombik.x);
	printf("zombik-y-%f-------\n", zombik.y);
	printf("------------------\n");
}

void game_zombie_paint(void)
{
	if (!zombik.alive)
		return;

	zombik.dx = zombik.x - player.x;
	zombik.dy = zombik.y - player.y;
	int dist = sqrtf(zombik.dx * zombik.dx + zombik.dy * zombik.dy);

	if (dist > 0.1f && dist < 5.0f) {
		float angleToZombie = atan2f(zombik.dy, zombik.dx);
		float relativeAngle = angleToZombie - player.angle;

		while (relativeAngle > M_PI)
			relativeAngle -= 2 * M_PI;
		while (relativeAngle < -M_PI)
			relativeAngle += 2 * M_PI;

		if (fabsf(relativeAngle) < player.fov) {
			float screenX = (relativeAngle / player.fov + 1.0f) * 0.5f * 160;

			int size = (int)(120.0f / dist);
			if (size > 60)
				size = 60;

			int drawX = (int)screenX - size / 2;
			int drawY = 60 - size / 2;

			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++) {
					tft_draw_pixel(drawX + x, drawY + y,
						       rgb_to_rgb565(0, 255, 0));
				}
			}
			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++) {
					// Outline green box
					color_t c = (x == 0 || y == 0 || x == size - 1 ||
						     y == size - 1) ?
							    rgb_to_rgb565(0, 255, 0) :
							    rgb_to_rgb565(50, 100, 50);
					tft_draw_pixel(drawX + x, drawY + y, c);
				}
			}
		}
	}
}
