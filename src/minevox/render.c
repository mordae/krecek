#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * One ray per pixel through a voxel DDA, in the spirit of the classic
 * grid raycasters, but stepping in all three axes.
 */

#define FOV_DEG 75.0f
#define VIEW_DIST 38.0f
#define FOG_START 20.0f
#define MAX_STEPS 56

/* How much of a block is left after we look at it through this one. */
#define ALPHA_WATER 21
#define ALPHA_GLASS 9

int render_scale = 1;

#if TFT_SWAP_XY
#define FB_PIXEL(x, y) tft_input[x][y]
#else
#define FB_PIXEL(x, y) tft_input[y][x]
#endif

/*
 * Brightness of a face, out of 32. The sun comes from above and the two
 * horizontal directions differ a little, which is what gives blocks their
 * shape without any real lighting.
 */
#define SHADE_TOP 32
#define SHADE_BOTTOM 16
#define SHADE_X 20
#define SHADE_Y 26

/* Clouds live on a plane this high and are not drawn beyond this range. */
#define CLOUD_HEIGHT 48.0f
#define CLOUD_RANGE 320.0f

/* Cloud coverage, tiled over the sky. */
#define CLOUD_SIZE 64
static uint8_t cloud_map[CLOUD_SIZE][CLOUD_SIZE];

/* Crack overlay for the block being mined. */
#define CRACK_STAGES 4
static uint8_t crack_map[CRACK_STAGES][TEX_SIZE * TEX_SIZE];

/* Recomputed every frame. */
static color_t sky_zenith, sky_horizon, sky_ground, sun_color;
static float daylight;
static float sun_dx, sun_dy, sun_dz;
static uint8_t sky_shade[WORLD_Z + 1];
static int water_wave;

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
	/* Blobby clouds from a couple of smoothed random layers. */
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

	/* Crack stages, each one adding more chips. */
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

void render_sky_colors(color_t *zenith, color_t *horizon, float *light)
{
	float phase = world_time / DAY_LENGTH;
	float sun_alt = sinf(phase * 2.0f * (float)M_PI);

	daylight = clampf(sun_alt * 1.8f + 0.42f, 0.10f, 1.0f);

	/* How close we are to sunrise or sunset. */
	float dusk = clampf(1.0f - fabsf(sun_alt) * 3.2f, 0.0f, 1.0f);

	const color_t day_zenith = rgb_to_rgb565(62, 130, 226);
	const color_t day_horizon = rgb_to_rgb565(164, 201, 240);
	const color_t night_zenith = rgb_to_rgb565(6, 8, 26);
	const color_t night_horizon = rgb_to_rgb565(22, 26, 56);
	const color_t dusk_horizon = rgb_to_rgb565(236, 132, 62);

	float f = clampf((daylight - 0.10f) / 0.90f, 0.0f, 1.0f);

	sky_zenith = lerp_color(night_zenith, day_zenith, f);
	sky_horizon = lerp_color(night_horizon, day_horizon, f);
	sky_horizon = lerp_color(sky_horizon, dusk_horizon, dusk * 0.75f);
	sky_ground = color_mul(sky_horizon, 14);
	sun_color = sun_alt > -0.1f ? rgb_to_rgb565(255, 246, 214) : rgb_to_rgb565(224, 228, 240);

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

/* Color of the sky in a given direction. */
static color_t shade_sky(float dx, float dy, float dz, float ex, float ey, float ez)
{
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

	/* Sun or moon disc with a soft glow around it. */
	float dot = dx * sun_dx + dy * sun_dy + dz * sun_dz;

	if (dot > 0.9975f)
		return sun_color;

	if (dot > 0.96f)
		c = lerp_color(c, sun_color, (dot - 0.96f) / 0.0375f * 0.5f);

	/* Stars, only once it gets dark enough. */
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
	 * Clouds drifting over a plane high above the world. They have to
	 * fade out towards the horizon, where a single pixel would cover
	 * far more than one cloud cell and turn into noise.
	 */
	if (dz > 0.08f) {
		float t2 = (CLOUD_HEIGHT - ez) / dz;

		if (t2 > 0.0f && t2 < CLOUD_RANGE) {
			int cx = (int)(ex + dx * t2 + world_time * 0.6f) & (CLOUD_SIZE - 1);
			int cy = (int)(ey + dy * t2) & (CLOUD_SIZE - 1);
			uint8_t cov = cloud_map[cy][cx];

			if (cov) {
				color_t cc =
					color_mul(rgb_to_rgb565(255, 255, 255),
						  (uint8_t)clampi((int)(daylight * 32.0f), 6, 32));
				float fade = 1.0f - t2 / CLOUD_RANGE;
				c = lerp_color(c, cc, cov * fade / 300.0f);
			}
		}
	}

	return c;
}

/*
 * Rays that leave the world either see the sky or the endless ocean
 * our island floats in.
 */
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

			/* Fade into the horizon, the sea has no visible end. */
			float dist = t * len;
			return lerp_color(c, sky_horizon, dist / (dist + 30.0f));
		}
	}

	return shade_sky(dx, dy, dz, ex, ey, ez);
}

/* Light reaching a block face, out of 32. */
static inline int shade_light(int bx, int by, int bz, float hx, float hy, float hz)
{
	int height = world_height[bx & (WORLD_X - 1)][by & (WORLD_Y - 1)];
	int depth = height - bz;
	int light = sky_shade[clampi(depth, 0, WORLD_Z)];

	for (int i = 0; i < num_active_lights; i++) {
		float dx = active_lights[i].x + 0.5f - hx;
		float dy = active_lights[i].y + 0.5f - hy;
		float dz = active_lights[i].z + 0.5f - hz;
		float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 > 169.0f)
			continue;

		light += (int)((1.0f - d2 * (1.0f / 169.0f)) * 30.0f);
	}

	return light > 32 ? 32 : light;
}

void render_world(void)
{
	render_sky_colors(NULL, NULL, NULL);

	/* Precompute how dark it gets under N blocks of rock. */
	for (int d = 0; d <= WORLD_Z; d++) {
		float sky = d <= 0 ? 1.0f : clampf(1.0f - d * 0.10f, 0.0f, 1.0f);
		float v = 0.14f + sky * daylight * 0.86f;
		sky_shade[d] = (uint8_t)clampi((int)(v * 32.0f), 3, 32);
	}

	world_select_lights(player.x, player.y, player.z + PLAYER_EYE);
	water_wave = (int)(world_time * 6.0f);

	float ex = player.x;
	float ey = player.y;
	float ez = player.z + PLAYER_EYE + player.bob;

	float cy = cosf(player.yaw), sy = sinf(player.yaw);
	float cp = cosf(player.pitch), sp = sinf(player.pitch);

	/* Camera basis: forward, right and up. */
	float fx = cy * cp, fy = sy * cp, fz = sp;
	float rx = sy, ry = -cy, rz = 0.0f;
	float ux = -cy * sp, uy = -sy * sp, uz = cp;

	float tan_fov = tanf(FOV_DEG * (float)M_PI / 360.0f);
	float aspect = (float)SCREEN_W / (float)SCREEN_H;

	int step = render_scale < 1 ? 1 : render_scale;

	float cam_x_step = 2.0f * tan_fov * aspect / SCREEN_W;
	float cam_y_step = -2.0f * tan_fov / SCREEN_H * step;

	bool submerged = player.head_in_water;
	const color_t water_tint = rgb_to_rgb565(30, 90, 190);

	int base_mx = (int)floorf(ex);
	int base_my = (int)floorf(ey);
	int base_mz = (int)floorf(ez);

	float frac_x = ex - base_mx;
	float frac_y = ey - base_my;
	float frac_z = ez - base_mz;

	/* Direction at the top of a column, walked down by cam_y_step. */
	float cam_y_top = (SCREEN_H * 0.5f - 0.5f) * (2.0f * tan_fov / SCREEN_H);

	float ax = ux * cam_y_step;
	float ay = uy * cam_y_step;
	float az = uz * cam_y_step;

	for (int px = 0; px < SCREEN_W; px += step) {
		float cam_x = (px + 0.5f - SCREEN_W * 0.5f) * cam_x_step;

		float bx = fx + rx * cam_x + ux * cam_y_top;
		float by = fy + ry * cam_x + uy * cam_y_top;
		float bz = fz + rz * cam_x + uz * cam_y_top;

		for (int py = 0; py < SCREEN_H; py += step) {
			float dx = bx, dy = by, dz = bz;

			bx += ax;
			by += ay;
			bz += az;

			int mx = base_mx, my = base_my, mz = base_mz;

			float ddx = (dx == 0.0f) ? 1e30f : fabsf(1.0f / dx);
			float ddy = (dy == 0.0f) ? 1e30f : fabsf(1.0f / dy);
			float ddz = (dz == 0.0f) ? 1e30f : fabsf(1.0f / dz);

			int stepx = dx < 0.0f ? -1 : 1;
			int stepy = dy < 0.0f ? -1 : 1;
			int stepz = dz < 0.0f ? -1 : 1;

			float tx = (dx < 0.0f ? frac_x : 1.0f - frac_x) * ddx;
			float ty = (dy < 0.0f ? frac_y : 1.0f - frac_y) * ddy;
			float tz = (dz < 0.0f ? frac_z : 1.0f - frac_z) * ddz;

			color_t layer_col[2];
			uint8_t layer_amt[2];
			int layers = 0;

			color_t color = 0;
			bool solid = false;

			for (int i = 0; i < MAX_STEPS; i++) {
				int side;
				float dist;

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

				/* Climbing above everything, only sky from here. */
				if (stepz > 0 && mz >= world_top)
					break;

				unsigned vi = world_index(mx, my, mz);
				uint8_t pair = world_data[vi >> 1];
				uint8_t block = (vi & 1) ? (pair >> 4) : (pair & 0x0f);

				if (!block)
					continue;

				uint8_t alpha = 0;

				if (block == BLOCK_WATER) {
					if (submerged)
						continue;

					alpha = ALPHA_WATER;
				} else if (block == BLOCK_GLASS) {
					alpha = ALPHA_GLASS;
				}

				/* Where exactly did we land. */
				float hx = ex + dx * dist;
				float hy = ey + dy * dist;
				float hz = ez + dz * dist;

				int face;
				float u, v;

				/* Inside the world all three are positive. */
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

				if (block == BLOCK_WATER) {
					tu = (tu + water_wave) & TEX_MASK;
					tv = (tv + (water_wave >> 1)) & TEX_MASK;
				}

				color_t texel = block_tex[block][face][tv * TEX_SIZE + tu];

				/* Which empty voxel are we looking from. */
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

				if (block_info[block].light)
					total = 32;

				color_t c = color_mul(texel, (uint8_t)clampi(total, 1, 32));

				/* Outline and cracks on the block we are aiming at. */
				if (player.has_target && mx == player.tx && my == player.ty &&
				    mz == player.tz) {
					if (u < 0.035f || u > 0.965f || v < 0.035f || v > 0.965f) {
						c = color_mul(c, 6);
					} else if (player.mining > 0.0f) {
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
					break;
				}

				if (layers < 2) {
					layer_col[layers] = c;
					layer_amt[layers] = alpha;
					layers++;
				}
			}

			if (!solid)
				color = shade_outside(dx, dy, dz, ex, ey, ez);

			while (layers--)
				color = color_blend(color, layer_col[layers], layer_amt[layers]);

			if (submerged)
				color = color_blend(color, water_tint, 11);

			if (step == 1) {
				FB_PIXEL(px, py) = color;
			} else {
				for (int oy = 0; oy < step && py + oy < SCREEN_H; oy++)
					for (int ox = 0; ox < step && px + ox < SCREEN_W; ox++)
						FB_PIXEL(px + ox, py + oy) = color;
			}
		}
	}
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
