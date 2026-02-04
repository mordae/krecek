#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Ensure M_PI is defined
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Game Constants
#define MAP_W 30
#define MAP_H 30
#define MAP_D 5

#define SCREEN_W 160
#define SCREEN_H 120

#define FOV_DEG 90.0f
#define TAN_HALF_FOV (tanf(FOV_DEG * M_PI / 360.0f))

// Colors
#define COL_SKY rgb_to_rgb565(10, 10, 20) // Dark Sky
#define COL_GRAY rgb_to_rgb565(160, 160, 160)
#define COL_WHITE rgb_to_rgb565(255, 255, 255)
#define COL_BLACK rgb_to_rgb565(0, 0, 0)

#define COL_FLOOR rgb_to_rgb565(50, 50, 50)
#define COL_WALL COL_GRAY
#define COL_BLOCK_1 COL_WHITE
#define COL_BLOCK_2 rgb_to_rgb565(50, 200, 50)
#define COL_BLOCK_3 rgb_to_rgb565(50, 50, 200)

// Map and Player
static uint8_t map[MAP_W][MAP_H][MAP_D];

typedef struct {
	float x, y, z;
	float yaw, pitch;
} Player;

static Player player;

// Game State
typedef enum { MODE_MOVE_LOOK = 0, MODE_EDIT = 1 } GameMode;

static GameMode current_mode = MODE_MOVE_LOOK;
static int selected_block_type = 1;

// Lighting
typedef struct {
	int x, y, z;
} LightSource;

#define MAX_LIGHTS 64
static LightSource lights[MAX_LIGHTS];
static int num_lights = 0;

// Game Info
sdk_game_info("tester-3d", NULL);

// Initialize Map
static void init_map()
{
	num_lights = 0;
	for (int x = 0; x < MAP_W; x++) {
		for (int y = 0; y < MAP_H; y++) {
			for (int z = 0; z < MAP_D; z++) {
				if (z == 0) {
					map[x][y][z] = 1; // Floor
				} else if (z == MAP_D - 1) {
					map[x][y][z] = 2; // Ceiling
				} else if (x == 0 || x == MAP_W - 1 || y == 0 || y == MAP_H - 1) {
					map[x][y][z] = 2; // Wall
				} else {
					map[x][y][z] = 0; // Air
				}
			}
		}
	}

	// Add a single light source in the ceiling center
	int lx = MAP_W / 2;
	int ly = MAP_H / 2;
	int lz = MAP_D - 1; // In the ceiling
	map[lx][ly][lz] = 3; // White Block (Light)
	lights[num_lights++] = (LightSource){ lx, ly, lz };
}

void game_start(void)
{
	init_map();
	player.x = MAP_W / 2.0f;
	player.y = MAP_H / 2.0f;
	player.z = 2.5f;
	player.yaw = 0.0f;
	player.pitch = 0.0f;
	current_mode = MODE_MOVE_LOOK;

	sdk_set_backlight(SDK_BACKLIGHT_STD);
}

void game_reset(void)
{
	game_start();
}

// Raycast function for Rendering and Interaction
// Returns 1 if hit, 0 if no hit. Fills hit info if provided.
// if 'target_block' is passed, it fills the coordinates of the block hit.
// if 'prev_block' is passed, it fills the coordinates of the air block before the hit.
int cast_ray(float start_x, float start_y, float start_z, float dir_x, float dir_y, float dir_z,
	     float max_dist, int *out_x, int *out_y, int *out_z, int *prev_x, int *prev_y,
	     int *prev_z, int *out_side)
{
	int mapX = (int)start_x;
	int mapY = (int)start_y;
	int mapZ = (int)start_z;

	float deltaDistX = (dir_x == 0) ? 1e30f : fabsf(1.0f / dir_x);
	float deltaDistY = (dir_y == 0) ? 1e30f : fabsf(1.0f / dir_y);
	float deltaDistZ = (dir_z == 0) ? 1e30f : fabsf(1.0f / dir_z);

	int stepX, stepY, stepZ;
	float sideDistX, sideDistY, sideDistZ;

	if (dir_x < 0) {
		stepX = -1;
		sideDistX = (start_x - mapX) * deltaDistX;
	} else {
		stepX = 1;
		sideDistX = (mapX + 1.0f - start_x) * deltaDistX;
	}
	if (dir_y < 0) {
		stepY = -1;
		sideDistY = (start_y - mapY) * deltaDistY;
	} else {
		stepY = 1;
		sideDistY = (mapY + 1.0f - start_y) * deltaDistY;
	}
	if (dir_z < 0) {
		stepZ = -1;
		sideDistZ = (start_z - mapZ) * deltaDistZ;
	} else {
		stepZ = 1;
		sideDistZ = (mapZ + 1.0f - start_z) * deltaDistZ;
	}

	int hit = 0;
	int side = -1;
	float dist = 0.0f;

	// Track previous step
	int lastX = mapX, lastY = mapY, lastZ = mapZ;

	while (hit == 0 && dist < max_dist) {
		lastX = mapX;
		lastY = mapY;
		lastZ = mapZ;

		if (sideDistX < sideDistY) {
			if (sideDistX < sideDistZ) {
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
			} else {
				sideDistZ += deltaDistZ;
				mapZ += stepZ;
				side = 2;
			}
		} else {
			if (sideDistY < sideDistZ) {
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			} else {
				sideDistZ += deltaDistZ;
				mapZ += stepZ;
				side = 2;
			}
		}

		// Check Bounds
		if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H || mapZ < 0 ||
		    mapZ >= MAP_D) {
			return 0; // Out of bounds
		}

		if (map[mapX][mapY][mapZ] > 0) {
			hit = 1;
		}

		// Approximate distance check (Manhattan-ish for loop safety)
		// Accurate distance isn't strictly needed for the loop condition
		// if we trust max_dist steps, but let's just use a step counter or large number
	}

	if (hit) {
		if (out_x)
			*out_x = mapX;
		if (out_y)
			*out_y = mapY;
		if (out_z)
			*out_z = mapZ;
		if (prev_x)
			*prev_x = lastX;
		if (prev_y)
			*prev_y = lastY;
		if (prev_z)
			*prev_z = lastZ;
		if (out_side)
			*out_side = side;
		return 1;
	}
	return 0;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	float move_speed = 5.0f;
	float rot_speed = 2.0f;

	// --- Mode Switching ---
	if (sdk_inputs_delta.select == 1) {
		current_mode = (current_mode == MODE_MOVE_LOOK) ? MODE_EDIT : MODE_MOVE_LOOK;
	}

	// --- Movement (Always Active) ---
	// Joystick Y: Forward/Backward
	if (abs(sdk_inputs.joy_y) > 200) {
		float move = -(sdk_inputs.joy_y / 2048.0f) * move_speed * dt;
		float new_x = player.x + cosf(player.yaw) * move;
		float new_y = player.y + sinf(player.yaw) * move;

		// Simple bounds check
		if (new_x >= 0.1f && new_x < MAP_W - 0.1f)
			player.x = new_x;
		if (new_y >= 0.1f && new_y < MAP_H - 0.1f)
			player.y = new_y;
	}

	// Joystick X: Yaw
	if (abs(sdk_inputs.joy_x) > 200) {
		float rot = (sdk_inputs.joy_x / 2048.0f) * rot_speed * dt;
		player.yaw -= rot; // Inverted rotation
	}

	// --- Actions based on Mode ---
	if (current_mode == MODE_MOVE_LOOK) {
		// Fly Up (A)
		if (sdk_inputs.a) {
			player.z += move_speed * dt;
			if (player.z >= MAP_D - 0.5f)
				player.z = MAP_D - 0.5f;
		}
		// Fly Down (B)
		if (sdk_inputs.b) {
			player.z -= move_speed * dt;
			if (player.z < 0.5f)
				player.z = 0.5f;
		}
		// Pitch Up (Y)
		if (sdk_inputs.y) {
			player.pitch += rot_speed * dt;
		}
		// Pitch Down (X)
		if (sdk_inputs.x) {
			player.pitch -= rot_speed * dt;
		}
	} else { // MODE_EDIT
		// Interaction Ray
		float cy = cosf(player.yaw);
		float sy = sinf(player.yaw);
		float cp = cosf(player.pitch);
		float sp = sinf(player.pitch);
		float dir_x = cy * cp;
		float dir_y = sy * cp;
		float dir_z = sp;

		// Place Block (A)
		if (sdk_inputs_delta.a == 1) {
			int hx, hy, hz, px, py, pz;
			if (cast_ray(player.x, player.y, player.z, dir_x, dir_y, dir_z, 10.0f, &hx,
				     &hy, &hz, &px, &py, &pz, NULL)) {
				// Place at previous (air) spot
				if (px >= 0 && px < MAP_W && py >= 0 && py < MAP_H && pz >= 0 &&
				    pz < MAP_D) {
					// Don't place inside player
					if (!((int)player.x == px && (int)player.y == py &&
					      (int)player.z == pz)) {
						// Check if we are overwriting a light (unlikely since we target air, but safe)
						if (map[px][py][pz] == 3) {
							for (int i = 0; i < num_lights; i++) {
								if (lights[i].x == px &&
								    lights[i].y == py &&
								    lights[i].z == pz) {
									lights[i] = lights[--num_lights];
									break;
								}
							}
						}

						map[px][py][pz] = selected_block_type;
						// Update lights if block is white
						if (selected_block_type == 3 &&
						    num_lights < MAX_LIGHTS) {
							lights[num_lights++] =
								(LightSource){ px, py, pz };
						}
					}
				}
			}
		}

		// Break Block (B)
		if (sdk_inputs_delta.b == 1) {
			int hx, hy, hz;
			if (cast_ray(player.x, player.y, player.z, dir_x, dir_y, dir_z, 10.0f, &hx,
				     &hy, &hz, NULL, NULL, NULL, NULL)) {
				// Remove light if it was white
				if (map[hx][hy][hz] == 3) {
					for (int i = 0; i < num_lights; i++) {
						if (lights[i].x == hx && lights[i].y == hy &&
						    lights[i].z == hz) {
							// Swap with last
							lights[i] = lights[--num_lights];
							break;
						}
					}
				}
				map[hx][hy][hz] = 0;
			}
		}

		// Change Block Type (X/Y)
		if (sdk_inputs_delta.y == 1) {
			selected_block_type++;
			if (selected_block_type > 5)
				selected_block_type = 1;
		}
	}

	// Clamp Pitch
	float max_pitch = 89.0f * M_PI / 180.0f;
	if (player.pitch > max_pitch)
		player.pitch = max_pitch;
	if (player.pitch < -max_pitch)
		player.pitch = -max_pitch;
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(COL_SKY);

	// Camera Basis
	float cy = cosf(player.yaw);
	float sy = sinf(player.yaw);
	float cp = cosf(player.pitch);
	float sp = sinf(player.pitch);

	float fx = cy * cp;
	float fy = sy * cp;
	float fz = sp;

	float rx = sy;
	float ry = -cy;
	float rz = 0.0f;

	float ux = ry * fz - rz * fy;
	float uy = rz * fx - rx * fz;
	float uz = rx * fy - ry * fx;

	float aspect = (float)SCREEN_W / SCREEN_H;
	float tan_fov = TAN_HALF_FOV;

	// Render Loop
	for (int y = 0; y < SCREEN_H; y++) {
		float ndc_y = 1.0f - (2.0f * y / SCREEN_H);
		float cam_y = ndc_y * tan_fov;

		float ray_part_x = ux * cam_y;
		float ray_part_y = uy * cam_y;
		float ray_part_z = uz * cam_y;

		for (int x = 0; x < SCREEN_W; x++) {
			float ndc_x = (2.0f * x / SCREEN_W) - 1.0f;
			float cam_x = ndc_x * tan_fov * aspect;

			float dir_x = fx + rx * cam_x + ray_part_x;
			float dir_y = fy + ry * cam_x + ray_part_y;
			float dir_z = fz + rz * cam_x + ray_part_z;

			// DDA directly in render loop for performance
			// (Re-implementing DDA inline to avoid function call overhead per pixel)
			int mapX = (int)player.x;
			int mapY = (int)player.y;
			int mapZ = (int)player.z;

			float deltaDistX = (dir_x == 0) ? 1e30f : fabsf(1.0f / dir_x);
			float deltaDistY = (dir_y == 0) ? 1e30f : fabsf(1.0f / dir_y);
			float deltaDistZ = (dir_z == 0) ? 1e30f : fabsf(1.0f / dir_z);

			int stepX, stepY, stepZ;
			float sideDistX, sideDistY, sideDistZ;

			if (dir_x < 0) {
				stepX = -1;
				sideDistX = (player.x - mapX) * deltaDistX;
			} else {
				stepX = 1;
				sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
			}
			if (dir_y < 0) {
				stepY = -1;
				sideDistY = (player.y - mapY) * deltaDistY;
			} else {
				stepY = 1;
				sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
			}
			if (dir_z < 0) {
				stepZ = -1;
				sideDistZ = (player.z - mapZ) * deltaDistZ;
			} else {
				stepZ = 1;
				sideDistZ = (mapZ + 1.0f - player.z) * deltaDistZ;
			}

			int hit = 0;
			int side = -1;
			int steps = 0;

			while (hit == 0 && steps < 40) { // Limit draw distance
				if (sideDistX < sideDistY) {
					if (sideDistX < sideDistZ) {
						sideDistX += deltaDistX;
						mapX += stepX;
						side = 0;
					} else {
						sideDistZ += deltaDistZ;
						mapZ += stepZ;
						side = 2;
					}
				} else {
					if (sideDistY < sideDistZ) {
						sideDistY += deltaDistY;
						mapY += stepY;
						side = 1;
					} else {
						sideDistZ += deltaDistZ;
						mapZ += stepZ;
						side = 2;
					}
				}

				if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H ||
				    mapZ < 0 || mapZ >= MAP_D) {
					break;
				}

				if (map[mapX][mapY][mapZ] > 0) {
					hit = 1;
				}
				steps++;
			}

			if (hit) {
				uint8_t block = map[mapX][mapY][mapZ];
				uint16_t color;

				// Calculate Hit Position
				float perpWallDist;
				if (side == 0)
					perpWallDist = sideDistX - deltaDistX;
				else if (side == 1)
					perpWallDist = sideDistY - deltaDistY;
				else
					perpWallDist = sideDistZ - deltaDistZ;

				float hit_x = player.x + dir_x * perpWallDist;
				float hit_y = player.y + dir_y * perpWallDist;
				float hit_z = player.z + dir_z * perpWallDist;

				// --- Lighting Calculation ---
				float light_intensity = 0.0f;
				float max_light_dist = 20.0f; // Increased for room coverage

				for (int i = 0; i < num_lights; i++) {
					float lx = lights[i].x + 0.5f;
					float ly = lights[i].y + 0.5f;
					float lz = lights[i].z + 0.5f;

					float dx = lx - hit_x;
					float dy = ly - hit_y;
					float dz = lz - hit_z;
					float dist_sq = dx * dx + dy * dy + dz * dz;
					float dist = sqrtf(dist_sq);

					if (dist < max_light_dist) {
						// Shadow Ray
						float ldir_x = dx / dist;
						float ldir_y = dy / dist;
						float ldir_z = dz / dist;

						// Offset start position slightly to avoid self-intersection
						int shadow_hit = 0;
						if (dist > 1.0f) {
							// Simple raycast to light
							// We can reuse cast_ray logic or a simplified version
							// For performance, let's use cast_ray but only for geometry check
							// Note: cast_ray is relatively expensive to call in a loop.
							// Since user said "don't worry about performance", we do it.
							int hx, hy, hz;
							if (cast_ray(hit_x - dir_x * 0.01f, hit_y - dir_y * 0.01f, hit_z - dir_z * 0.01f,
										 ldir_x, ldir_y, ldir_z, dist + 1.0f, // Go past light center to ensure we hit it if valid
										 &hx, &hy, &hz, NULL, NULL, NULL, NULL)) {
								// Check if we hit the light source itself
								if (hx == lights[i].x && hy == lights[i].y && hz == lights[i].z) {
									shadow_hit = 0; // Visible!
								} else {
									shadow_hit = 1; // Blocked by something else
								}
							}
						}

						if (!shadow_hit) {
							// Smoother falloff (quadratic-ish)
							float norm_dist = dist / max_light_dist;
							float attenuation = 1.0f - (norm_dist * norm_dist);
							if (attenuation > 0)
								light_intensity += attenuation * 1.5f; // Boost slightly
						}
					}
				}
				if (light_intensity > 1.0f) light_intensity = 1.0f;


				// Outline Logic
				float u, v;
				if (side == 0) {
					u = hit_y - floorf(hit_y);
					v = hit_z - floorf(hit_z);
				} else if (side == 1) {
					u = hit_x - floorf(hit_x);
					v = hit_z - floorf(hit_z);
				} else {
					u = hit_x - floorf(hit_x);
					v = hit_y - floorf(hit_y);
				}

				float margin = 1.0f / 16.0f;
				int is_edge = 0;
				if (u < margin || u > 1.0f - margin || v < margin ||
				    v > 1.0f - margin) {
					is_edge = 1;
				}

				switch (block) {
				case 1:
					color = COL_FLOOR;
					break;
				case 2:
					color = COL_WALL;
					break;
				case 3:
					color = COL_BLOCK_1;
					// White blocks are self-illuminated
					light_intensity = 1.0f;
					break;
				case 4:
					color = COL_BLOCK_2;
					break;
				case 5:
					color = COL_BLOCK_3;
					break;
				default:
					color = COL_WALL;
					break;
				}

				if (is_edge) {
					color = COL_BLACK;
				}

				// Apply Lighting (White)
				// Base lighting (ambient) - Dark Room
				float ambient = 0.0f; // Pitch black ambient
				float total_light = ambient + light_intensity;
				if (total_light > 1.0f) total_light = 1.0f;

				// Mix original color with White light
				uint8_t r = rgb565_red(color);
				uint8_t g = rgb565_green(color);
				uint8_t b = rgb565_blue(color);

				// Modulative Lighting: SurfaceColor * (Ambient + Intensity)
				// This preserves black outlines (0 * anything = 0)
				float intensity_factor = ambient + light_intensity;

				float lit_r = r * intensity_factor;
				float lit_g = g * intensity_factor;
				float lit_b = b * intensity_factor;

				// Use original shading as a modulation factor if needed
				// For now, let's keep the simple side shading
				float shade = 1.0f;
				if (side == 2 && stepZ > 0) shade = 0.5f;
				else if (side == 0) shade = 0.8f;
				else if (side == 1) shade = 0.7f;

				// Apply directional shading to the result?
				lit_r *= shade;
				lit_g *= shade;
				lit_b *= shade;

				// Dithering (Ordered Bayer 2x2)
				// Coordinates: x, y
				// Matrix: [ 0  2 ]
				//         [ 3  1 ]
				// Scale factor: 4
				int dither_val = 0;
				if ((x % 2 == 0) && (y % 2 == 0)) dither_val = 0;
				else if ((x % 2 != 0) && (y % 2 == 0)) dither_val = 2;
				else if ((x % 2 == 0) && (y % 2 != 0)) dither_val = 3;
				else dither_val = 1;

				// Map 0..3 to a small range, e.g., -4..4 or similar, to affect the LSBs
				float dither_offset = (dither_val - 1.5f) * 4.0f;

				lit_r += dither_offset;
				lit_g += dither_offset;
				lit_b += dither_offset;

				// Clamp
				if (lit_r > 255) lit_r = 255;
				if (lit_g > 255) lit_g = 255;
				if (lit_b > 255) lit_b = 255;
				if (lit_r < 0) lit_r = 0;
				if (lit_g < 0) lit_g = 0;
				if (lit_b < 0) lit_b = 0;

				color = rgb_to_rgb565((int)lit_r, (int)lit_g, (int)lit_b);

				tft_draw_pixel(x, y, color);
			} else {
				// Dark Sky for dark room
				tft_draw_pixel(x, y, COL_SKY);
			}

			// Draw Crosshair (in center pixels)
			if (x == SCREEN_W / 2 && y == SCREEN_H / 2) {
				tft_draw_pixel(x, y, rgb_to_rgb565(255, 255, 255));
			}
		}
	}

	// UI Overlay
	char buf[32];
	tft_draw_string(5, 5, rgb_to_rgb565(255, 255, 255), "Mode: %s",
			(current_mode == MODE_MOVE_LOOK) ? "MOVE/LOOK" : "EDIT");
	if (current_mode == MODE_EDIT) {
		tft_draw_string(5, 15, rgb_to_rgb565(255, 255, 255), "Block: %d",
				selected_block_type);
		tft_draw_string(5, 105, rgb_to_rgb565(200, 200, 200), "A: Place, B: Break");
	} else {
		tft_draw_string(5, 105, rgb_to_rgb565(200, 200, 200), "A/B: Fly, Y/X: Look");
	}
}

void game_audio(int nsamples)
{
	(void)nsamples;
}

void game_inbox(sdk_message_t msg)
{
	(void)msg;
}
int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = 0,
	};
	sdk_main(&config);
}
