#pragma once
#include <sdk.h>
#include <stdint.h>
#include <limits.h>
#include "../../src/experiment/tile.h"

/* Available scenes. */
extern sdk_scene_t scene_root;
extern sdk_scene_t scene_level;
extern sdk_scene_t scene_target;
extern sdk_scene_t scene_target;

/*
 * List of existing map identifiers.
 * Determined when root is pushed, i.e. when the app starts.
 */
extern uint8_t map_ids[MAX_MAPS];
extern int map_count;

/* No level in particular. */
#define NO_LEVEL INT_MAX

/*
 * What is the user pointing at, including the corresponding map data.
 */
struct cursor {
	/* Level ID and cursor tile coordinates. */
	int level;
	int px, py;

	/*
	 * Indicator for what level we have loaded data the last time.
	 * Use load_level() to update to match `level`.
	 *
	 * When set to `-level`, it indicates that we have attempted
	 * but failed to load that particular level data.
	 */
	int has_level;

	/* Level tiles. */
	Tile map[MAP_ROWS][MAP_COLS];
};

/* Cursor of the root/level editor scenes. */
extern struct cursor root_cursor;

/* Cursor of the target selection scene. */
extern struct cursor target_cursor;

/* Load correct level data for given cursor. */
void load_level(struct cursor *cursor);

/* Save level data for given cursor. */
void save_level(struct cursor *cursor);

/* Load `map_ids` and `map_count`. */
void load_map_ids(void);
