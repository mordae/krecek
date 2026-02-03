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
#define COL_SKY     rgb_to_rgb565(135, 206, 235)
#define COL_FLOOR   rgb_to_rgb565(50, 50, 50)
#define COL_WALL    rgb_to_rgb565(180, 180, 180)
#define COL_BLOCK_R rgb_to_rgb565(200, 50, 50)
#define COL_BLOCK_G rgb_to_rgb565(50, 200, 50)
#define COL_BLOCK_B rgb_to_rgb565(50, 50, 200)

// Map and Player
static uint8_t map[MAP_W][MAP_H][MAP_D];

struct {
    float x, y, z;
    float yaw, pitch;
} player;

// Game Info
sdk_game_info("tester-3d", NULL);

// Initialize Map
static void init_map() {
    for (int x = 0; x < MAP_W; x++) {
        for (int y = 0; y < MAP_H; y++) {
            for (int z = 0; z < MAP_D; z++) {
                if (z == 0) {
                    map[x][y][z] = 1; // Floor
                } else if (x == 0 || x == MAP_W - 1 || y == 0 || y == MAP_H - 1) {
                    map[x][y][z] = 2; // Wall
                } else if ((x % 5 == 0 && y % 5 == 0) && z < 3) {
                     map[x][y][z] = 3; // Pillars
                } else if (rand() % 40 == 0 && z > 0) {
                     map[x][y][z] = 4 + (rand() % 3); // Random Blocks
                } else {
                    map[x][y][z] = 0; // Air
                }
            }
        }
    }
}

void game_start(void) {
    init_map();
    player.x = MAP_W / 2.0f;
    player.y = MAP_H / 2.0f;
    player.z = 1.5f; // Eye height (Floor is z=0 block, so on top of it is z=1.0 to 2.0)
    player.yaw = 0.0f;
    player.pitch = 0.0f;

    // Set backlight to standard
    sdk_set_backlight(SDK_BACKLIGHT_STD);
}

void game_reset(void) {
    game_start();
}

void game_input(unsigned dt_usec) {
    float dt = dt_usec / 1000000.0f;
    float move_speed = 5.0f;
    float rot_speed = 2.0f;

    // Joystick Y - Move Forward/Back
    if (abs(sdk_inputs.joy_y) > 200) {
        float move = -(sdk_inputs.joy_y / 2048.0f) * move_speed * dt;
        float new_x = player.x + cosf(player.yaw) * move;
        float new_y = player.y + sinf(player.yaw) * move;

        // Simple collision check (only check floor/walls at head height? nah just bounds for now)
        if (new_x > 0 && new_x < MAP_W) player.x = new_x;
        if (new_y > 0 && new_y < MAP_H) player.y = new_y;
    }

    // Joystick X - Yaw
    if (abs(sdk_inputs.joy_x) > 200) {
        float rot = (sdk_inputs.joy_x / 2048.0f) * rot_speed * dt;
        player.yaw += rot;
    }

    // Buttons for Pitch
    if (sdk_inputs.a) { // Look Up
        player.pitch += rot_speed * dt;
    }
    if (sdk_inputs.y) { // Look Down
        player.pitch -= rot_speed * dt;
    }

    // Clamp Pitch (-89 to 89 degrees)
    float max_pitch = 89.0f * M_PI / 180.0f;
    if (player.pitch > max_pitch) player.pitch = max_pitch;
    if (player.pitch < -max_pitch) player.pitch = -max_pitch;
}

void game_paint(unsigned dt_usec) {
    (void)dt_usec;
    tft_fill(COL_SKY);

    // Camera Vectors
    float cy = cosf(player.yaw);
    float sy = sinf(player.yaw);
    float cp = cosf(player.pitch);
    float sp = sinf(player.pitch);

    // Forward (F)
    float fx = cy * cp;
    float fy = sy * cp;
    float fz = sp;

    // Right (R) - Horizontal
    float rx = sy;
    float ry = -cy;
    float rz = 0.0f;

    // Up (U) = R x F
    float ux = ry * fz - rz * fy; // -cy * sp
    float uy = rz * fx - rx * fz; // -sy * sp
    float uz = rx * fy - ry * fx; // sy*sy*cp - (-cy*cy*cp) = cp

    // Precalculate aspect ratio factor
    float aspect = (float)SCREEN_W / SCREEN_H;
    float tan_fov = TAN_HALF_FOV;

    // Raycast per pixel
    for (int y = 0; y < SCREEN_H; y++) {
        // Optimization: Precalculate Y-dependent camera coord
        float ndc_y = 1.0f - (2.0f * y / SCREEN_H);
        float cam_y = ndc_y * tan_fov;

        float ray_part_x = ux * cam_y;
        float ray_part_y = uy * cam_y;
        float ray_part_z = uz * cam_y;

        for (int x = 0; x < SCREEN_W; x++) {
            float ndc_x = (2.0f * x / SCREEN_W) - 1.0f;
            float cam_x = ndc_x * tan_fov * aspect;

            // Ray Direction
            float dx = fx + rx * cam_x + ray_part_x;
            float dy = fy + ry * cam_x + ray_part_y;
            float dz = fz + rz * cam_x + ray_part_z;

            // Normalize Ray
            float len = sqrtf(dx*dx + dy*dy + dz*dz);
            dx /= len; dy /= len; dz /= len;

            // Map Position
            int mapX = (int)player.x;
            int mapY = (int)player.y;
            int mapZ = (int)player.z;

            // Delta Distance
            float deltaDistX = (dx == 0) ? 1e30f : fabsf(1.0f / dx);
            float deltaDistY = (dy == 0) ? 1e30f : fabsf(1.0f / dy);
            float deltaDistZ = (dz == 0) ? 1e30f : fabsf(1.0f / dz);

            // Step and SideDist
            int stepX, stepY, stepZ;
            float sideDistX, sideDistY, sideDistZ;

            if (dx < 0) {
                stepX = -1;
                sideDistX = (player.x - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
            }
            if (dy < 0) {
                stepY = -1;
                sideDistY = (player.y - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
            }
            if (dz < 0) {
                stepZ = -1;
                sideDistZ = (player.z - mapZ) * deltaDistZ;
            } else {
                stepZ = 1;
                sideDistZ = (mapZ + 1.0f - player.z) * deltaDistZ;
            }

            // DDA Loop
            int hit = 0;
            int side = -1; // 0=X, 1=Y, 2=Z
            int steps = 0;

            while (hit == 0 && steps < 50) {
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
                if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H || mapZ < 0 || mapZ >= MAP_D) {
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

                switch (block) {
                    case 1: color = COL_FLOOR; break;
                    case 2: color = COL_WALL; break;
                    case 3: color = COL_BLOCK_R; break;
                    case 4: color = COL_BLOCK_G; break;
                    case 5: color = COL_BLOCK_B; break;
                    default: color = COL_WALL; break;
                }

                // Shading based on side
                if (side == 0) {
                    // X side - Normal
                } else if (side == 1) {
                    // Y side - Darker
                    color = rgb_to_rgb565(
                        (rgb565_red(color) * 200) / 255,
                        (rgb565_green(color) * 200) / 255,
                        (rgb565_blue(color) * 200) / 255
                    );
                } else {
                    // Z side (Top/Bottom) - Darkest or Brightest?
                    // Usually Top is bright, Bottom is dark?
                    // Let's make Z side distinct.
                     color = rgb_to_rgb565(
                        (rgb565_red(color) * 150) / 255,
                        (rgb565_green(color) * 150) / 255,
                        (rgb565_blue(color) * 150) / 255
                    );
                }

                tft_draw_pixel(x, y, color);
            }
        }
    }
}

void game_audio(int nsamples) {
    (void)nsamples;
}

void game_inbox(sdk_message_t msg) {
    (void)msg;
}
