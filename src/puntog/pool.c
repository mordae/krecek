#include "pool.h"
#include <string.h>
#include <stdlib.h>

// Zombie pool storage & free-list pointer stack
static Zombie zombie_storage[POOL_SIZE_ZOMBIES];
static Zombie *zombie_free_stack[POOL_SIZE_ZOMBIES];
static int zombie_free_top = 0;

Zombie *active_zombies[MAX_ACTIVE_ZOMBIES];
int active_zombie_count = 0;

// Projectile pool storage & free-list pointer stack
static Projectile projectile_storage[POOL_SIZE_PROJECTILES];
static Projectile *projectile_free_stack[POOL_SIZE_PROJECTILES];
static int projectile_free_top = 0;

Projectile *active_projectiles[MAX_ACTIVE_PROJECTILES];
int active_projectile_count = 0;

// Particle pool storage & free-list pointer stack
static Particle particle_storage[POOL_SIZE_PARTICLES];
static Particle *particle_free_stack[POOL_SIZE_PARTICLES];
static int particle_free_top = 0;

Particle *active_particles[MAX_ACTIVE_PARTICLES];
int active_particle_count = 0;

// Pickups and Traps static arrays
Pickup active_pickups[MAX_ACTIVE_PICKUPS];
int active_pickup_count = 0;

Trap active_traps[MAX_ACTIVE_TRAPS];
int active_trap_count = 0;

void pools_init(void)
{
	// Initialize Zombie Pool
	zombie_free_top = 0;
	active_zombie_count = 0;
	for (int i = 0; i < POOL_SIZE_ZOMBIES; i++) {
		zombie_storage[i].active = false;
		zombie_storage[i].pool_index = i;
		zombie_free_stack[zombie_free_top++] = &zombie_storage[i];
	}

	// Initialize Projectile Pool
	projectile_free_top = 0;
	active_projectile_count = 0;
	for (int i = 0; i < POOL_SIZE_PROJECTILES; i++) {
		projectile_storage[i].active = false;
		projectile_storage[i].pool_index = i;
		projectile_free_stack[projectile_free_top++] = &projectile_storage[i];
	}

	// Initialize Particle Pool
	particle_free_top = 0;
	active_particle_count = 0;
	for (int i = 0; i < POOL_SIZE_PARTICLES; i++) {
		particle_storage[i].active = false;
		particle_storage[i].pool_index = i;
		particle_free_stack[particle_free_top++] = &particle_storage[i];
	}

	active_pickup_count = 0;
	for (int i = 0; i < MAX_ACTIVE_PICKUPS; i++) {
		active_pickups[i].active = false;
	}

	active_trap_count = 0;
	for (int i = 0; i < MAX_ACTIVE_TRAPS; i++) {
		active_traps[i].active = false;
	}
}

void clear_all_entities(void)
{
	while (active_zombie_count > 0) {
		zombie_free(active_zombies[0]);
	}
	while (active_projectile_count > 0) {
		projectile_free(active_projectiles[0]);
	}
	while (active_particle_count > 0) {
		particle_free(active_particles[0]);
	}
	active_pickup_count = 0;
	active_trap_count = 0;
}

Zombie *zombie_alloc(void)
{
	// Enforce hard cap on active zombies
	if (active_zombie_count >= MAX_ACTIVE_ZOMBIES || zombie_free_top <= 0) {
		return NULL;
	}

	Zombie *z = zombie_free_stack[--zombie_free_top];
	int p_idx = z->pool_index;
	memset(z, 0, sizeof(Zombie));
	z->pool_index = p_idx;
	z->active = true;

	active_zombies[active_zombie_count++] = z;
	return z;
}

void zombie_free(Zombie *z)
{
	if (!z || !z->active)
		return;

	// Locate in active tracking array and remove in O(1) by swapping with end
	for (int i = 0; i < active_zombie_count; i++) {
		if (active_zombies[i] == z) {
			active_zombies[i] = active_zombies[active_zombie_count - 1];
			active_zombie_count--;
			break;
		}
	}

	z->active = false;
	zombie_free_stack[zombie_free_top++] = z;
}

Projectile *projectile_alloc(void)
{
	if (active_projectile_count >= MAX_ACTIVE_PROJECTILES || projectile_free_top <= 0) {
		return NULL;
	}

	Projectile *p = projectile_free_stack[--projectile_free_top];
	int p_idx = p->pool_index;
	memset(p, 0, sizeof(Projectile));
	p->pool_index = p_idx;
	p->active = true;

	active_projectiles[active_projectile_count++] = p;
	return p;
}

void projectile_free(Projectile *p)
{
	if (!p || !p->active)
		return;

	for (int i = 0; i < active_projectile_count; i++) {
		if (active_projectiles[i] == p) {
			active_projectiles[i] = active_projectiles[active_projectile_count - 1];
			active_projectile_count--;
			break;
		}
	}

	p->active = false;
	projectile_free_stack[projectile_free_top++] = p;
}

Particle *particle_alloc(void)
{
	if (active_particle_count >= MAX_ACTIVE_PARTICLES || particle_free_top <= 0) {
		return NULL;
	}

	Particle *pt = particle_free_stack[--particle_free_top];
	int p_idx = pt->pool_index;
	memset(pt, 0, sizeof(Particle));
	pt->pool_index = p_idx;
	pt->active = true;

	active_particles[active_particle_count++] = pt;
	return pt;
}

void particle_free(Particle *pt)
{
	if (!pt || !pt->active)
		return;

	for (int i = 0; i < active_particle_count; i++) {
		if (active_particles[i] == pt) {
			active_particles[i] = active_particles[active_particle_count - 1];
			active_particle_count--;
			break;
		}
	}

	pt->active = false;
	particle_free_stack[particle_free_top++] = pt;
}

void spawn_particles(float x, float y, int count, color_t color, float speed)
{
	for (int i = 0; i < count; i++) {
		Particle *pt = particle_alloc();
		if (!pt)
			break;

		float angle = ((float)rand() / (float)RAND_MAX) * 6.283185f;
		float spd = speed * (0.4f + 0.6f * ((float)rand() / (float)RAND_MAX));

		pt->x = x;
		pt->y = y;
		pt->vx = cosf(angle) * spd;
		pt->vy = sinf(angle) * spd;
		pt->max_life = 0.25f + 0.35f * ((float)rand() / (float)RAND_MAX);
		pt->life = pt->max_life;
		pt->color = color;
		pt->size = 1 + (rand() % 2);
	}
}

Pickup *pickup_spawn(float x, float y, int type, int value)
{
	if (active_pickup_count >= MAX_ACTIVE_PICKUPS)
		return NULL;

	Pickup *pk = &active_pickups[active_pickup_count++];
	pk->x = x;
	pk->y = y;
	pk->type = type;
	pk->value = value;
	pk->life = 25.0f; // despawn after 25s
	pk->active = true;
	return pk;
}

void pickup_remove(int idx)
{
	if (idx < 0 || idx >= active_pickup_count)
		return;
	active_pickups[idx] = active_pickups[active_pickup_count - 1];
	active_pickup_count--;
}

Trap *trap_spawn(float x, float y)
{
	if (active_trap_count >= MAX_ACTIVE_TRAPS)
		return NULL;

	Trap *tr = &active_traps[active_trap_count++];
	tr->x = x;
	tr->y = y;
	tr->arm_timer = 0.4f;
	tr->armed = false;
	tr->active = true;
	return tr;
}

void trap_remove(int idx)
{
	if (idx < 0 || idx >= active_trap_count)
		return;
	active_traps[idx] = active_traps[active_trap_count - 1];
	active_trap_count--;
}
