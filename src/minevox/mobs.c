#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * ===========================================================================
 * Mobs
 * ===========================================================================
 *
 * Creatures are drawn as flat sprites that always turn to face the camera,
 * the trick Doom used in 1993. A real cube-built mob would need its own
 * geometry pass; a sprite is one texture lookup per pixel and, at the size
 * a pig occupies on a 160x120 screen, you cannot tell the difference.
 *
 * Their behaviour is a small state machine with no pathfinding at all.
 * Wander in a direction for a few seconds, pick a new one, and if you are
 * hostile and the player is close, walk at them instead. That is enough to
 * feel alive, and it costs almost nothing per frame.
 */

/* clang-format off */
const struct mob_info mob_info[NUM_MOB_TYPES] = {
	/*                  name        hostile hp dmg nether speed height drop            n */
	[MOB_PIG]      = { "Pig",       0, 10, 0, 0, 1.2f, 0.9f, ITEM_PORKCHOP,   2 },
	[MOB_SHEEP]    = { "Sheep",     0,  8, 0, 0, 1.1f, 1.1f, 0,               0 },
	[MOB_COW]      = { "Cow",       0, 10, 0, 0, 1.0f, 1.2f, ITEM_PORKCHOP,   3 },
	[MOB_ZOMBIE]   = { "Zombie",    1, 20, 3, 0, 1.7f, 1.8f, 0,               0 },
	[MOB_SKELETON] = { "Skeleton",  1, 16, 2, 0, 1.9f, 1.8f, ITEM_COAL,       1 },
	[MOB_PIGMAN]   = { "Pigman",    1, 20, 4, 1, 1.8f, 1.8f, ITEM_GOLD_INGOT, 1 },
};
/* clang-format on */

/*
 * The pool. Twenty is plenty: the world is 48 blocks across and anything
 * further than the fog is invisible anyway, so more would only cost time.
 */
#define MAX_MOBS 20

static struct mem_pool mob_pool;
struct mob *mobs;

color_t mob_sprite[NUM_MOB_TYPES][MOB_SPRITE * MOB_SPRITE];

/* How many hostile mobs we allow at once, so nights stay survivable. */
#define MAX_HOSTILE 6

static float spawn_timer;

/*
 * ---------------------------------------------------------------------------
 * Sprites
 * ---------------------------------------------------------------------------
 *
 * Drawn with rectangles, because a mob made of cubes is mostly rectangles
 * anyway. TRANSPARENT is the colour the renderer treats as see through.
 */
static void spr_clear(int type)
{
	for (int i = 0; i < MOB_SPRITE * MOB_SPRITE; i++)
		mob_sprite[type][i] = TRANSPARENT;
}

static void spr_rect(int type, int x0, int y0, int x1, int y1, color_t color)
{
	x0 = clampi(x0, 0, MOB_SPRITE - 1);
	x1 = clampi(x1, 0, MOB_SPRITE - 1);
	y0 = clampi(y0, 0, MOB_SPRITE - 1);
	y1 = clampi(y1, 0, MOB_SPRITE - 1);

	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			mob_sprite[type][y * MOB_SPRITE + x] = color;
}

/* A four legged animal, seen from the side. */
static void make_quadruped(int type, color_t body, color_t head, color_t leg, color_t detail)
{
	spr_clear(type);

	spr_rect(type, 2, 5, 12, 11, body);   /* Barrel of the body. */
	spr_rect(type, 11, 3, 15, 9, head);   /* Head, held a little higher. */
	spr_rect(type, 13, 6, 15, 8, detail); /* Snout or muzzle. */
	spr_rect(type, 12, 4, 13, 4, leg);    /* Ear. */
	spr_rect(type, 3, 12, 4, 15, leg);    /* Front leg. */
	spr_rect(type, 9, 12, 10, 15, leg);   /* Back leg. */
	spr_rect(type, 1, 5, 1, 7, detail);   /* Tail. */

	/* One dark pixel for an eye does an enormous amount of work. */
	spr_rect(type, 13, 5, 13, 5, rgb_to_rgb565(20, 20, 20));
}

/* A person shaped mob, seen from the front. */
static void make_humanoid(int type, color_t skin, color_t shirt, color_t legs, color_t eyes)
{
	spr_clear(type);

	spr_rect(type, 5, 0, 10, 5, skin); /* Head. */
	spr_rect(type, 6, 2, 6, 3, eyes);
	spr_rect(type, 9, 2, 9, 3, eyes);
	spr_rect(type, 5, 6, 10, 11, shirt); /* Torso. */
	spr_rect(type, 3, 6, 4, 11, skin);   /* Arms, held out in front. */
	spr_rect(type, 11, 6, 12, 11, skin);
	spr_rect(type, 5, 12, 7, 15, legs);
	spr_rect(type, 8, 12, 10, 15, legs);
}

static void mobs_build_sprites(void)
{
	make_quadruped(MOB_PIG, rgb_to_rgb565(232, 146, 148), rgb_to_rgb565(232, 146, 148),
		       rgb_to_rgb565(198, 112, 116), rgb_to_rgb565(210, 124, 128));

	make_quadruped(MOB_SHEEP, rgb_to_rgb565(232, 232, 226), rgb_to_rgb565(226, 226, 220),
		       rgb_to_rgb565(78, 74, 70), rgb_to_rgb565(212, 210, 204));

	make_quadruped(MOB_COW, rgb_to_rgb565(74, 58, 48), rgb_to_rgb565(60, 48, 40),
		       rgb_to_rgb565(48, 38, 32), rgb_to_rgb565(226, 226, 220));

	make_humanoid(MOB_ZOMBIE, rgb_to_rgb565(96, 148, 84), rgb_to_rgb565(52, 116, 128),
		      rgb_to_rgb565(48, 62, 118), rgb_to_rgb565(24, 40, 24));

	make_humanoid(MOB_SKELETON, rgb_to_rgb565(222, 222, 214), rgb_to_rgb565(206, 206, 198),
		      rgb_to_rgb565(196, 196, 188), rgb_to_rgb565(20, 20, 20));

	make_humanoid(MOB_PIGMAN, rgb_to_rgb565(226, 140, 138), rgb_to_rgb565(126, 158, 96),
		      rgb_to_rgb565(198, 112, 116), rgb_to_rgb565(30, 20, 20));

	/* Give the cow its patches, which is what tells it from a pig. */
	spr_rect(MOB_COW, 4, 6, 6, 8, rgb_to_rgb565(226, 226, 220));
	spr_rect(MOB_COW, 8, 9, 10, 11, rgb_to_rgb565(226, 226, 220));

	/* And the pigman a gold sword, so you know it means business. */
	spr_rect(MOB_PIGMAN, 13, 5, 13, 10, rgb_to_rgb565(238, 206, 92));
}

void mobs_init(void)
{
	mem_pool_init(&mob_pool, "mobs", sizeof(struct mob), MAX_MOBS);
	mobs = NULL;
	mobs_build_sprites();
}

void mobs_reset(void)
{
	while (mobs) {
		struct mob *next = mobs->next;
		mem_pool_free(&mob_pool, mobs);
		mobs = next;
	}

	spawn_timer = 0.0f;
}

int mobs_live_count(void)
{
	return mem_pool_used(&mob_pool);
}

struct mob *mobs_spawn(int type, float x, float y, float z)
{
	if (type < 0 || type >= NUM_MOB_TYPES)
		return NULL;

	struct mob *m = mem_pool_alloc(&mob_pool);

	if (!m)
		return NULL;

	m->type = type;
	m->health = mob_info[type].max_health;
	m->x = x;
	m->y = y;
	m->z = z;
	m->yaw = (sdk_random() % 628) / 100.0f;
	m->think = 1.0f + (sdk_random() % 200) / 100.0f;

	m->next = mobs;
	mobs = m;
	return m;
}

/*
 * ---------------------------------------------------------------------------
 * Physics
 * ---------------------------------------------------------------------------
 *
 * Mobs use a simpler collision model than the player: a single column
 * check rather than a full box. They are not precise enough for anyone to
 * notice, and it keeps twenty of them cheap.
 */
static bool mob_blocked(const struct mob *m, float x, float y, float z)
{
	float h = mob_info[m->type].height;

	for (float dz = 0.1f; dz < h; dz += 0.8f) {
		int bx = (int)floorf(x);
		int by = (int)floorf(y);
		int bz = (int)floorf(z + dz);

		if (!world_inside(bx, by, bz))
			return true;

		if (block_is_solid(world_get_fast(bx, by, bz)))
			return true;
	}

	return false;
}

static void mob_move(struct mob *m, float dt)
{
	/* Gravity first, then try each axis on its own. */
	m->vz -= 22.0f * dt;
	m->vz = clampf(m->vz, -30.0f, 12.0f);

	float nz = m->z + m->vz * dt;

	if (mob_blocked(m, m->x, m->y, nz)) {
		if (m->vz < 0.0f)
			m->on_ground = true;

		m->vz = 0.0f;
	} else {
		m->z = nz;
		m->on_ground = false;
	}

	float nx = m->x + m->vx * dt;
	float ny = m->y + m->vy * dt;

	if (!mob_blocked(m, nx, m->y, m->z)) {
		m->x = nx;
	} else if (m->on_ground && !mob_blocked(m, nx, m->y, m->z + 1.05f)) {
		/* Step up a single block, the same courtesy the player gets. */
		m->x = nx;
		m->z += 1.05f;
	} else {
		m->vx = 0.0f;
		m->think = 0.0f; /* Blocked: pick a new direction now. */
	}

	if (!mob_blocked(m, m->x, ny, m->z)) {
		m->y = ny;
	} else if (m->on_ground && !mob_blocked(m, m->x, ny, m->z + 1.05f)) {
		m->y = ny;
		m->z += 1.05f;
	} else {
		m->vy = 0.0f;
		m->think = 0.0f;
	}

	/* Never let one fall out of the world. */
	if (m->z < 0.5f) {
		m->z = 0.5f;
		m->vz = 0.0f;
		m->on_ground = true;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Behaviour
 * ---------------------------------------------------------------------------
 */
static bool is_night(void)
{
	/* Matches the daylight curve in render.c: dark for a third of the day. */
	float sun = sinf(world_time / DAY_LENGTH * 2.0f * (float)M_PI);
	return sun < -0.05f;
}

static void mob_think(struct mob *m, float dt)
{
	const struct mob_info *info = &mob_info[m->type];

	if (m->hurt > 0.0f)
		m->hurt -= dt;

	if (m->attack > 0.0f)
		m->attack -= dt;

	float dx = player.x - m->x;
	float dy = player.y - m->y;
	float dz = (player.z + 0.9f) - (m->z + info->height * 0.5f);
	float dist2 = dx * dx + dy * dy + dz * dz;

	bool chasing = false;

	if (info->hostile && dist2 < 14.0f * 14.0f && survival_mode) {
		chasing = true;
		m->yaw = atan2f(dy, dx);

		/* Close enough to swing, if the cooldown has expired. */
		if (dist2 < 1.7f * 1.7f && m->attack <= 0.0f) {
			player_hurt(info->damage);
			m->attack = 1.1f;

			/* Shove the player back a little, so hits are readable. */
			float dist = sqrtf(dist2);

			if (dist > 0.001f) {
				player.vx += dx / dist * 3.0f;
				player.vy += dy / dist * 3.0f;
			}
		}
	} else {
		/* Wander: hold a heading for a few seconds, then change. */
		m->think -= dt;

		if (m->think <= 0.0f) {
			m->think = 2.0f + (sdk_random() % 300) / 100.0f;
			m->yaw = (sdk_random() % 628) / 100.0f;

			/* Stand still now and then, it looks far more natural. */
			if (sdk_random() % 3 == 0)
				m->yaw = INFINITY;
		}
	}

	if (isfinite(m->yaw)) {
		float speed = info->speed * (chasing ? 1.25f : 0.55f);
		m->vx = cosf(m->yaw) * speed;
		m->vy = sinf(m->yaw) * speed;
		m->bob += dt * 6.0f;
	} else {
		m->vx = 0.0f;
		m->vy = 0.0f;
	}
}

/*
 * Spawn hostiles at night and friendly animals during the day, both a
 * respectful distance from the player so nothing appears in their face.
 */
static void spawn_wave(void)
{
	int hostile = 0;

	for (struct mob *m = mobs; m; m = m->next)
		if (mob_info[m->type].hostile)
			hostile++;

	bool nether = nether_state == NETHER_ACTIVE;
	bool want_hostile = survival_mode && (nether || is_night());

	if (want_hostile && hostile >= MAX_HOSTILE)
		return;

	if (mobs_live_count() >= MAX_MOBS - 1)
		return;

	/*
	 * The island is only about twenty blocks of dry land across, so the
	 * band we are allowed to spawn in is narrow and a random guess
	 * often lands in the sea. Try a good many times before giving up.
	 */
	for (int tries = 0; tries < 60; tries++) {
		int x = sdk_random() % WORLD_X;
		int y = sdk_random() % WORLD_Y;

		float dx = x - player.x, dy = y - player.y;
		float dist2 = dx * dx + dy * dy;

		/* Not on top of the player, not so far it never matters. */
		if (dist2 < 7.0f * 7.0f || dist2 > 26.0f * 26.0f)
			continue;

		int z = world_height[x][y];

		if (z <= SEA_LEVEL + 1 || z + 2 >= WORLD_Z)
			continue;

		uint8_t ground = world_get(x, y, z - 1);

		if (ground != BLOCK_GRASS && ground != BLOCK_SAND && ground != BLOCK_NETHERRACK)
			continue;

		int type;

		if (nether)
			type = MOB_PIGMAN;
		else if (want_hostile)
			type = (sdk_random() & 1) ? MOB_ZOMBIE : MOB_SKELETON;
		else
			type = sdk_random() % 3; /* Pig, sheep or cow. */

		mobs_spawn(type, x + 0.5f, y + 0.5f, z);
		return;
	}
}

void mobs_update(float dt)
{
	spawn_timer -= dt;

	if (spawn_timer <= 0.0f) {
		spawn_timer = 3.0f;
		spawn_wave();
	}

	struct mob **link = &mobs;

	while (*link) {
		struct mob *m = *link;

		/*
		 * Hostiles that survive into the morning burn away, which is
		 * both traditional and a tidy way to keep the pool from
		 * silting up with mobs the player walked away from.
		 */
		bool doomed = false;

		if (mob_info[m->type].hostile && !mob_info[m->type].nether && !is_night())
			doomed = true;

		if (mob_info[m->type].nether && nether_state == NETHER_IDLE)
			doomed = true;

		if (doomed) {
			float dx = m->x - player.x, dy = m->y - player.y;

			/* Only out of sight, so nothing vanishes in front of you. */
			if (dx * dx + dy * dy > 14.0f * 14.0f) {
				*link = m->next;
				mem_pool_free(&mob_pool, m);
				continue;
			}
		}

		mob_think(m, dt);
		mob_move(m, dt);

		/* Lava and drowning are hazards for mobs too. */
		uint8_t inside =
			world_get((int)floorf(m->x), (int)floorf(m->y), (int)floorf(m->z + 0.5f));

		if (inside == BLOCK_LAVA && m->hurt <= 0.0f) {
			if (mobs_hurt(m, 4, 0.0f, 0.0f)) {
				*link = m->next;
				mem_pool_free(&mob_pool, m);
				continue;
			}
		}

		link = &m->next;
	}
}

void mobs_populate(void)
{
	/* A handful of animals so a fresh world is not empty. */
	for (int i = 0; i < 8; i++)
		spawn_wave();
}

bool mobs_hurt(struct mob *m, int amount, float push_x, float push_y)
{
	if (m->health <= amount) {
		m->health = 0;

		const struct mob_info *info = &mob_info[m->type];

		if (info->drop)
			items_spawn(m->x, m->y, m->z + 0.4f, info->drop, info->drop_count);

		sfx_play("/i:square (p) >c<g");
		return true;
	}

	m->health -= amount;
	m->hurt = 0.35f;

	/* Knock it back, and up a touch so it visibly reacts. */
	m->vx += push_x * 4.0f;
	m->vy += push_y * 4.0f;

	if (m->on_ground)
		m->vz = 4.0f;

	sfx_play("/i:square (ppp) >d");
	return false;
}

/*
 * Take a dead mob out of the live list and hand its slot back.
 *
 * Kept separate from mobs_hurt because the update loop already holds a
 * pointer to the link it would have to rewrite, and doing it twice would
 * mean walking the list for nothing.
 */
void mobs_kill(struct mob *mob)
{
	struct mob **link = &mobs;

	while (*link && *link != mob)
		link = &(*link)->next;

	if (!*link)
		return;

	*link = mob->next;
	mem_pool_free(&mob_pool, mob);
}

/*
 * Which mob is the player looking at?
 *
 * We treat each mob as a sphere and ask how close the ray passes to its
 * centre. Projecting the mob onto the ray gives the distance along it, and
 * the leftover perpendicular distance tells us whether we clipped it. This
 * is far cheaper than a box test and, for something you are pointing at
 * from a couple of blocks away, indistinguishable.
 */
struct mob *mobs_pick(float ox, float oy, float oz, float dx, float dy, float dz, float reach,
		      float *out_dist)
{
	struct mob *best = NULL;
	float best_t = reach;

	for (struct mob *m = mobs; m; m = m->next) {
		float radius = 0.5f;
		float cx = m->x - ox;
		float cy = m->y - oy;
		float cz = (m->z + mob_info[m->type].height * 0.5f) - oz;

		/* How far along the ray the mob's centre projects. */
		float t = cx * dx + cy * dy + cz * dz;

		if (t < 0.0f || t > best_t)
			continue;

		/* Perpendicular distance from the ray to that centre. */
		float px = cx - dx * t;
		float py = cy - dy * t;
		float pz = cz - dz * t;

		if (px * px + py * py + pz * pz > radius * radius)
			continue;

		best = m;
		best_t = t;
	}

	if (best && out_dist)
		*out_dist = best_t;

	return best;
}
