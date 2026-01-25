#include <stdio.h>
#include <string.h> // for memset
#include "common.h"

// --------------------------------------------------------
// Temporary structures matching the OLD editor save format
// --------------------------------------------------------
typedef struct {
	int ws, we;  // wall start, wall end
	int z1, z2;  // bottom height, top height
	int st, ss;  // surface texture, scale
	int surf[1]; // Unused in load, but kept for struct alignment if needed
} LegacySector;

typedef struct {
	int x1, y1;
	int x2, y2;
	int wt, u, v;
	int shade;
} LegacyWall;

// Max buffers for loading the raw file
#define MAX_LEGACY_SECTORS 128
#define MAX_LEGACY_WALLS 256

void legacy_load_sectors(void)
{
	FILE *fp = fopen("level.h", "r");
	if (fp == NULL) {
		printf("Error: Could not open level.h\n");
		return;
	}

	// Temporary storage for the old data
	LegacySector tempS[MAX_LEGACY_SECTORS];
	LegacyWall tempW[MAX_LEGACY_WALLS];
	int numSectLegacy = 0;
	int numWallLegacy = 0;
	int s, w;

	// 1. READ SECTORS (Old Format)
	if (fscanf(fp, "%i", &numSectLegacy) != 1) {
		fclose(fp);
		return;
	}

	for (s = 0; s < numSectLegacy && s < MAX_LEGACY_SECTORS; s++) {
		fscanf(fp, "%i", &tempS[s].ws);
		fscanf(fp, "%i", &tempS[s].we);
		fscanf(fp, "%i", &tempS[s].z1);
		fscanf(fp, "%i", &tempS[s].z2);
		fscanf(fp, "%i", &tempS[s].st);
		fscanf(fp, "%i", &tempS[s].ss);
	}

	// 2. READ WALLS (Old Format)
	if (fscanf(fp, "%i", &numWallLegacy) != 1) {
		fclose(fp);
		return;
	}

	for (w = 0; w < numWallLegacy && w < MAX_LEGACY_WALLS; w++) {
		fscanf(fp, "%i", &tempW[w].x1);
		fscanf(fp, "%i", &tempW[w].y1);
		fscanf(fp, "%i", &tempW[w].x2);
		fscanf(fp, "%i", &tempW[w].y2);
		fscanf(fp, "%i", &tempW[w].wt);
		fscanf(fp, "%i", &tempW[w].u);
		fscanf(fp, "%i", &tempW[w].v);
		fscanf(fp, "%i", &tempW[w].shade);
	}

	// 3. READ PLAYER (From the save function logic)
	// The save function puts P.x, P.y, etc at the end.
	fscanf(fp, "%i %i %i %i %i", &P.x, &P.y, &P.z, &P.a, &P.l);

	fclose(fp);

	// --------------------------------------------------------
	// CONVERT TO NEW STRUCTURE (current_map)
	// --------------------------------------------------------

	// Reset current map
	memset(&current_map, 0, sizeof(struct Map));
	current_map.num_sectors = numSectLegacy;

	for (s = 0; s < current_map.num_sectors; s++) {
		struct Sector *newSect = &current_map.sectors[s];
		LegacySector *oldSect = &tempS[s];

		// Map Sector Properties
		newSect->floor.texture.texture_index = oldSect->st;
		newSect->floor.walkable = 1;

		// Determine number of walls in this sector
		int start = oldSect->ws;
		int end = oldSect->we;
		int wall_count = end - start;

		// Safety check for fixed array size
		if (wall_count > MAX_WALLS_PER_SECTOR) {
			printf("Warning: Sector %d has too many walls (%d). Truncating to %d.\n", s,
			       wall_count, MAX_WALLS_PER_SECTOR);
			wall_count = MAX_WALLS_PER_SECTOR;
		}

		// Loop through the walls belonging to this sector
		for (int i = 0; i < wall_count; i++) {
			int legacyIndex = start + i;
			if (legacyIndex >= numWallLegacy)
				break;

			LegacyWall *oldWall = &tempW[legacyIndex];
			struct WallProperties *newWall = &newSect->walls[i];

			newWall->active = 1;
			newWall->solid = 1; // Default to solid

			// Map Coordinates
			newWall->p1_x = oldWall->x1;
			newWall->p1_y = oldWall->y1;
			newWall->p2_x = oldWall->x2;
			newWall->p2_y = oldWall->y2;

			// Map Texture
			newWall->texture.texture_index = oldWall->wt;
			newWall->texture.shade = oldWall->shade;
			newWall->texture_scale = oldWall->u; // Assuming 'u' was scale

			// Map Heights
			// Note: In new struct, walls have height. In old struct, sectors had height.
			// We copy the sector height into the wall height.
			newWall->bottom_height = oldSect->z1;
			newWall->top_height = oldSect->z2;
		}
	}

	printf("Map loaded successfully: %d Sectors\n", current_map.num_sectors);
}
