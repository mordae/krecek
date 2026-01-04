#ifndef COMMON_H
#define COMMON_H
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#define FOV_DEGREES 60.0f
#define FOV_RADIANS (FOV_DEGREES * (float)M_PI / 180.0f)
#define TILE_SIZE 64.0f
#define PROJECTION_PLANE_DISTANCE ((SCREEN_WIDTH / 2.0f) / tanf(FOV_RADIANS / 2.0f))
#define MOVE_SPEED 225.0f
#define ROTATE_SPEED 3.0f

// Increased weapon damage
#define PISTOL_DAMAGE 35
#define CHAINSAW_DAMAGE 10
#define SHOTGUN_DAMAGE 60 // Increased to kill ENEMY1 in one hit
#define N_GUNS 3

#define PISTOL_MAX_RANGE_TILES 15.0f
#define SHOTGUN_MAX_RANGE_TILES 5.0f
#define CHAINSAW_MAX_RANGE_TILES 1.5f
#define BULLET_VISUAL_DURATION_MS 100
#define SHOTGUN_PELLETS 5

#define SHOTGUN_SPREAD_DEGREES 25.0f

// Adjusted enemy stats for easier killing
#define MAX_ENEMIES 16
#define ENEMY_SPEED 50.0f
#define ENEMY_DAMAGE 10
#define ENEMY_ATTACK_RANGE 1.5f * TILE_SIZE
#define ENEMY_DETECTION_RANGE 5.0f * TILE_SIZE
#define ENEMY_ATTACK_COOLDOWN 1.0f
#define ENEMY_HEALTH 60 // ENEMY1 health (killed by 1 shotgun hit of 60 damage)
// ENEMY2 will have ENEMY_HEALTH * 2 = 120 health (killed by 2 shotgun hits)

// Pickup values
#define HEALTH_PACK_HEAL 25
#define AMMO_BOX_AMOUNT 10
#define MAX_HEALTH 100
#define MAX_AMMO 60
#define AMMO_SHOTBOX 5
#define MAX_PICKUPS 64

// Colors
#define DRAW_RED rgb_to_rgb565(183, 0, 0)
#define RED rgb_to_rgb565(183, 0, 0)
#define WHITE rgb_to_rgb565(255, 255, 255)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GRAY rgb_to_rgb565(60, 60, 60)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define ORANGE rgb_to_rgb565(255, 165, 0)
#define CYAN rgb_to_rgb565(0, 255, 255)

// Muzzle flash
#define MUZZLE_FLASH_DURATION_MS 50

#define FIXED_SHIFT 16
#define FIXED_SCALE (1 << FIXED_SHIFT)

#define float_to_fixed(x) ((fixed_t)((x) * FIXED_SCALE))
#define fixed_to_float(x) ((x) / (float)FIXED_SCALE)
#define fixed_mul(a, b) ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define fixed_div(a, b) ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))

int seed = 11565;
float volume = 0.5f;
float timer = 0;
float footstep_timer = 0;

#include <stdint.h>
#endif
