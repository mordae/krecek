#include "common.h"
#include <stdlib.h>
#include <stdio.h>

// Assets
#include "assets/T_00.h"
#include "assets/T_01.h"
#include "assets/T_02.h"
#include "assets/T_03.h"
#include "assets/T_04.h"
#include "assets/T_05.h"
#include "assets/T_06.h"
#include "assets/T_07.h"
#include "assets/T_08.h"
#include "assets/T_09.h"
#include "assets/T_10.h"
#include "assets/T_11.h"
#include "assets/T_12.h"
#include "assets/T_13.h"
#include "assets/T_14.h"
#include "assets/T_15.h"
#include "assets/T_16.h"
#include "assets/T_17.h"
#include "assets/T_18.h"
#include "assets/T_19.h"

extern void drawpixel(int x, int y, int r, int g, int b);

// Replacement for S[s].surf[x] since it is missing in the new struct
static int16_t sector_surf[MAX_NUMBER_SECTORS][TFT_WIDTH];

static void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2)
{
	float da = *y1;
	float db = y2;
	float d = da - db;
	if (d == 0)
		d = 1;

	float s = da / (da - db);
	*x1 = *x1 + s * (x2 - (*x1));
	*y1 = *y1 + s * (y2 - (*y1));
	if (*y1 == 0)
		*y1 = 1;
	*z1 = *z1 + s * (z2 - (*z1));
}

static void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack)
{
	struct Sector *sect = &current_map.sectors[s];
	struct WallProperties *wall = &sect->walls[w];

	// wall texture
	int wt = wall->texture.texture_index;
	if (wt < 0 || wt >= 64)
		wt = 0;

	// Mapping 'u' from old code to 'texture_scale' in new code.
	// If texture_scale is 0, default to 1 to avoid divide by zero or bad scaling.
	float u_val = (wall->texture_scale > 0) ? (float)wall->texture_scale : 1.0f;
	float v_val = u_val; // Assuming uniform scaling since 'v' is missing in new struct

	float ht = 0, ht_step = (float)Textures[wt].w * u_val / (float)(x2 - x1);

	int x, y;
	//Hold diffrent
	int dyb = b2 - b1;
	int dyt = t2 - t1;
	int dx = x2 - x1;
	if (dx == 0) {
		dx = 1;
	}
	int xs = x1;
	//CLIP X
	if (x1 < 0) {
		ht -= ht_step * x1;
		x1 = 0;
	}
	if (x2 < 0) {
		x2 = 0;
	}
	if (x1 > TFT_WIDTH) {
		x1 = TFT_WIDTH;
	}
	if (x2 > TFT_WIDTH) {
		x2 = TFT_WIDTH;
	}

	//draw x ver. line
	for (x = x1; x < x2; x++) {
		int y1 = dyb * (x - xs + 0.5) / dx + b1;
		int y2 = dyt * (x - xs + 0.5) / dx + t1;
		if ((y2 - y1) == 0) {
			continue; // Skip
		}

		float vt = 0, vt_step = (float)Textures[wt].h * v_val / (float)(y2 - y1);
		int clipped_y = 0;
		if (y1 < 0) {
			clipped_y = 0 - y1;
			vt += clipped_y * vt_step;
			y1 = 0;
		}
		if (y2 < 0) {
			y2 = 0;
		}
		if (y1 > TFT_HEIGHT) {
			y1 = TFT_HEIGHT;
		}
		if (y2 > TFT_HEIGHT) {
			y2 = TFT_HEIGHT;
		}
		//draw front wall
		if (frontBack == 0) {
			if (sect->surface == 1) {
				sector_surf[s][x] = y1; // WAS: S[s].surf[x] = y1;
			}
			if (sect->surface == 2) {
				sector_surf[s][x] = y2; // WAS: S[s].surf[x] = y2;
			}

			for (y = y1; y < y2; y++) {
				int tex_x = (int)ht % Textures[wt].w;
				int tex_y = (int)vt % Textures[wt].h;
				int pixel = tex_y * 3 * Textures[wt].w + tex_x * 3;

				int shade = wall->texture.shade / 4;
				int r = Textures[wt].name[pixel + 0] - shade;
				if (r < 0) {
					r = 0;
				}
				int g = Textures[wt].name[pixel + 1] - shade;
				if (g < 0) {
					g = 0;
				}
				int b = Textures[wt].name[pixel + 2] - shade;
				if (b < 0) {
					b = 0;
				}
				drawpixel(x, y, r, g, b);
				vt += vt_step;
			}
			ht += ht_step;
		}
		if (frontBack == 1) {
			int sector_index = s;
			int tex_index =
				sect->floor.texture.texture_index; // WAS: S[sector_index].st

			if (tex_index < 0 || tex_index >= 64) {
				continue;
			}

			const TextureMaps *tex = &Textures[tex_index];
			// WAS: float tile = (float)S[sector_index].ss * 7.0f;
			// Since .ss is missing, defaulting to 1 * 7.0f or hardcoded 7.0f
			float tile = 7.0f;
			if (tile <= 0.0f) {
				tile = 7.0f;
			}

			int xo = TFT_WIDTH2;  // x offset (screen center)
			int yo = TFT_HEIGHT2; // y offset (screen center)
			float fov = FOV;      // field of view
			int x2_pos = x - xo;  // renamed local var to x2_pos to avoid conflict

			int wo = 0; // world floor/ceiling height reference
			if (sect->surface == 1) {
				// bottom surface (floor)
				y2 = sector_surf[s][x];	  // WAS: S[s].surf[x];
				wo = wall->bottom_height; // WAS: S[s].z1;
			}
			if (sect->surface == 2) {
				// top surface (ceiling)
				y1 = sector_surf[s][x]; // WAS: S[s].surf[x];
				wo = wall->top_height;	// WAS: S[s].z2;
			}

			float lookUpDown = -P.l * 6.2f;
			if (lookUpDown > TFT_HEIGHT) {
				lookUpDown = TFT_HEIGHT;
			}
			if (lookUpDown < -TFT_HEIGHT) {
				lookUpDown = -TFT_HEIGHT;
			}

			// Player height relative to the floor/ceiling plane.
			float moveUpDown = (float)(P.z - wo) / (float)yo;
			if (moveUpDown == 0.0f) {
				moveUpDown = 0.001f;
			}

			int ys = y1 - yo;
			int ye = y2 - yo;

			// FIX: Clip floor/ceiling drawing to horizon to avoid divide-by-zero or inversion
			// The horizon in screen-relative Y coordinates (y = PixelY - yo) is at -lookUpDown.
			// We must ensure 'z' (which is y + lookUpDown) never crosses 0.

			int hor_y = (int)(-lookUpDown);

			if (moveUpDown > 0.0f) {
				// Floor mode: We need z > 0 => y > -lookUpDown
				// Clip top of strip (ys) to be below horizon
				if (ys <= hor_y) {
					ys = hor_y + 1;
				}
			} else {
				// Ceiling mode: We need z < 0 => y < -lookUpDown
				// Clip bottom of strip (ye) to be above horizon
				if (ye >= hor_y) {
					ye = hor_y - 1;
				}
			}

			// Safety check if clipping emptied the range
			if (ys >= ye) {
				continue;
			}

			for (y = ys; y < ye; y++) {
				float z = y + lookUpDown;
				if (z == 0.0f) {
					z = 0.0001f;
				}

				// World floor/ceiling coordinates before rotation.
				float fx = x2_pos / z * moveUpDown * tile;
				float fy = fov / z * moveUpDown * tile;

				// Rotate and offset by player position (matching the example).
				float rx = fx * M.sin[P.a] - fy * M.cos[P.a] + (P.y / 60.0f * tile);
				float ry = fx * M.cos[P.a] + fy * M.sin[P.a] - (P.x / 60.0f * tile);

				// Remove negative values as in the example.
				if (rx < 0.0f) {
					rx = -rx + 1.0f;
				}
				if (ry < 0.0f) {
					ry = -ry + 1.0f;
				}

				// Map to texture coordinates.
				int tw = tex->w;
				int th = tex->h;
				if (tw <= 0 || th <= 0) {
					continue;
				}

				int u = (int)rx;
				int v = (int)ry;
				u = (u % tw + tw) % tw;
				v = (v % th + th) % th;

				int pixel = v * 3 * tw + u * 3;
				int r = tex->name[pixel + 0];
				int g = tex->name[pixel + 1];
				int b = tex->name[pixel + 2];

				int sy = y + yo;
				int sx = x2_pos + xo;
				if (sx >= 0 && sx < TFT_WIDTH && sy >= 0 && sy < TFT_HEIGHT) {
					drawpixel(sx, sy, r, g, b);
				}
			} // textured floor/ceiling
		}
	}
}

static void update_sector_distances(void)
{
	float CS = M.cos[P.a], SN = M.sin[P.a];

	for (int s = 0; s < current_map.num_sectors; s++) {
		// Compute a view-space depth for the sector based on the average
		// Y coordinate (along the view direction) of its wall midpoints.
		float sum_depth = 0.0f;
		int count = 0;

		for (int w = 0; w < MAX_WALLS_PER_SECTOR; w++) {
			if (!current_map.sectors[s].walls[w].active)
				continue;

			struct WallProperties *wall = &current_map.sectors[s].walls[w];
			int x1 = wall->p1_x - P.x;
			int y1 = wall->p1_y - P.y;
			int x2 = wall->p2_x - P.x;
			int y2 = wall->p2_y - P.y;

			//int wx0 = x1 * CS - y1 * SN;
			int wy0 = y1 * CS + x1 * SN;
			//int wx1 = x2 * CS - y2 * SN;
			int wy1 = y2 * CS + x2 * SN;

			int mid_y = (wy0 + wy1) / 2;
			sum_depth += (float)mid_y;
			count++;
		}

		if (count > 0) {
			current_map.sectors[s].distance = sum_depth / (float)count;
		} else {
			current_map.sectors[s].distance = 0.0f;
		}
	}
}

void draw_3d(void)
{
	int x, s, w, frontBack, cycles, wx[4], wy[4], wz[4];
	float CS = M.cos[P.a], SN = M.sin[P.a];

	// First compute current distances for all sectors.
	update_sector_distances();

	// Create an index array to sort, protecting the actual sector structs from being moved
	int draw_order[MAX_NUMBER_SECTORS];
	for (int i = 0; i < current_map.num_sectors; i++) {
		draw_order[i] = i;
	}

	// Order the indices based on distance (Bubble Sort)
	for (int i = 0; i < current_map.num_sectors - 1; i++) {
		for (int j = 0; j < current_map.num_sectors - i - 1; j++) {
			int idxA = draw_order[j];
			int idxB = draw_order[j + 1];

			// Compare distances using the indices
			if (current_map.sectors[idxA].distance <
			    current_map.sectors[idxB].distance) {
				// Swap indices only
				draw_order[j] = idxB;
				draw_order[j + 1] = idxA;
			}
		}
	}

	//draw sectors
	for (int i = 0; i < current_map.num_sectors; i++) {
		s = draw_order[i]; // Get the actual sector index to draw
		struct Sector *sect = &current_map.sectors[s];

		// Find floor/ceil heights from the first active wall (since they are stored in walls now)
		int32_t z1 = 0, z2 = 0;
		for (int k = 0; k < MAX_WALLS_PER_SECTOR; k++) {
			if (sect->walls[k].active) {
				z1 = sect->walls[k].bottom_height;
				z2 = sect->walls[k].top_height;
				break;
			}
		}

		if (P.z < z1) {
			sect->surface = 1;
			cycles = 2;
			for (x = 0; x < TFT_WIDTH; x++) {
				sector_surf[s][x] = TFT_HEIGHT; // WAS: S[s].surf[x]
			}
		} else if (P.z > z2) {
			sect->surface = 2;
			cycles = 2;
			for (x = 0; x < TFT_WIDTH; x++) {
				sector_surf[s][x] = 0; // WAS: S[s].surf[x]
			}
		} else {
			sect->surface = 0;
			cycles = 1;
		}
		for (frontBack = 0; frontBack < cycles; frontBack++) {
			for (w = 0; w < MAX_WALLS_PER_SECTOR; w++) {
				if (!sect->walls[w].active)
					continue;
				struct WallProperties *wall = &sect->walls[w];

				//offset
				int x1 = wall->p1_x - P.x;
				int y1 = wall->p1_y - P.y;
				int x2 = wall->p2_x - P.x;
				int y2 = wall->p2_y - P.y;

				//swap
				if (frontBack == 1) {
					int swp = x1;
					x1 = x2;
					x2 = swp;
					swp = y1;
					y1 = y2;
					y2 = swp;
				}

				wx[0] = x1 * CS - y1 * SN;
				wx[1] = x2 * CS - y2 * SN;
				wx[2] = wx[0];
				wx[3] = wx[1];

				wy[0] = y1 * CS + x1 * SN;
				wy[1] = y2 * CS + x2 * SN;
				wy[2] = wy[0];
				wy[3] = wy[1];

				wz[0] = wall->bottom_height - P.z +
					((P.l * wy[0]) / 32.0); // WAS S[s].z1
				wz[1] = wall->bottom_height - P.z + ((P.l * wy[1]) / 32.0);
				wz[2] = wall->top_height - P.z +
					((P.l * wy[0]) / 32.0); // WAS S[s].z2
				wz[3] = wall->top_height - P.z + ((P.l * wy[1]) / 32.0);

				if (wy[0] < 1 && wy[1] < 1) {
					continue;
				}

				if (wy[0] < 1) {
					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1],
							 wz[1]); //bottom line
					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3],
							 wz[3]); //top line
				}

				if (wy[1] < 1) {
					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0],
							 wz[0]); //bottom line
					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2],
							 wz[2]); //top line
				}

				wx[0] = wx[0] * FOV / wy[0] + TFT_WIDTH2;
				wy[0] = wz[0] * FOV / wy[0] + TFT_HEIGHT2;
				wx[1] = wx[1] * FOV / wy[1] + TFT_WIDTH2;
				wy[1] = wz[1] * FOV / wy[1] + TFT_HEIGHT2;

				wx[2] = wx[2] * FOV / wy[2] + TFT_WIDTH2;
				wy[2] = wz[2] * FOV / wy[2] + TFT_HEIGHT2;
				wx[3] = wx[3] * FOV / wy[3] + TFT_WIDTH2;
				wy[3] = wz[3] * FOV / wy[3] + TFT_HEIGHT2;

				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], s, w, frontBack);
			}
			//int numWallsInSector = S[s].we - S[s].ws;
		}
	}
}

void textures_load()
{
	Textures[0].h = T_00_HEIGHT;
	Textures[0].w = T_00_WIDTH;
	Textures[1].h = T_01_HEIGHT;
	Textures[1].w = T_01_WIDTH;
	Textures[2].h = T_02_HEIGHT;
	Textures[2].w = T_02_WIDTH;
	Textures[3].h = T_03_HEIGHT;
	Textures[3].w = T_03_WIDTH;
	Textures[4].h = T_04_HEIGHT;
	Textures[4].w = T_04_WIDTH;
	Textures[5].h = T_05_HEIGHT;
	Textures[5].w = T_05_WIDTH;
	Textures[6].h = T_06_HEIGHT;
	Textures[6].w = T_06_WIDTH;
	Textures[7].h = T_07_HEIGHT;
	Textures[7].w = T_07_WIDTH;
	Textures[8].h = T_08_HEIGHT;
	Textures[8].w = T_08_WIDTH;
	Textures[9].h = T_09_HEIGHT;
	Textures[9].w = T_09_WIDTH;
	Textures[10].h = T_10_HEIGHT;
	Textures[10].w = T_10_WIDTH;
	Textures[11].h = T_11_HEIGHT;
	Textures[11].w = T_11_WIDTH;
	Textures[12].h = T_12_HEIGHT;
	Textures[12].w = T_12_WIDTH;
	Textures[13].h = T_13_HEIGHT;
	Textures[13].w = T_13_WIDTH;
	Textures[14].h = T_14_HEIGHT;
	Textures[14].w = T_14_WIDTH;
	Textures[15].h = T_15_HEIGHT;
	Textures[15].w = T_15_WIDTH;
	Textures[16].h = T_16_HEIGHT;
	Textures[16].w = T_16_WIDTH;
	Textures[17].h = T_17_HEIGHT;
	Textures[17].w = T_17_WIDTH;
	Textures[18].h = T_18_HEIGHT;
	Textures[18].w = T_18_WIDTH;
	Textures[19].h = T_19_HEIGHT;
	Textures[19].w = T_19_WIDTH;
	Textures[0].name = (const unsigned char *)T_00;
	Textures[1].name = (const unsigned char *)T_01;
	Textures[2].name = (const unsigned char *)T_02;
	Textures[3].name = (const unsigned char *)T_03;
	Textures[4].name = (const unsigned char *)T_04;
	Textures[5].name = (const unsigned char *)T_05;
	Textures[6].name = (const unsigned char *)T_06;
	Textures[7].name = (const unsigned char *)T_07;
	Textures[8].name = (const unsigned char *)T_08;
	Textures[9].name = (const unsigned char *)T_09;
	Textures[10].name = (const unsigned char *)T_10;
	Textures[11].name = (const unsigned char *)T_11;
	Textures[12].name = (const unsigned char *)T_12;
	Textures[13].name = (const unsigned char *)T_13;
	Textures[14].name = (const unsigned char *)T_14;
	Textures[15].name = (const unsigned char *)T_15;
	Textures[16].name = (const unsigned char *)T_16;
	Textures[17].name = (const unsigned char *)T_17;
	Textures[18].name = (const unsigned char *)T_18;
	Textures[19].name = (const unsigned char *)T_19;
}
