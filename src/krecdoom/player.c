#include <sdk.h>
#include "common.h"

#include <math.h>
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <tft.h>

void game_player_inputs(float dt)
{
	float move_speed = 50.0f * dt;	// Units per second
	float turn_speed = 100.0f * dt; // Degrees per second
	float look_speed = 50.0f * dt;	// Degrees per second

	// --- Example: Hooking into SDK buttons ---
	// You'll need to define your button mappings or use sdk_get_buttons()
	// For illustration, let's assume direct button access:
	// button_up = sdk_is_button_pressed(BUTTON_UP); // Or whatever your SDK uses
	// button_down = sdk_is_button_pressed(BUTTON_DOWN);
	// button_left = sdk_is_button_pressed(BUTTON_LEFT);
	// button_right = sdk_is_button_pressed(BUTTON_RIGHT);
	// button_L = sdk_is_button_pressed(BUTTON_L); // Turn Left
	// button_R = sdk_is_button_pressed(BUTTON_R); // Turn Right
	// button_Y = sdk_is_button_pressed(BUTTON_Y); // Look Up
	// button_A = sdk_is_button_pressed(BUTTON_A); // Look Down

	// Forward/Backward movement (relative to player's current facing angle)
	if (sdk_inputs.joy_y < -500) {
		g_player.x += g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
		g_player.y += g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	}
	if (sdk_inputs.joy_y > 500) {
		g_player.x -= g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
		g_player.y -= g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	}

	// Strafing (perpendicular to player's facing angle)
	if (sdk_inputs.joy_x < -500) { // Strafe Left
		g_player.x += g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
		g_player.y -= g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	}
	if (sdk_inputs.joy_x > 500) { // Strafe Right
		g_player.x -= g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
		g_player.y += g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	}

	// Turning (yaw)
	if (sdk_inputs.x) { // Turn Left
		g_player.angle -= turn_speed;
	}
	if (sdk_inputs.b) { // Turn Right
		g_player.angle += turn_speed;
	}

	// Wrap angle to keep it between 0 and ANGLE_MAX-1
	g_player.angle = fmodf(g_player.angle, ANGLE_MAX);
	if (g_player.angle < 0)
		g_player.angle += ANGLE_MAX;

	// Looking Up/Down (pitch)
	if (sdk_inputs.y) { // Look Up
		g_player.look += look_speed;
	}
	if (sdk_inputs.a) { // Look Down
		g_player.look -= look_speed;
	}

	// Clamp look angle to prevent flipping over (-90 to +90 degrees)
	if (g_player.look > 90.0f)
		g_player.look = 90.0f;
	if (g_player.look < -90.0f)
		g_player.look = -90.0f;
}
