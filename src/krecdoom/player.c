#include <sdk.h>
#include "common.h"

#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <tft.h>

void game_player_inputs(float dt)
{
	float move_step = dt * 2.0f;
	float rot_step = dt * 2.5f;

	if (sdk_inputs.joy_x > -500) {
		player.angle -= rot_step;
	}
	if (sdk_inputs.joy_x < 500) {
		player.angle += rot_step;
	}
	player.dx = cosf(player.angle);
	player.dy = sinf(player.angle);

	if (sdk_inputs.joy_y < -500) {
		player.nx = player.x + player.dx * move_step;
		player.ny = player.y + player.dy * move_step;
		if (map[(int)player.ny][(int)player.nx] == 0) {
			player.x = player.nx;
			player.y = player.ny;
		}
	}
	if (sdk_inputs.joy_y > 500) {
		player.nx = player.x - player.dx * move_step;
		player.ny = player.y - player.dy * move_step;
		if (map[(int)player.ny][(int)player.nx] == 0) {
			player.x = player.nx;
			player.y = player.ny;
		}
	}
}
