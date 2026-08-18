#include "minevox.h"

#include <math.h>
#include <stdlib.h>

/*
 * ===========================================================================
 * The player
 * ===========================================================================
 *
 * Physics, survival stats, and what each button does. This is the file to
 * change when the game feels wrong: every number that decides how it plays
 * is at the top.
 */

struct player player;

#define WALK_SPEED 4.3f
#define FLY_SPEED 9.0f
#define SWIM_SPEED 3.0f
#define TURN_SPEED 2.6f
#define PITCH_SPEED 2.0f
#define GRAVITY 26.0f
#define JUMP_SPEED 8.2f
#define MAX_PITCH 1.52f
#define STEP_HEIGHT 1.05f

/* Survival tuning. */
#define BREATH_SECONDS 12.0f
#define REGEN_SECONDS 4.5f
#define SAFE_FALL 3.5f
#define LAVA_DAMAGE 4

static float double_tap_timer;
static float pick_hold;
static float drown_timer;
static float lava_timer;

/* Head bobbing while walking. */
static float bob_phase;
static float bob_amp;
static bool bob_active;

/*
 * The shoulder triggers are pressure sensitive on the console and plain
 * keys on the desktop build, so they need reading differently. Everything
 * else about the two builds is identical.
 */
static float trigger(int16_t digital, float analog)
{
#ifdef RUNNING_ON_HOST
	(void)analog;
	return digital ? 1.0f : 0.0f;
#else
	(void)digital;

	if (analog < 0.18f)
		return 0.0f;

	return clampf((analog - 0.18f) * 2.0f, 0.0f, 1.0f);
#endif
}

void player_look_dir(float *dx, float *dy, float *dz)
{
	float cp = cosf(player.pitch);
	*dx = cosf(player.yaw) * cp;
	*dy = sinf(player.yaw) * cp;
	*dz = sinf(player.pitch);
}

uint16_t player_held_item(void)
{
	return inventory[player.slot].item;
}

void player_reset(bool keep_position)
{
	player.vx = player.vy = player.vz = 0.0f;
	player.bob = 0.0f;
	player.mining = 0.0f;
	player.mining_need = 0.0f;
	player.swing = 0.0f;
	player.max_health = PLAYER_MAX_HEALTH;
	player.health = PLAYER_MAX_HEALTH;
	player.air = BREATH_SECONDS;
	player.regen = 0.0f;
	player.hurt_flash = 0.0f;
	player.has_target = false;
	drown_timer = 0.0f;
	lava_timer = 0.0f;

	if (keep_position)
		return;

	player.slot = 0;
	player.yaw = 0.6f;
	player.pitch = -0.15f;
	player.flying = false;

	world_find_spawn(&player.x, &player.y, &player.z);

	player.respawn_x = player.x;
	player.respawn_y = player.y;
	player.respawn_z = player.z;

	player.fall_from = player.z;

	/*
	 * Creative starts with a full set of blocks to build with; survival
	 * starts with nothing, which is rather the point.
	 */
	if (!survival_mode) {
		static const uint16_t kit[HOTBAR_SLOTS] = {
			BLOCK_GRASS, BLOCK_DIRT,   BLOCK_STONE, BLOCK_COBBLE,	 BLOCK_SAND,
			BLOCK_LOG,   BLOCK_PLANKS, BLOCK_GLASS, BLOCK_GLOWSTONE,
		};

		for (int i = 0; i < HOTBAR_SLOTS; i++) {
			inventory[i].item = kit[i];
			inventory[i].count = 64;
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Collision
 * ---------------------------------------------------------------------------
 *
 * The player is a box 0.6 blocks wide and 1.8 tall. To move, we try each
 * axis separately and give up only on the axis that is blocked. That is
 * why you slide along a wall instead of stopping dead when you walk into
 * it at an angle, and it is the simplest collision response that feels
 * right.
 */
static bool solid_at(int x, int y, int z)
{
	if (!world_inside(x, y, z))
		return true; /* The edge of the world is a wall. */

	return block_is_solid(world_get_fast(x, y, z));
}

static bool box_blocked(float x, float y, float z)
{
	int x0 = (int)floorf(x - PLAYER_RADIUS);
	int x1 = (int)floorf(x + PLAYER_RADIUS);
	int y0 = (int)floorf(y - PLAYER_RADIUS);
	int y1 = (int)floorf(y + PLAYER_RADIUS);
	int z0 = (int)floorf(z + 0.001f);
	int z1 = (int)floorf(z + PLAYER_HEIGHT - 0.001f);

	for (int bz = z0; bz <= z1; bz++)
		for (int by = y0; by <= y1; by++)
			for (int bx = x0; bx <= x1; bx++)
				if (solid_at(bx, by, bz))
					return true;

	return false;
}

static bool liquid_at(float x, float y, float z, uint8_t *what)
{
	uint8_t block = world_get((int)floorf(x), (int)floorf(y), (int)floorf(z));

	if (what)
		*what = block;

	return block_info[block].liquid;
}

static void move_axis(float dx, float dy, float dz)
{
	float nx = player.x + dx;
	float ny = player.y + dy;
	float nz = player.z + dz;

	if (!box_blocked(nx, ny, nz)) {
		player.x = nx;
		player.y = ny;
		player.z = nz;
		return;
	}

	if (dz != 0.0f) {
		if (dz < 0.0f)
			player.on_ground = true;

		player.vz = 0.0f;
		return;
	}

	/*
	 * Step up a single block automatically. Jumping over every ledge
	 * gets tiring when one stick has to both turn and walk.
	 */
	if ((player.on_ground || player.in_water) && !box_blocked(nx, ny, player.z + STEP_HEIGHT)) {
		player.x = nx;
		player.y = ny;
		player.z += STEP_HEIGHT;
		return;
	}

	if (dx != 0.0f)
		player.vx = 0.0f;
	if (dy != 0.0f)
		player.vy = 0.0f;
}

/*
 * ---------------------------------------------------------------------------
 * Survival
 * ---------------------------------------------------------------------------
 */
void player_hurt(int amount)
{
	if (!survival_mode || amount <= 0)
		return;

	if (player.health <= 0)
		return;

	player.health -= amount;
	player.hurt_flash = 0.35f;
	player.regen = 0.0f;
	sfx_play("/i:square (p) >c<a");

	if (player.health > 0)
		return;

	/* Death: everything you carried stays where you fell. */
	player.health = 0;
	inventory_drop_all(player.x, player.y, player.z);
	menu_open(STATE_DEAD);
}

void player_heal(int amount)
{
	player.health = clampi(player.health + amount, 0, player.max_health);
}

/* Put the player back at their spawn point with a clean slate. */
static void respawn(void)
{
	player.x = player.respawn_x;
	player.y = player.respawn_y;
	player.z = player.respawn_z;
	player.vx = player.vy = player.vz = 0.0f;
	player.health = player.max_health;
	player.air = BREATH_SECONDS;
	player.hurt_flash = 0.0f;
	player.fall_from = player.z;
	player.mining = 0.0f;
	drown_timer = 0.0f;
	lava_timer = 0.0f;
}

static void survival_tick(float dt)
{
	if (!survival_mode) {
		player.health = player.max_health;
		player.air = BREATH_SECONDS;
		return;
	}

	if (player.hurt_flash > 0.0f)
		player.hurt_flash -= dt;

	/* Breath, and drowning once it runs out. */
	if (player.head_in_water) {
		player.air -= dt;

		if (player.air <= 0.0f) {
			player.air = 0.0f;
			drown_timer -= dt;

			if (drown_timer <= 0.0f) {
				drown_timer = 1.0f;
				player_hurt(2);
			}
		}
	} else {
		player.air = clampf(player.air + dt * 4.0f, 0.0f, BREATH_SECONDS);
		drown_timer = 0.0f;
	}

	/* Lava hurts continuously, which is what makes the deep frightening. */
	uint8_t inside = BLOCK_AIR;
	liquid_at(player.x, player.y, player.z + 0.4f, &inside);

	if (inside == BLOCK_LAVA) {
		lava_timer -= dt;

		if (lava_timer <= 0.0f) {
			lava_timer = 0.6f;
			player_hurt(LAVA_DAMAGE);
		}
	} else {
		lava_timer = 0.0f;
	}

	/* Slow regeneration, so a bad night is survivable but costly. */
	if (player.health > 0 && player.health < player.max_health) {
		player.regen += dt;

		if (player.regen >= REGEN_SECONDS) {
			player.regen = 0.0f;
			player_heal(1);
		}
	}
}

/*
 * Fall damage.
 *
 * We remember the height where the fall started rather than looking at
 * speed, because that is how the real game does it and it behaves better
 * when you clip a ledge on the way down.
 */
static void fall_check(bool was_on_ground)
{
	if (!player.on_ground) {
		if (was_on_ground)
			player.fall_from = player.z;

		if (player.z > player.fall_from)
			player.fall_from = player.z;

		return;
	}

	if (was_on_ground)
		return;

	float drop = player.fall_from - player.z;

	if (survival_mode && !player.in_water && drop > SAFE_FALL)
		player_hurt((int)(drop - SAFE_FALL));

	player.fall_from = player.z;
}

/*
 * ---------------------------------------------------------------------------
 * Aiming, mining and building
 * ---------------------------------------------------------------------------
 */
static void update_target(void)
{
	float dx, dy, dz;
	player_look_dir(&dx, &dy, &dz);

	float ez = player.z + PLAYER_EYE;
	int hx, hy, hz, px, py, pz;

	if (world_raycast(player.x, player.y, ez, dx, dy, dz, REACH, &hx, &hy, &hz, &px, &py,
			  &pz)) {
		/* Aiming somewhere new cancels whatever we were digging. */
		if (player.has_target && (player.tx != hx || player.ty != hy || player.tz != hz))
			player.mining = 0.0f;

		player.has_target = true;
		player.tx = hx;
		player.ty = hy;
		player.tz = hz;
		player.px = px;
		player.py = py;
		player.pz = pz;
	} else {
		player.has_target = false;
		player.mining = 0.0f;
	}
}

/*
 * Tools.
 *
 * Only a PICKAXE is ever a hard requirement. Blocks marked TOOL_AXE are
 * merely quicker with one: you can always pull a tree apart with your
 * hands, it just takes longer, exactly as in the real game.
 *
 * Getting this wrong is worth remembering. An earlier version demanded an
 * exact tool match for every block that named one, which meant logs
 * needed an axe, and there is no axe in the game. Wood is the very first
 * thing a new player mines, so the entire progression was sealed shut
 * behind a tool that did not exist.
 */
static bool needs_tool(const struct block_info *info)
{
	return info->tool == TOOL_PICK;
}

/* How long this block takes to break with what we are holding. */
static float mining_time(uint8_t block)
{
	const struct block_info *info = &block_info[block];
	float base = info->hardness * 0.1f;

	uint16_t held = player_held_item();
	int speed = 16; /* Sixteenths, so 16 means bare hands. */

	if (held >= ITEM_BASE && held < NUM_ITEM_IDS)
		speed = item_info[held - ITEM_BASE].tool_speed;

	bool holding_right_tool = info->tool == TOOL_NONE || item_tool_kind(held) == info->tool;

	if (!holding_right_tool) {
		/* Bare hands on rock is hopeless; on wood, merely slow. */
		return base * (needs_tool(info) ? 3.0f : 1.6f);
	}

	return base * 16.0f / (float)speed;
}

/* Does the block we are breaking actually give us anything? */
static bool tool_gets_drop(uint8_t block)
{
	const struct block_info *info = &block_info[block];

	if (!needs_tool(info))
		return true;

	return item_tool_kind(player_held_item()) == TOOL_PICK;
}

static void try_break(float dt)
{
	if (!player.has_target)
		return;

	uint8_t block = world_get(player.tx, player.ty, player.tz);

	if (block == BLOCK_AIR || block_info[block].hardness == 255) {
		player.mining = 0.0f;
		return;
	}

	player.swing = 1.0f;

	/* Creative mode breaks instantly, which is what it is for. */
	player.mining_need = survival_mode ? mining_time(block) : 0.05f;
	player.mining += dt;

	if (player.mining < player.mining_need)
		return;

	uint16_t drop = block_info[block].drop;

	world_set(player.tx, player.ty, player.tz, BLOCK_AIR);
	player.mining = 0.0f;

	if (survival_mode && drop && tool_gets_drop(block))
		items_spawn(player.tx + 0.5f, player.ty + 0.5f, player.tz + 0.5f, drop, 1);

	sfx_play("/i:square (p) >g");
}

static void try_attack(void)
{
	float dx, dy, dz;
	player_look_dir(&dx, &dy, &dz);

	struct mob *m =
		mobs_pick(player.x, player.y, player.z + PLAYER_EYE, dx, dy, dz, 3.2f, NULL);

	if (!m)
		return;

	uint16_t held = player_held_item();
	int damage = 2;

	if (held >= ITEM_BASE && held < NUM_ITEM_IDS)
		damage += item_info[held - ITEM_BASE].damage;

	player.swing = 1.0f;

	if (mobs_hurt(m, damage, dx, dy))
		mobs_kill(m);
}

static void try_use(void)
{
	uint16_t held = player_held_item();

	/* Eating comes first: food is never placed. */
	if (held >= ITEM_BASE && held < NUM_ITEM_IDS && item_info[held - ITEM_BASE].heal) {
		if (survival_mode && player.health >= player.max_health) {
			menu_toast("Not hungry");
			return;
		}

		if (inventory_take(held, 1)) {
			player_heal(item_info[held - ITEM_BASE].heal);
			player.swing = 1.0f;
			sfx_play("/i:sine (p) egc");
		}

		return;
	}

	if (!player.has_target)
		return;

	/* Tapping a completed reactor lights it. */
	if (world_get(player.tx, player.ty, player.tz) == BLOCK_REACTOR) {
		if (nether_activate(player.tx, player.ty, player.tz))
			return;

		menu_toast("The pattern is wrong");
		return;
	}

	if (!item_is_block(held))
		return;

	int x = player.px, y = player.py, z = player.pz;

	if (!world_inside(x, y, z))
		return;

	uint8_t existing = world_get_fast(x, y, z);

	if (existing != BLOCK_AIR && !block_info[existing].liquid)
		return;

	/*
	 * Refuse to place a block inside ourselves. We test by putting it
	 * there, asking the collision code, and taking it straight back
	 * out, which is much simpler than duplicating the box maths.
	 */
	world_put_raw(x, y, z, held);
	bool stuck = block_is_solid(held) && box_blocked(player.x, player.y, player.z);
	world_put_raw(x, y, z, existing);

	if (stuck)
		return;

	if (survival_mode && !inventory_take(held, 1))
		return;

	world_set(x, y, z, held);
	player.swing = 1.0f;
	sfx_play("/i:square (p) >c");
}

/*
 * ---------------------------------------------------------------------------
 * Input
 * ---------------------------------------------------------------------------
 *
 * One stick and six buttons for a game that normally wants a keyboard and
 * a mouse. The mapping:
 *
 *   stick      walk forwards and back, turn left and right
 *   L / R      look down and up
 *   A          jump, or swim up; double tap toggles flight in creative
 *   B          mine what you are looking at, or hit a mob (hold)
 *   X          use: place a block, eat food, light a reactor
 *   Y          tap for the next hotbar slot, hold to open the inventory
 *   START      pause menu
 */
void player_input(float dt)
{
	if (player.health <= 0)
		return;

	float jx = sdk_inputs.jx;
	float jy = sdk_inputs.jy;

	/* Ignore the middle of the stick, real ones never centre exactly. */
	if (fabsf(jx) < 0.15f)
		jx = 0.0f;
	if (fabsf(jy) < 0.15f)
		jy = 0.0f;

	player.yaw -= jx * TURN_SPEED * dt;

	float look_up = trigger(sdk_inputs.brack_r, sdk_inputs.br);
	float look_down = trigger(sdk_inputs.brack_l, sdk_inputs.bl);

	player.pitch += (look_up - look_down) * PITCH_SPEED * dt;
	player.pitch = clampf(player.pitch, -MAX_PITCH, MAX_PITCH);

	if (player.yaw > 2.0f * (float)M_PI)
		player.yaw -= 2.0f * (float)M_PI;
	if (player.yaw < 0.0f)
		player.yaw += 2.0f * (float)M_PI;

	/* Double tapping A toggles flight, but only outside survival. */
	if (double_tap_timer > 0.0f)
		double_tap_timer -= dt;

	if (sdk_inputs_delta.a > 0) {
		if (double_tap_timer > 0.0f && !survival_mode) {
			player.flying = !player.flying;
			player.vz = 0.0f;
			double_tap_timer = 0.0f;
			sfx_play(player.flying ? "/i:sine (p) cG" : "/i:sine (p) Gc");
		} else {
			double_tap_timer = 0.35f;
		}
	}

	if (survival_mode)
		player.flying = false;

	float forward = -jy;
	float speed = player.flying ? FLY_SPEED : (player.in_water ? SWIM_SPEED : WALK_SPEED);

	float dx, dy, dz;
	player_look_dir(&dx, &dy, &dz);

	if (player.flying) {
		/* Flying follows the whole view vector, so you steer with pitch. */
		player.vx = dx * forward * speed;
		player.vy = dy * forward * speed;
		player.vz = dz * forward * speed;

		if (sdk_inputs.a)
			player.vz += FLY_SPEED * 0.6f;
	} else {
		/* Walking ignores pitch: flatten the direction first. */
		float len = sqrtf(dx * dx + dy * dy);

		if (len > 0.0001f) {
			dx /= len;
			dy /= len;
		}

		player.vx = dx * forward * speed;
		player.vy = dy * forward * speed;

		if (sdk_inputs.a) {
			if (player.in_water)
				player.vz = SWIM_SPEED * 1.2f;
			else if (player.on_ground)
				player.vz = JUMP_SPEED;
		}
	}

	bob_active = forward != 0.0f && player.on_ground && !player.flying;

	/* B mines, unless there is a mob in the way, in which case it hits. */
	if (sdk_inputs.b) {
		float mob_dist = 0.0f;
		struct mob *m = mobs_pick(player.x, player.y, player.z + PLAYER_EYE, dx, dy, dz,
					  3.2f, &mob_dist);

		if (m && sdk_inputs_delta.b > 0)
			try_attack();
		else if (!m)
			try_break(dt);
	} else {
		player.mining = 0.0f;
	}

	if (sdk_inputs_delta.x > 0)
		try_use();

	/*
	 * Y taps to the next hotbar slot and holds to open the inventory.
	 * We only start counting on a fresh press, so releasing Y after
	 * closing the inventory does not also switch blocks.
	 */
	if (sdk_inputs_delta.y > 0)
		pick_hold = 0.001f;

	if (pick_hold > 0.0f) {
		if (sdk_inputs.y) {
			pick_hold += dt;

			if (pick_hold > 0.35f) {
				menu_open(survival_mode ? STATE_INVENTORY : STATE_PICKER);
				pick_hold = 0.0f;
			}
		} else {
			player.slot = (player.slot + 1) % HOTBAR_SLOTS;
			pick_hold = 0.0f;
		}
	}
}

void player_update(float dt)
{
	/* Never integrate a huge step: it tunnels straight through walls. */
	if (dt > 0.05f)
		dt = 0.05f;

	if (player.health <= 0) {
		/* Dead players only wait for the respawn screen. */
		if (game_state == STATE_PLAY)
			respawn();

		return;
	}

	uint8_t feet = BLOCK_AIR;
	player.in_water = liquid_at(player.x, player.y, player.z + 0.4f, &feet);
	player.head_in_water = liquid_at(player.x, player.y, player.z + PLAYER_EYE, NULL) &&
			       feet != BLOCK_LAVA;

	if (!player.flying) {
		if (player.in_water) {
			/* Water is thick: weak gravity and a lot of drag. */
			player.vz -= GRAVITY * 0.18f * dt;
			player.vz *= 0.88f;
			player.vz = clampf(player.vz, -4.0f, 6.0f);
			player.fall_from = player.z;
		} else {
			player.vz -= GRAVITY * dt;
			player.vz = clampf(player.vz, -40.0f, 20.0f);
		}
	} else {
		player.vz *= 0.86f;
	}

	bool was_on_ground = player.on_ground;
	player.on_ground = false;

	move_axis(player.vx * dt, 0.0f, 0.0f);
	move_axis(0.0f, player.vy * dt, 0.0f);
	move_axis(0.0f, 0.0f, player.vz * dt);

	if (player.z < 0.5f) {
		player.z = 0.5f;
		player.vz = 0.0f;
		player.on_ground = true;
	}

	fall_check(was_on_ground);
	survival_tick(dt);

	/* Head bob: a sine wave whose amplitude eases in and out. */
	if (bob_active) {
		bob_phase += dt * 9.0f;
		bob_amp = clampf(bob_amp + dt * 0.4f, 0.0f, 0.055f);
	} else {
		bob_amp = clampf(bob_amp - dt * 0.4f, 0.0f, 0.055f);
	}

	player.bob = sinf(bob_phase) * bob_amp;

	if (player.swing > 0.0f)
		player.swing -= dt * 3.0f;

	/* Standing on solid ground marks a new place to respawn. */
	if (player.on_ground && !player.in_water && player.z > SEA_LEVEL) {
		player.respawn_x = player.x;
		player.respawn_y = player.y;
		player.respawn_z = player.z;
	}

	update_target();
}
