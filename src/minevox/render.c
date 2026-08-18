#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * ===========================================================================
 * The renderer
 * ===========================================================================
 *
 * One ray per screen pixel, marched through the voxel grid with the DDA
 * explained in world.c. There is no triangle, no depth sort and no mesh
 * anywhere in this game: for every one of the 19200 pixels we ask "what is
 * the first block along this line", and colour the pixel accordingly.
 *
 * WHY A RAY PER PIXEL RATHER THAN PER COLUMN
 *
 * The classic 1990s raycasters cast one ray per screen column and drew a
 * vertical strip, which is why you could never look up or down in them.
 * Casting per pixel costs 120 times more rays but frees the camera
 * completely: any pitch, any roll if we wanted it, floors and ceilings for
 * free, and blocks above and below you all handled by the same code.
 *
 * THE TWO PASSES
 *
 *   1. render_world casts the rays and, as well as a colour, records how
 *      far away each pixel is in depth_buffer.
 *   2. draw_sprites projects mobs and dropped items onto the screen and
 *      draws them, testing that depth so a pig behind a hill is hidden by
 *      it. This is a Z-BUFFER, the same idea a GPU uses, at one byte per
 *      pixel instead of four.
 */

#define FOV_DEG 75.0f
#define VIEW_DIST 38.0f
#define FOG_START 20.0f
#define MAX_STEPS 56

/* How much of what is behind shows through, out of 32. */
#define ALPHA_WATER 21
#define ALPHA_GLASS 9

int render_scale = 1;
uint8_t depth_buffer[DEPTH_W * DEPTH_H];

/*
 * The frame buffer is stored with x as the outer index on this hardware.
 * Hiding that behind a macro keeps the loops readable and lets the same
 * code work if the screen is ever mounted the other way round.
 */
#if TFT_SWAP_XY
#define FB_PIXEL(x, y) tft_input[x][y]
#else
#define FB_PIXEL(x, y) tft_input[y][x]
#endif

/*
 * Face brightness, out of 32.
 *
 * There is no real lighting model here. Giving each of the six directions
 * a fixed brightness is enough for the eye to read a cube as a cube, and
 * it costs one table lookup. The values are close to the ones the real
 * game uses.
 */
#define SHADE_TOP 32
#define SHADE_BOTTOM 16
#define SHADE_X 20
#define SHADE_Y 26

/* Clouds live on a plane this high and fade out at this range. */
#define CLOUD_HEIGHT 48.0f
#define CLOUD_RANGE 320.0f

#define CLOUD_SIZE 64
static uint8_t cloud_map[CLOUD_SIZE][CLOUD_SIZE];

/* Crack overlay for the block being mined. */
#define CRACK_STAGES 4
static uint8_t crack_map[CRACK_STAGES][TEX_SIZE * TEX_SIZE];

/* Recomputed once per frame. */
static color_t sky_zenith, sky_horizon, sky_ground, sun_color;
static float daylight;
static float sun_dx, sun_dy, sun_dz;
static uint8_t sky_shade[WORLD_Z + 1];
static int water_wave;
static float nether_mix;

/* Camera basis for the current frame, shared with the sprite pass. */
static float cam_ex, cam_ey, cam_ez;
static float cam_fx, cam_fy, cam_fz;
static float cam_rx, cam_ry, cam_rz;
static float cam_ux, cam_uy, cam_uz;
static float cam_tan_fov, cam_aspect;

static uint32_t rnd_hash(uint32_t a)
{
	a ^= a >> 17;
	a *= 0xed5ad4bbU;
	a ^= a >> 11;
	a *= 0xac4c1b51U;
	a ^= a >> 15;
	return a;
}

void render_init(void)
{
	/*
	 * Clouds: three layers of blocky noise at different scales added
	 * together, then everything below a threshold thrown away. Keeping
	 * only the peaks is what turns a fuzzy field into distinct clouds
	 * with clear sky between them.
	 */
	for (int y = 0; y < CLOUD_SIZE; y++) {
		for (int x = 0; x < CLOUD_SIZE; x++) {
			int v = 0;

			for (int o = 0; o < 3; o++) {
				int step = 4 << o;
				int gx = x / step, gy = y / step;
				v += (int)(rnd_hash(gx * 7919u + gy * 104729u + o * 31u) & 0xff) >>
				     o;
			}

			cloud_map[y][x] = v > 220 ? clampi((v - 220) * 3, 0, 255) : 0;
		}
	}

	/* Crack stages, each adding more chips than the last. */
	for (int s = 0; s < CRACK_STAGES; s++) {
		for (int i = 0; i < TEX_SIZE * TEX_SIZE; i++) {
			uint32_t h = rnd_hash(i * 2654435761u) & 0xff;
			crack_map[s][i] = h < (uint32_t)(20 + s * 45);
		}
	}
}

static color_t lerp_color(color_t a, color_t b, float t)
{
	return color_blend(a, b, (uint8_t)clampi((int)(t * 32.0f), 0, 32));
}

/*
 * Work out what the sky looks like right now.
 *
 * The sun's height above the horizon drives everything: the overall light
 * level, the colours at the zenith and the horizon, and how orange the
 * horizon goes at dawn and dusk. An active nether reactor then drags the
 * whole palette towards red.
 */
void render_sky_colors(color_t *zenith, color_t *horizon, float *light)
{
	float phase = world_time / DAY_LENGTH;
	float sun_alt = sinf(phase * 2.0f * (float)M_PI);

	daylight = clampf(sun_alt * 1.8f + 0.42f, 0.10f, 1.0f);

	/* Dusk peaks when the sun is near the horizon, either way up. */
	float dusk = clampf(1.0f - fabsf(sun_alt) * 3.2f, 0.0f, 1.0f);

	const color_t day_zenith = rgb_to_rgb565(62, 130, 226);
	const color_t day_horizon = rgb_to_rgb565(164, 201, 240);
	const color_t night_zenith = rgb_to_rgb565(6, 8, 26);
	const color_t night_horizon = rgb_to_rgb565(22, 26, 56);
	const color_t dusk_horizon = rgb_to_rgb565(236, 132, 62);
	const color_t nether_zenith = rgb_to_rgb565(70, 10, 12);
	const color_t nether_horizon = rgb_to_rgb565(170, 44, 26);

	float f = clampf((daylight - 0.10f) / 0.90f, 0.0f, 1.0f);

	sky_zenith = lerp_color(night_zenith, day_zenith, f);
	sky_horizon = lerp_color(night_horizon, day_horizon, f);
	sky_horizon = lerp_color(sky_horizon, dusk_horizon, dusk * 0.75f);

	nether_mix = nether_intensity();

	if (nether_mix > 0.0f) {
		sky_zenith = lerp_color(sky_zenith, nether_zenith, nether_mix);
		sky_horizon = lerp_color(sky_horizon, nether_horizon, nether_mix);

		/* The reactor lights the world even in the dead of night. */
		daylight = daylight + (0.55f - daylight) * nether_mix * 0.8f;
	}

	sky_ground = color_mul(sky_horizon, 14);
	sun_color = sun_alt > -0.1f ? rgb_to_rgb565(255, 246, 214) : rgb_to_rgb565(224, 228, 240);

	/* Sun direction, tilted so it does not pass exactly overhead. */
	float ang = phase * 2.0f * (float)M_PI;
	float len = sqrtf(1.0f + 0.36f);
	sun_dx = cosf(ang) / len;
	sun_dy = 0.6f / len;
	sun_dz = sinf(ang) / len;

	if (zenith)
		*zenith = sky_zenith;
	if (horizon)
		*horizon = sky_horizon;
	if (light)
		*light = daylight;
}

/* The colour of the sky in a given direction. */
static color_t shade_sky(float dx, float dy, float dz, float ex, float ey, float ez)
{
	/* Ray directions are not unit length, and the maths below wants them. */
	float len = sqrtf(dx * dx + dy * dy + dz * dz);

	if (len > 0.0f) {
		float inv = 1.0f / len;
		dx *= inv;
		dy *= inv;
		dz *= inv;
	}

	if (dz < 0.0f) {
		float t = clampf(-dz * 3.0f, 0.0f, 1.0f);
		return lerp_color(sky_horizon, sky_ground, t);
	}

	float t = clampf(dz * 1.6f, 0.0f, 1.0f);
	color_t c = lerp_color(sky_horizon, sky_zenith, t);

	/*
	 * The sun. A dot product with the sun direction tells us the angle
	 * between the two, and thresholds on it give a disc with a soft
	 * glow around it, without a single square root.
	 */
	float dot = dx * sun_dx + dy * sun_dy + dz * sun_dz;

	if (dot > 0.9975f)
		return sun_color;

	if (dot > 0.96f)
		c = lerp_color(c, sun_color, (dot - 0.96f) / 0.0375f * 0.5f);

	/* Stars: a hash of the quantised direction, so they hold still. */
	if (daylight < 0.45f) {
		int sx = (int)(dx * 90.0f);
		int sy = (int)(dy * 90.0f);
		int sz = (int)(dz * 90.0f);
		uint32_t h = rnd_hash(sx * 73856093u ^ sy * 19349663u ^ sz * 83492791u);

		if ((h & 511) == 0) {
			float bright = (1.0f - daylight / 0.45f) * ((h >> 12 & 31) + 32) / 64.0f;
			c = lerp_color(c, rgb_to_rgb565(255, 255, 255), bright);
		}
	}

	/*
	 * Clouds, drawn by intersecting the ray with a plane high above the
	 * world. They must fade out towards the horizon: there, one pixel
	 * covers a huge stretch of the plane and the pattern degenerates
	 * into noise.
	 */
	if (dz > 0.08f && nether_mix < 0.5f) {
		float t2 = (CLOUD_HEIGHT - ez) / dz;

		if (t2 > 0.0f && t2 < CLOUD_RANGE) {
			int cx = (int)(ex + dx * t2 + world_time * 0.6f) & (CLOUD_SIZE - 1);
			int cy = (int)(ey + dy * t2) & (CLOUD_SIZE - 1);
			uint8_t cov = cloud_map[cy][cx];

			if (cov) {
				color_t cc =
					color_mul(rgb_to_rgb565(255, 255, 255),
						  (uint8_t)clampi((int)(daylight * 32.0f), 6, 32));
				float fade = (1.0f - t2 / CLOUD_RANGE) * (1.0f - nether_mix * 2.0f);
				c = lerp_color(c, cc, cov * fade / 300.0f);
			}
		}
	}

	return c;
}

/* Rays that leave the world see either sky or the ocean around the island. */
static color_t shade_outside(float dx, float dy, float dz, float ex, float ey, float ez)
{
	if (dz < -0.001f && ez > SEA_LEVEL + 1.0f) {
		float len = sqrtf(dx * dx + dy * dy + dz * dz);
		float t = (SEA_LEVEL + 1.0f - ez) / dz;

		if (t > 0.0f && t < 900.0f) {
			float wx = ex + dx * t;
			float wy = ey + dy * t;

			int tu = ((int)(wx * TEX_SIZE) + water_wave) & TEX_MASK;
			int tv = ((int)(wy * TEX_SIZE) + (water_wave >> 1)) & TEX_MASK;

			color_t c = block_tex[BLOCK_WATER][FACE_TOP][tv * TEX_SIZE + tu];
			c = color_mul(c, sky_shade[0]);

			/* The sea has no visible end, so fade it into the haze. */
			float dist = t * len;
			return lerp_color(c, sky_horizon, dist / (dist + 30.0f));
		}
	}

	return shade_sky(dx, dy, dz, ex, ey, ez);
}

/*
 * How much light reaches a block face, out of 32.
 *
 * Two contributions. Daylight, faded by how deeply the block is buried,
 * which we know from the height map without tracing anything. And the few
 * nearby glowing blocks, attenuated by distance squared, which is cheap
 * because we picked the shortlist once for the whole frame.
 */
static inline int shade_light(int bx, int by, int bz, float hx, float hy, float hz)
{
	int height = world_height[clampi(bx, 0, WORLD_X - 1)][clampi(by, 0, WORLD_Y - 1)];
	int depth = height - bz;
	int light = sky_shade[clampi(depth, 0, WORLD_Z)];

	for (int i = 0; i < num_active_lights; i++) {
		float dx = active_lights[i].x + 0.5f - hx;
		float dy = active_lights[i].y + 0.5f - hy;
		float dz = active_lights[i].z + 0.5f - hz;
		float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 > 169.0f)
			continue;

		/*
		 * Linear in distance squared rather than a real inverse
		 * square: no square root, and the falloff looks better on a
		 * screen with only 32 brightness levels anyway.
		 */
		light += (int)((1.0f - d2 * (1.0f / 169.0f)) * 30.0f);
	}

	return light > 32 ? 32 : light;
}

/* Turn a distance into the byte we keep in the depth buffer. */
static inline uint8_t depth_encode(float dist)
{
	int d = (int)(dist * (255.0f / VIEW_DIST));
	return (uint8_t)clampi(d, 0, 255);
}

/*
 * ---------------------------------------------------------------------------
 * Sprites
 * ---------------------------------------------------------------------------
 *
 * Everything that is not made of blocks is a flat image that turns to face
 * the camera. Projecting one takes three dot products, because our camera
 * basis is orthonormal: the forward, right and up vectors are unit length
 * and at right angles, so transforming a point into camera space is just
 * asking how far it lies along each of them.
 */
static void draw_sprite(float wx, float wy, float wz, float world_height_blocks,
			const color_t *pixels, int pw, int ph, int light, bool flash)
{
	float vx = wx - cam_ex;
	float vy = wy - cam_ey;
	float vz = wz - cam_ez;

	/* Camera space: right, up and forward components. */
	float sx = vx * cam_rx + vy * cam_ry + vz * cam_rz;
	float sy = vx * cam_ux + vy * cam_uy + vz * cam_uz;
	float sz = vx * cam_fx + vy * cam_fy + vz * cam_fz;

	/* Behind us, or so far away that fog has swallowed it. */
	if (sz < 0.25f || sz > VIEW_DIST)
		return;

	/*
	 * Perspective divide. The constants mirror exactly how the ray
	 * caster below turns a pixel into a direction, which is what keeps
	 * sprites lined up with the blocks they stand on.
	 */
	float inv = 1.0f / sz;
	float cx =
		SCREEN_W * 0.5f - 0.5f + (sx * inv) * SCREEN_W / (2.0f * cam_tan_fov * cam_aspect);
	float cy = SCREEN_H * 0.5f - 0.5f - (sy * inv) * SCREEN_H / (2.0f * cam_tan_fov);

	/* Height in pixels of something this tall at this distance. */
	float size = world_height_blocks * SCREEN_H / (2.0f * cam_tan_fov * sz);

	if (size < 1.0f)
		return;

	int half = (int)(size * 0.5f);
	int x0 = (int)cx - half;
	int x1 = (int)cx + half;
	int y0 = (int)cy - half;
	int y1 = (int)cy + half;

	if (x1 < 0 || x0 >= SCREEN_W || y1 < 0 || y0 >= SCREEN_H)
		return;

	int span = x1 - x0 + 1;

	if (span < 1)
		return;

	uint8_t depth = depth_encode(sz);
	uint8_t bright = (uint8_t)clampi(light, 4, 32);

	/* Distance haze, matching what the blocks behind it get. */
	float fog = sz > FOG_START ? (sz - FOG_START) / (VIEW_DIST - FOG_START) : 0.0f;
	fog = fog * fog;

	for (int py = y0; py <= y1; py++) {
		if (py < 0 || py >= SCREEN_H)
			continue;

		/* Nearest neighbour sampling: exactly the blocky look we want. */
		int ty = ((py - y0) * ph) / span;

		if (ty < 0 || ty >= ph)
			continue;

		for (int px = x0; px <= x1; px++) {
			if (px < 0 || px >= SCREEN_W)
				continue;

			/* The depth test: is a block already in front of us? */
			if (depth_at(px, py) < depth)
				continue;

			int tx = ((px - x0) * pw) / span;

			if (tx < 0 || tx >= pw)
				continue;

			color_t texel = pixels[ty * pw + tx];

			if (texel == TRANSPARENT)
				continue;

			color_t c = color_mul(texel, bright);

			if (flash)
				c = lerp_color(c, rgb_to_rgb565(255, 60, 60), 0.6f);

			if (fog > 0.0f)
				c = lerp_color(c, sky_horizon, fog);

			FB_PIXEL(px, py) = c;
			depth_store(px, py, depth);
		}
	}
}

/* One frame's worth of mobs and dropped items. */
static void draw_sprites(void)
{
	for (struct mob *m = mobs; m; m = m->next) {
		const struct mob_info *info = &mob_info[m->type];

		/* A little vertical bounce while walking, purely for life. */
		float bounce = fabsf(sinf(m->bob)) * 0.06f;
		float cz = m->z + info->height * 0.5f + bounce;

		int light = shade_light((int)m->x, (int)m->y, (int)m->z, m->x, m->y, m->z);

		draw_sprite(m->x, m->y, cz, info->height * 1.25f, mob_sprite[m->type], MOB_SPRITE,
			    MOB_SPRITE, light, m->hurt > 0.0f);
	}

	for (struct drop *d = drops; d; d = d->next) {
		/*
		 * Dropped items are drawn straight from the block texture,
		 * or as a plain coloured square for things that are not
		 * blocks. Either way they bob gently so they catch the eye.
		 */
		static color_t swatch[4 * 4];
		const color_t *pixels;
		int side;

		if (item_is_block(d->item)) {
			pixels = block_tex[d->item][FACE_SIDE];
			side = TEX_SIZE;
		} else {
			color_t c = rgb_to_rgb565(200, 200, 200);

			if (d->item >= ITEM_BASE && d->item < NUM_ITEM_IDS)
				c = item_info[d->item - ITEM_BASE].color;

			for (int i = 0; i < 16; i++)
				swatch[i] = c;

			/* Darker corners, so it reads as an object not a hole. */
			swatch[0] = swatch[3] = swatch[12] = swatch[15] = color_mul(c, 20);

			pixels = swatch;
			side = 4;
		}

		float bob = sinf(d->age * 2.5f) * 0.08f;
		int light = shade_light((int)d->x, (int)d->y, (int)d->z, d->x, d->y, d->z);

		draw_sprite(d->x, d->y, d->z + 0.25f + bob, 0.35f, pixels, side, side, light,
			    false);
	}
}

/*
 * ---------------------------------------------------------------------------
 * The world pass
 * ---------------------------------------------------------------------------
 */
void render_world(void)
{
	render_sky_colors(NULL, NULL, NULL);

	/*
	 * Precompute how dark it is under N blocks of rock, so the inner
	 * loop only has to index a table. Recomputed each frame because it
	 * depends on the time of day.
	 */
	for (int d = 0; d <= WORLD_Z; d++) {
		float sky = d <= 0 ? 1.0f : clampf(1.0f - d * 0.10f, 0.0f, 1.0f);

		/*
		 * An active reactor glows enough to light its own inside,
		 * which would otherwise be pitch dark: the spire buries
		 * everything under twelve blocks of netherrack.
		 */
		float v = 0.14f + sky * daylight * 0.86f + nether_mix * 0.22f;
		sky_shade[d] = (uint8_t)clampi((int)(v * 32.0f), 3, 32);
	}

	world_select_lights(player.x, player.y, player.z + PLAYER_EYE);
	water_wave = (int)(world_time * 6.0f);

	/* Start every frame with nothing drawn, so 255 means "far away". */
	memset(depth_buffer, 255, sizeof(depth_buffer));

	/* The eye, including the head bob. */
	cam_ex = player.x;
	cam_ey = player.y;
	cam_ez = player.z + PLAYER_EYE + player.bob;

	float cy = cosf(player.yaw), sy = sinf(player.yaw);
	float cp = cosf(player.pitch), sp = sinf(player.pitch);

	/*
	 * The camera basis. Forward is where we look; right is forward
	 * turned ninety degrees in the horizontal plane; up is the cross
	 * product of the two. All three are unit length and mutually
	 * perpendicular, which is what makes the sprite projection above
	 * three dot products instead of a matrix inverse.
	 */
	cam_fx = cy * cp;
	cam_fy = sy * cp;
	cam_fz = sp;
	cam_rx = sy;
	cam_ry = -cy;
	cam_rz = 0.0f;
	cam_ux = -cy * sp;
	cam_uy = -sy * sp;
	cam_uz = cp;

	cam_tan_fov = tanf(FOV_DEG * (float)M_PI / 360.0f);
	cam_aspect = (float)SCREEN_W / (float)SCREEN_H;

	int step = render_scale < 1 ? 1 : render_scale;

	float cam_x_step = 2.0f * cam_tan_fov * cam_aspect / SCREEN_W;
	float cam_y_step = -2.0f * cam_tan_fov / SCREEN_H * step;

	bool submerged = player.head_in_water;
	const color_t water_tint = rgb_to_rgb565(30, 90, 190);

	int base_mx = (int)floorf(cam_ex);
	int base_my = (int)floorf(cam_ey);
	int base_mz = (int)floorf(cam_ez);

	float frac_x = cam_ex - base_mx;
	float frac_y = cam_ey - base_my;
	float frac_z = cam_ez - base_mz;

	/*
	 * Direction at the top of a column. Walking down a column is then a
	 * single vector addition per pixel instead of three trig-free but
	 * still costly multiply-adds, which matters when the loop runs
	 * nineteen thousand times a frame.
	 */
	float cam_y_top = (SCREEN_H * 0.5f - 0.5f) * (2.0f * cam_tan_fov / SCREEN_H);

	float ax = cam_ux * cam_y_step;
	float ay = cam_uy * cam_y_step;
	float az = cam_uz * cam_y_step;

	for (int px = 0; px < SCREEN_W; px += step) {
		float cam_x = (px + 0.5f - SCREEN_W * 0.5f) * cam_x_step;

		float bx = cam_fx + cam_rx * cam_x + cam_ux * cam_y_top;
		float by = cam_fy + cam_ry * cam_x + cam_uy * cam_y_top;
		float bz = cam_fz + cam_rz * cam_x + cam_uz * cam_y_top;

		for (int py = 0; py < SCREEN_H; py += step) {
			float dx = bx, dy = by, dz = bz;

			bx += ax;
			by += ay;
			bz += az;

			int mx = base_mx, my = base_my, mz = base_mz;

			/* Set up the DDA. See world.c for what these mean. */
			float ddx = (dx == 0.0f) ? 1e30f : fabsf(1.0f / dx);
			float ddy = (dy == 0.0f) ? 1e30f : fabsf(1.0f / dy);
			float ddz = (dz == 0.0f) ? 1e30f : fabsf(1.0f / dz);

			int stepx = dx < 0.0f ? -1 : 1;
			int stepy = dy < 0.0f ? -1 : 1;
			int stepz = dz < 0.0f ? -1 : 1;

			float tx = (dx < 0.0f ? frac_x : 1.0f - frac_x) * ddx;
			float ty = (dy < 0.0f ? frac_y : 1.0f - frac_y) * ddy;
			float tz = (dz < 0.0f ? frac_z : 1.0f - frac_z) * ddz;

			/* Up to two panes of glass or water in front of a wall. */
			color_t layer_col[2];
			uint8_t layer_amt[2];
			int layers = 0;

			color_t color = 0;
			bool solid = false;
			float hit_dist = VIEW_DIST;

			for (int i = 0; i < MAX_STEPS; i++) {
				int side;
				float dist;

				/* Step to whichever wall is nearest. */
				if (tx < ty && tx < tz) {
					mx += stepx;
					dist = tx;
					tx += ddx;
					side = 0;
				} else if (ty < tz) {
					my += stepy;
					dist = ty;
					ty += ddy;
					side = 1;
				} else {
					mz += stepz;
					dist = tz;
					tz += ddz;
					side = 2;
				}

				if (dist > VIEW_DIST)
					break;

				if ((unsigned)mx >= WORLD_X || (unsigned)my >= WORLD_Y ||
				    (unsigned)mz >= WORLD_Z)
					break;

				/* Climbing above everything: only sky from here. */
				if (stepz > 0 && mz >= world_top)
					break;

				uint8_t block = world_data[world_index(mx, my, mz)];

				if (!block)
					continue;

				uint8_t alpha = 0;

				if (block == BLOCK_WATER) {
					/* From inside, water should not hide the view. */
					if (submerged)
						continue;

					alpha = ALPHA_WATER;
				} else if (block == BLOCK_GLASS) {
					alpha = ALPHA_GLASS;
				}

				/* Exactly where on the block did we land. */
				float hx = cam_ex + dx * dist;
				float hy = cam_ey + dy * dist;
				float hz = cam_ez + dz * dist;

				/*
				 * Texture coordinates. Which two of the three
				 * hit coordinates we use depends on which
				 * face we came through, and one of them is
				 * flipped so the texture is not mirrored on
				 * opposite faces.
				 */
				int face;
				float u, v;

				if (side == 0) {
					face = FACE_SIDE;
					u = hy - (int)hy;
					v = 1.0f - (hz - (int)hz);
					if (stepx < 0)
						u = 1.0f - u;
				} else if (side == 1) {
					face = FACE_SIDE;
					u = hx - (int)hx;
					v = 1.0f - (hz - (int)hz);
					if (stepy > 0)
						u = 1.0f - u;
				} else {
					face = stepz > 0 ? FACE_BOTTOM : FACE_TOP;
					u = hx - (int)hx;
					v = hy - (int)hy;
				}

				int tu = (int)(u * TEX_SIZE) & TEX_MASK;
				int tv = (int)(v * TEX_SIZE) & TEX_MASK;

				/* Liquids scroll their texture so they look alive. */
				if (block == BLOCK_WATER || block == BLOCK_LAVA) {
					tu = (tu + water_wave) & TEX_MASK;
					tv = (tv + (water_wave >> 1)) & TEX_MASK;
				}

				color_t texel = block_tex[block][face][tv * TEX_SIZE + tu];

				/* Which empty voxel are we looking in from? */
				int lx = mx, ly = my, lz = mz;

				if (side == 0)
					lx -= stepx;
				else if (side == 1)
					ly -= stepy;
				else
					lz -= stepz;

				int lit = shade_light(lx, ly, lz, hx, hy, hz);
				int shade = side == 0 ? SHADE_X :
							(side == 1 ? SHADE_Y :
								     (stepz > 0 ? SHADE_BOTTOM :
										  SHADE_TOP));
				int total = (shade * lit) >> 5;

				/* Blocks that glow are never in shadow. */
				if (block_info[block].light >= 12)
					total = 32;

				color_t c = color_mul(texel, (uint8_t)clampi(total, 1, 32));

				/* Outline and cracks on the block we are aiming at. */
				if (player.has_target && mx == player.tx && my == player.ty &&
				    mz == player.tz) {
					if (u < 0.035f || u > 0.965f || v < 0.035f || v > 0.965f) {
						c = color_mul(c, 6);
					} else if (player.mining > 0.0f &&
						   player.mining_need > 0.0f) {
						int st = clampi((int)(player.mining /
								      player.mining_need *
								      CRACK_STAGES),
								0, CRACK_STAGES - 1);

						if (crack_map[st][tv * TEX_SIZE + tu])
							c = color_mul(c, 12);
					}
				}

				/* Fade into the sky with distance. */
				if (dist > FOG_START) {
					float f = (dist - FOG_START) / (VIEW_DIST - FOG_START);
					c = lerp_color(c, sky_horizon, f * f);
				}

				if (!alpha) {
					color = c;
					solid = true;
					hit_dist = dist;
					break;
				}

				if (layers < 2) {
					layer_col[layers] = c;
					layer_amt[layers] = alpha;
					layers++;
				}
			}

			if (!solid)
				color = shade_outside(dx, dy, dz, cam_ex, cam_ey, cam_ez);

			/* Composite the transparent layers, furthest one first. */
			while (layers--)
				color = color_blend(color, layer_col[layers], layer_amt[layers]);

			if (submerged)
				color = color_blend(color, water_tint, 11);

			uint8_t depth = depth_encode(hit_dist);

			if (step == 1) {
				FB_PIXEL(px, py) = color;
				depth_store(px, py, depth);
			} else {
				for (int oy = 0; oy < step && py + oy < SCREEN_H; oy++) {
					for (int ox = 0; ox < step && px + ox < SCREEN_W; ox++) {
						FB_PIXEL(px + ox, py + oy) = color;
						depth_store(px + ox, py + oy, depth);
					}
				}
			}
		}
	}

	draw_sprites();
}

/* Dim a rectangle of the frame buffer, for menus and overlays. */
void render_darken(int x0, int y0, int x1, int y1, uint8_t amount)
{
	x0 = clampi(x0, 0, SCREEN_W - 1);
	x1 = clampi(x1, 0, SCREEN_W - 1);
	y0 = clampi(y0, 0, SCREEN_H - 1);
	y1 = clampi(y1, 0, SCREEN_H - 1);

	for (int x = x0; x <= x1; x++)
		for (int y = y0; y <= y1; y++)
			FB_PIXEL(x, y) = color_mul(FB_PIXEL(x, y), amount);
}
