#include "minevox.h"

#include <math.h>
#include <stdlib.h>

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

static float double_tap_timer = 0.0f;
static float pick_hold = 0.0f;

/* Head bobbing while walking. */
static float bob_phase = 0.0f;
static float bob_amp = 0.0f;
static bool bob_active = false;

/* Triggers are analog on the console and plain keys on the host. */
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

void player_reset(void)
{
	const uint8_t defaults[9] = {
		BLOCK_GRASS, BLOCK_DIRT,   BLOCK_STONE, BLOCK_COBBLE,	 BLOCK_SAND,
		BLOCK_LOG,   BLOCK_PLANKS, BLOCK_GLASS, BLOCK_GLOWSTONE,
	};

	for (int i = 0; i < 9; i++)
		player.hotbar[i] = defaults[i];

	player.slot = 0;
	player.yaw = 0.6f;
	player.pitch = -0.15f;
	player.vx = player.vy = player.vz = 0.0f;
	player.flying = false;
	player.bob = 0.0f;
	player.mining = 0.0f;
	player.mining_need = 0.0f;
	player.swing = 0.0f;

	world_find_spawn(&player.x, &player.y, &player.z);
}

/* Anything outside of the world counts as a wall. */
static bool solid_at(int x, int y, int z)
{
	if (!world_inside(x, y, z))
		return true;

	return block_is_solid(world_get_fast(x, y, z));
}

/* Does the player box at given feet position hit anything solid? */
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

static bool liquid_at(float x, float y, float z)
{
	uint8_t block = world_get((int)floorf(x), (int)floorf(y), (int)floorf(z));
	return block_info[block].liquid;
}

/* Move along one axis, stopping at the first wall. */
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
	 * Step up a single block on our own. Jumping over every ledge gets
	 * tiresome when turning and walking share one stick.
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

/* Find the block we are looking at. */
static void update_target(void)
{
	float dx, dy, dz;
	player_look_dir(&dx, &dy, &dz);

	float ex = player.x;
	float ey = player.y;
	float ez = player.z + PLAYER_EYE;

	int hx, hy, hz, px, py, pz;

	if (world_raycast(ex, ey, ez, dx, dy, dz, REACH, &hx, &hy, &hz, &px, &py, &pz)) {
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

static void try_break(float dt)
{
	if (!player.has_target)
		return;

	uint8_t block = world_get(player.tx, player.ty, player.tz);

	if (block == BLOCK_AIR || block == BLOCK_BEDROCK) {
		player.mining = 0.0f;
		return;
	}

	player.mining_need = block_info[block].hardness * 0.1f;
	player.mining += dt;
	player.swing = 1.0f;

	if (player.mining < player.mining_need)
		return;

	world_set(player.tx, player.ty, player.tz, BLOCK_AIR);
	player.mining = 0.0f;
	sfx_play("/i:square (p) >g");
}

static void try_place(void)
{
	if (!player.has_target)
		return;

	uint8_t block = player.hotbar[player.slot];

	if (!block)
		return;

	int x = player.px, y = player.py, z = player.pz;

	if (!world_inside(x, y, z))
		return;

	if (world_get_fast(x, y, z) != BLOCK_AIR && !block_info[world_get_fast(x, y, z)].liquid)
		return;

	/* Do not brick ourselves in. */
	uint8_t saved = world_get_fast(x, y, z);
	world_put_raw(x, y, z, block);
	bool stuck = block_is_solid(block) && box_blocked(player.x, player.y, player.z);
	world_put_raw(x, y, z, saved);

	if (stuck)
		return;

	world_set(x, y, z, block);
	player.swing = 1.0f;
	sfx_play("/i:square (p) >c");
}

void player_input(float dt)
{
	float jx = sdk_inputs.jx;
	float jy = sdk_inputs.jy;

	if (fabsf(jx) < 0.15f)
		jx = 0.0f;
	if (fabsf(jy) < 0.15f)
		jy = 0.0f;

	/* Turning and looking. */
	player.yaw -= jx * TURN_SPEED * dt;

	float look_up = trigger(sdk_inputs.brack_r, sdk_inputs.br);
	float look_down = trigger(sdk_inputs.brack_l, sdk_inputs.bl);

	player.pitch += (look_up - look_down) * PITCH_SPEED * dt;
	player.pitch = clampf(player.pitch, -MAX_PITCH, MAX_PITCH);

	if (player.yaw > 2.0f * (float)M_PI)
		player.yaw -= 2.0f * (float)M_PI;
	if (player.yaw < 0.0f)
		player.yaw += 2.0f * (float)M_PI;

	/* Double tap on A toggles creative flight. */
	if (double_tap_timer > 0.0f)
		double_tap_timer -= dt;

	if (sdk_inputs_delta.a > 0) {
		if (double_tap_timer > 0.0f) {
			player.flying = !player.flying;
			player.vz = 0.0f;
			double_tap_timer = 0.0f;
			sfx_play(player.flying ? "/i:sine (p) cG" : "/i:sine (p) Gc");
		} else {
			double_tap_timer = 0.35f;
		}
	}

	/* Walking, swimming or flying forward. */
	float forward = -jy;
	float speed = player.flying ? FLY_SPEED : (player.in_water ? SWIM_SPEED : WALK_SPEED);

	float dx, dy, dz;
	player_look_dir(&dx, &dy, &dz);

	if (player.flying) {
		player.vx = dx * forward * speed;
		player.vy = dy * forward * speed;
		player.vz = dz * forward * speed;

		if (sdk_inputs.a)
			player.vz += FLY_SPEED * 0.6f;
	} else {
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

	/* Mining and building. */
	if (sdk_inputs.b)
		try_break(dt);
	else
		player.mining = 0.0f;

	if (sdk_inputs_delta.x > 0)
		try_place();

	/*
	 * Tap Y to switch blocks, hold it to open the picker. We only count
	 * a hold that started here, so that letting go of Y after closing
	 * the picker does not immediately switch blocks.
	 */
	if (sdk_inputs_delta.y > 0)
		pick_hold = 0.001f;

	if (pick_hold > 0.0f) {
		if (sdk_inputs.y) {
			pick_hold += dt;

			if (pick_hold > 0.35f) {
				game_state = STATE_PICKER;
				pick_hold = 0.0f;
			}
		} else {
			player.slot = (player.slot + 1) % 9;
			pick_hold = 0.0f;
		}
	}
}

void player_update(float dt)
{
	if (dt > 0.05f)
		dt = 0.05f;

	player.in_water = liquid_at(player.x, player.y, player.z + 0.4f);
	player.head_in_water = liquid_at(player.x, player.y, player.z + PLAYER_EYE);

	if (!player.flying) {
		if (player.in_water) {
			player.vz -= GRAVITY * 0.18f * dt;
			player.vz *= 0.88f;
			player.vz = clampf(player.vz, -4.0f, 6.0f);
		} else {
			player.vz -= GRAVITY * dt;
			player.vz = clampf(player.vz, -40.0f, 20.0f);
		}
	} else {
		player.vz *= 0.86f;
	}

	player.on_ground = false;

	move_axis(player.vx * dt, 0.0f, 0.0f);
	move_axis(0.0f, player.vy * dt, 0.0f);
	move_axis(0.0f, 0.0f, player.vz * dt);

	/* Never fall out of the world. */
	if (player.z < 0.5f) {
		player.z = 0.5f;
		player.vz = 0.0f;
		player.on_ground = true;
	}

	if (bob_active) {
		bob_phase += dt * 9.0f;
		bob_amp = clampf(bob_amp + dt * 0.4f, 0.0f, 0.055f);
	} else {
		bob_amp = clampf(bob_amp - dt * 0.4f, 0.0f, 0.055f);
	}

	player.bob = sinf(bob_phase) * bob_amp;

	if (player.swing > 0.0f)
		player.swing -= dt * 3.0f;

	update_target();
}
