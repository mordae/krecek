#pragma once
#include "common.h"

typedef struct Zombie {
	float x, y;
	float vx, vy;
	float speed;
	int hp, max_hp;
	float burn_timer;
	float burn_tick;
	float freeze_timer;
	float knockback_x, knockback_y;
	float anim_timer;
	int anim_frame;
	int type; // 0=Walker, 1=Runner, 2=Brute
	bool active;
	int pool_index;
} Zombie;

typedef struct Projectile {
	float x, y;
	float vx, vy;
	SpellType spell_type;
	float life;
	int damage;
	int pierces_left;
	int target_zombie_idx;
	float homing_strength;
	bool active;
	int pool_index;
} Projectile;

typedef struct Particle {
	float x, y;
	float vx, vy;
	float life;
	float max_life;
	color_t color;
	uint8_t size;
	bool active;
	int pool_index;
} Particle;

typedef struct Pickup {
	float x, y;
	int type; // 0=Coin, 1=Banana, 2=Heart
	int value;
	float life;
	bool active;
} Pickup;

typedef struct Trap {
	float x, y;
	float arm_timer;
	bool armed;
	bool active;
} Trap;

// Active tracking arrays & counts
extern Zombie *active_zombies[MAX_ACTIVE_ZOMBIES];
extern int active_zombie_count;

extern Projectile *active_projectiles[MAX_ACTIVE_PROJECTILES];
extern int active_projectile_count;

extern Particle *active_particles[MAX_ACTIVE_PARTICLES];
extern int active_particle_count;

extern Pickup active_pickups[MAX_ACTIVE_PICKUPS];
extern int active_pickup_count;

extern Trap active_traps[MAX_ACTIVE_TRAPS];
extern int active_trap_count;

// Pool lifecycle
void pools_init(void);
void clear_all_entities(void);

// Custom pointer pool allocation / free functions
Zombie *zombie_alloc(void);
void zombie_free(Zombie *z);

Projectile *projectile_alloc(void);
void projectile_free(Projectile *p);

Particle *particle_alloc(void);
void particle_free(Particle *pt);
void spawn_particles(float x, float y, int count, color_t color, float speed);

Pickup *pickup_spawn(float x, float y, int type, int value);
void pickup_remove(int idx);

Trap *trap_spawn(float x, float y);
void trap_remove(int idx);
