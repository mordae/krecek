#pragma once
#include "common.h"
#include "pool.h"

typedef struct Room {
	int x, y, w, h;
	int cx, cy;
	int spawner_x, spawner_y;
	float spawn_timer;
	int spawns_left;
	bool has_spawner;
} Room;

extern uint8_t cur_map[DUNGEON_ROWS][DUNGEON_COLS];
extern int cur_map_cols;
extern int cur_map_rows;
extern int current_floor;
extern Room rooms[MAX_ROOMS];
extern int room_count;

void dungeon_init_lobby(Player *p);
void dungeon_init_tutorial(Player *p);
void dungeon_generate_floor(int floor, Player *p);
void dungeon_update(float dt, Player *p);
bool dungeon_is_solid(int tile_x, int tile_y);
bool dungeon_is_hazard(int tile_x, int tile_y);
bool dungeon_check_collision(float x, float y, float radius);
void dungeon_render_tiles(float cam_x, float cam_y, float anim_timer);
