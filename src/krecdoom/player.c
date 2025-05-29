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
	float fly_speed_vertical = 30.0f * dt;

	float dx = 0.0f;
	float dy = 0.0f; // For forward/backward and strafing

	// Read joystick input for movement direction
	if (sdk_inputs.joy_y < -500) { // Forward
		dy = -1.0f;
	} else if (sdk_inputs.joy_y > 500) { // Backward
		dy = 1.0f;
	}

	if (sdk_inputs.joy_x < -500) { // Strafe Left
		dx = -1.0f;
	} else if (sdk_inputs.joy_x > 500) { // Strafe Right
		dx = 1.0f;
	}

	// Normalize diagonal movement to prevent faster movement
	float magnitude = sqrtf(dx * dx + dy * dy);
	if (magnitude > 0.1f) { // Use a small epsilon to avoid division by zero and small noise
		dx /= magnitude;
		dy /= magnitude;
	}

	// Apply movement based on normalized direction and player's angle
	// Forward/Backward component
	g_player.x -= dy * g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	g_player.y -= dy * g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;

	// Strafing component (perpendicular to player's facing angle)
	g_player.x += dx * g_sin_table[(int)g_player.angle % ANGLE_MAX] * move_speed;
	g_player.y -= dx * g_cos_table[(int)g_player.angle % ANGLE_MAX] * move_speed;

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

	if (sdk_inputs.y && !g_player.is_flying) {    // 'Y' for Jump (if not flying)
		g_player.z += fly_speed_vertical * 2; // Make jump a bit faster than fly
	}
	if (sdk_inputs.a && !g_player.is_flying) { // 'A' for Crouch (if not flying)
		g_player.z -= fly_speed_vertical * 2;
		if (g_player.z < 0.0f)
			g_player.z = 0.0f; // Prevent going below floor
	}
	if (sdk_inputs_delta.start) {
		g_player.is_flying = !g_player.is_flying;
		printf("Flying mode: %s\n", g_player.is_flying ? "ON" : "OFF"); // For debugging
	}
	if (g_player.is_flying) {
		if (sdk_inputs.y) { // 'Y' button to ascend
			g_player.z += fly_speed_vertical;
		}
		if (sdk_inputs.a) { // 'A' button to descend
			g_player.z -= fly_speed_vertical;
		}
	}

	// Clamp look angle to prevent flipping over (-90 to +90 degrees)
	if (g_player.look > 90.0f)
		g_player.look = 90.0f;
	if (g_player.look < -90.0f)
		g_player.look = -90.0f;

	if (!g_player.is_flying && g_player.z < 0.0f) { // Assuming floor is at Z=0
		g_player.z = 0.0f;
	}
}
