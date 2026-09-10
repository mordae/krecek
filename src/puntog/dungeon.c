#include "dungeon.h"
#include <tiles-16x16.png.h>
#include <stdlib.h>
#include <string.h>

uint8_t cur_map[DUNGEON_ROWS][DUNGEON_COLS];
int cur_map_cols = DUNGEON_COLS;
int cur_map_rows = DUNGEON_ROWS;
int current_floor = 0; // 0 = Lobby

Room rooms[MAX_ROOMS];
int room_count = 0;

static inline bool in_bounds(int tx, int ty)
{
	return tx >= 0 && tx < cur_map_cols && ty >= 0 && ty < cur_map_rows;
}

bool dungeon_is_solid(int tx, int ty)
{
	if (!in_bounds(tx, ty))
		return true;

	uint8_t t = cur_map[ty][tx];
	if (t == EMPTY)
		return true;

	if (t >= TILE_LEFT_WALL && t <= TILE_CORNER_SE)
		return true;
	if (t == TILE_TORCH_0 || t == TILE_TORCH_1)
		return true;
	if (t == TILE_BANNER)
		return true;
	if (t == TILE_CHEST_CLOSED)
		return true;
	if (t == TILE_WARDROBE)
		return true;
	if (t == TILE_SHOP_ALTAR)
		return true;

	return false;
}

bool dungeon_is_hazard(int tx, int ty)
{
	if (!in_bounds(tx, ty))
		return false;

	uint8_t t = cur_map[ty][tx];
	return (t == TILE_LAVA_0 || t == TILE_LAVA_1);
}

bool dungeon_check_collision(float x, float y, float radius)
{
	// Test the 4 bounding box points around the circle
	int left   = (int)((x - radius) / TILE_SIZE);
	int right  = (int)((x + radius) / TILE_SIZE);
	int top    = (int)((y - radius) / TILE_SIZE);
	int bottom = (int)((y + radius) / TILE_SIZE);

	for (int ty = top; ty <= bottom; ty++) {
		for (int tx = left; tx <= right; tx++) {
			if (dungeon_is_solid(tx, ty)) {
				return true;
			}
		}
	}
	return false;
}

void dungeon_init_lobby(Player *p)
{
	current_floor = 0; // 0 = Lobby Sanctuary
	cur_map_cols = LOBBY_COLS;
	cur_map_rows = LOBBY_ROWS;
	clear_all_entities();
	room_count = 0;

	memset(cur_map, EMPTY, sizeof(cur_map));

	// Carve lobby floor (14x10 with 1 tile border walls)
	for (int y = 1; y < cur_map_rows - 1; y++) {
		for (int x = 1; x < cur_map_cols - 1; x++) {
			cur_map[y][x] = TILE_FLOOR;
		}
	}

	// North & South walls
	for (int x = 1; x < cur_map_cols - 1; x++) {
		cur_map[0][x] = TILE_NORTH_WALL;
		cur_map[cur_map_rows - 1][x] = TILE_SOUTH_WALL;
	}
	// Left & Right walls
	for (int y = 1; y < cur_map_rows - 1; y++) {
		cur_map[y][0] = TILE_LEFT_WALL;
		cur_map[y][cur_map_cols - 1] = TILE_RIGHT_WALL;
	}
	// Corners are left EMPTY (no protruding corners!)
	cur_map[0][0] = EMPTY;
	cur_map[0][cur_map_cols - 1] = EMPTY;
	cur_map[cur_map_rows - 1][0] = EMPTY;
	cur_map[cur_map_rows - 1][cur_map_cols - 1] = EMPTY;

	// Props: Torches on north wall
	cur_map[0][3] = TILE_TORCH_0;
	cur_map[0][cur_map_cols - 4] = TILE_TORCH_0;
	cur_map[0][cur_map_cols / 2] = TILE_BANNER;

	// Props in Lobby:
	// Wardrobe for character customizer
	cur_map[2][2] = TILE_WARDROBE;
	// Shop altar for upgrades
	cur_map[2][cur_map_cols - 3] = TILE_SHOP_ALTAR;
	// Dungeon Entrance Portal in center
	cur_map[2][cur_map_cols / 2] = TILE_PORTAL;
	// Training Grounds (Tutorial ladder)
	cur_map[cur_map_rows - 3][2] = TILE_LADDER;
	// Archives & Bestiary chest
	cur_map[cur_map_rows - 3][cur_map_cols - 3] = TILE_CHEST_CLOSED;

	// Place player in center lobby
	p->x = (cur_map_cols / 2) * TILE_SIZE + 4;
	p->y = 6 * TILE_SIZE;
	p->vx = 0;
	p->vy = 0;

	// Spawn an indestructible Training Dummy to test spells
	Zombie *dummy = zombie_alloc();
	if (dummy) {
		dummy->x = (cur_map_cols / 2 + 2) * TILE_SIZE;
		dummy->y = 5 * TILE_SIZE;
		dummy->type = 3; // Training Dummy
		dummy->speed = 0.0f;
		dummy->max_hp = 9999;
		dummy->hp = dummy->max_hp;
	}
}

void dungeon_init_tutorial(Player *p)
{
	current_floor = -1; // -1 = Tutorial Training Grounds
	cur_map_cols = LOBBY_COLS;
	cur_map_rows = LOBBY_ROWS;
	clear_all_entities();
	room_count = 0;

	memset(cur_map, EMPTY, sizeof(cur_map));

	for (int y = 1; y < cur_map_rows - 1; y++) {
		for (int x = 1; x < cur_map_cols - 1; x++) {
			cur_map[y][x] = TILE_FLOOR;
		}
	}

	for (int x = 1; x < cur_map_cols - 1; x++) {
		cur_map[0][x] = TILE_NORTH_WALL;
		cur_map[cur_map_rows - 1][x] = TILE_SOUTH_WALL;
	}
	for (int y = 1; y < cur_map_rows - 1; y++) {
		cur_map[y][0] = TILE_LEFT_WALL;
		cur_map[y][cur_map_cols - 1] = TILE_RIGHT_WALL;
	}
	cur_map[0][0] = EMPTY;
	cur_map[0][cur_map_cols - 1] = EMPTY;
	cur_map[cur_map_rows - 1][0] = EMPTY;
	cur_map[cur_map_rows - 1][cur_map_cols - 1] = EMPTY;

	cur_map[0][3] = TILE_TORCH_0;
	cur_map[0][cur_map_cols - 4] = TILE_TORCH_0;
	cur_map[0][cur_map_cols / 2] = TILE_BANNER;

	// Portal to return to lobby
	cur_map[5][cur_map_cols - 3] = TILE_PORTAL;

	// Place player on the left
	p->x = 3 * TILE_SIZE;
	p->y = 5 * TILE_SIZE;
	p->vx = 0;
	p->vy = 0;

	// Training Dummy in center
	Zombie *dummy = zombie_alloc();
	if (dummy) {
		dummy->x = 7 * TILE_SIZE;
		dummy->y = 5 * TILE_SIZE;
		dummy->type = 3; // Training Dummy
		dummy->speed = 0.0f;
		dummy->max_hp = 9999;
		dummy->hp = dummy->max_hp;
	}
}

static bool rooms_overlap(Room *a, Room *b)
{
	return !(a->x + a->w + 2 <= b->x ||
		 b->x + b->w + 2 <= a->x ||
		 a->y + a->h + 2 <= b->y ||
		 b->y + b->h + 2 <= a->y);
}

static inline bool is_passable(uint8_t t)
{
	return (t >= TILE_FLOOR && t <= TILE_FLOOR_4) ||
	       (t == TILE_CORRIDOR) ||
	       (t == TILE_LADDER) ||
	       (t == TILE_PORTAL);
}

void dungeon_generate_floor(int floor, Player *p)
{
	current_floor = floor;
	cur_map_cols = DUNGEON_COLS;
	cur_map_rows = DUNGEON_ROWS;
	clear_all_entities();

	memset(cur_map, EMPTY, sizeof(cur_map));

	// Procedurally generate 4 to 6 interconnected rooms
	int target_rooms = 4 + (rand() % 3); // 4, 5, or 6
	room_count = 0;

	for (int attempt = 0; attempt < 120 && room_count < target_rooms; attempt++) {
		Room r;
		r.w = 6 + (rand() % 4); // 6..9
		r.h = 5 + (rand() % 3); // 5..7
		r.x = 2 + (rand() % (cur_map_cols - r.w - 4));
		r.y = 2 + (rand() % (cur_map_rows - r.h - 4));
		r.cx = r.x + r.w / 2;
		r.cy = r.y + r.h / 2;
		r.has_spawner = false;
		r.spawns_left = 0;
		r.spawn_timer = 0;

		bool ok = true;
		for (int j = 0; j < room_count; j++) {
			if (rooms_overlap(&r, &rooms[j])) {
				ok = false;
				break;
			}
		}

		if (ok) {
			rooms[room_count++] = r;
		}
	}

	if (room_count < 3) {
		// Fallback minimal rooms
		room_count = 3;
		rooms[0] = (Room){ .x = 2, .y = 2, .w = 8, .h = 7, .cx = 6, .cy = 5 };
		rooms[1] = (Room){ .x = 13, .y = 3, .w = 8, .h = 6, .cx = 17, .cy = 6 };
		rooms[2] = (Room){ .x = 19, .y = 13, .w = 8, .h = 7, .cx = 23, .cy = 16 };
	}

	// 1. Carve rooms: outer perimeter gets directional walls, interior gets floor variants
	for (int i = 0; i < room_count; i++) {
		Room *r = &rooms[i];
		for (int y = r->y; y < r->y + r->h; y++) {
			for (int x = r->x; x < r->x + r->w; x++) {
				bool is_corner = (x == r->x || x == r->x + r->w - 1) &&
				                 (y == r->y || y == r->y + r->h - 1);
				if (is_corner) {
					cur_map[y][x] = EMPTY;
				} else if (y == r->y) {
					cur_map[y][x] = TILE_NORTH_WALL;
				} else if (y == r->y + r->h - 1) {
					cur_map[y][x] = TILE_SOUTH_WALL;
				} else if (x == r->x) {
					cur_map[y][x] = TILE_LEFT_WALL;
				} else if (x == r->x + r->w - 1) {
					cur_map[y][x] = TILE_RIGHT_WALL;
				} else {
					int variant = rand() % 8;
					if (variant == 0)
						cur_map[y][x] = TILE_FLOOR_1;
					else if (variant == 1)
						cur_map[y][x] = TILE_FLOOR_2;
					else if (variant == 2)
						cur_map[y][x] = TILE_FLOOR_3;
					else
						cur_map[y][x] = TILE_FLOOR;
				}
			}
		}
	}

	// 2. Connect room i to room i+1 with 2-wide corridors
	for (int i = 0; i < room_count - 1; i++) {
		int x1 = rooms[i].cx;
		int y1 = rooms[i].cy;
		int x2 = rooms[i + 1].cx;
		int y2 = rooms[i + 1].cy;

		// Horizontal corridor
		int x_start = (x1 < x2) ? x1 : x2;
		int x_end = (x1 < x2) ? x2 : x1;
		for (int x = x_start; x <= x_end; x++) {
			cur_map[y1][x] = TILE_CORRIDOR;
			if (y1 + 1 < cur_map_rows - 1)
				cur_map[y1 + 1][x] = TILE_CORRIDOR;
		}

		// Vertical corridor
		int y_start = (y1 < y2) ? y1 : y2;
		int y_end = (y1 < y2) ? y2 : y1;
		for (int y = y_start; y <= y_end; y++) {
			cur_map[y][x2] = TILE_CORRIDOR;
			if (x2 + 1 < cur_map_cols - 1)
				cur_map[y][x2 + 1] = TILE_CORRIDOR;
		}
	}

	// 3. Surround corridors through EMPTY space with walls using read-only snapshot
	uint8_t temp_map[DUNGEON_ROWS][DUNGEON_COLS];
	memcpy(temp_map, cur_map, sizeof(temp_map));

	for (int y = 0; y < cur_map_rows; y++) {
		for (int x = 0; x < cur_map_cols; x++) {
			if (cur_map[y][x] != EMPTY)
				continue;

			bool has_s = (y < cur_map_rows - 1 && is_passable(temp_map[y + 1][x]));
			bool has_n = (y > 0 && is_passable(temp_map[y - 1][x]));
			bool has_e = (x < cur_map_cols - 1 && is_passable(temp_map[y][x + 1]));
			bool has_w = (x > 0 && is_passable(temp_map[y][x - 1]));

			if (has_s) {
				cur_map[y][x] = TILE_NORTH_WALL;
			} else if (has_n) {
				cur_map[y][x] = TILE_SOUTH_WALL;
			} else if (has_e) {
				cur_map[y][x] = TILE_LEFT_WALL;
			} else if (has_w) {
				cur_map[y][x] = TILE_RIGHT_WALL;
			}
		}
	}

	// 4. Place props, torches, banners, spawners, chest, and exit ladder
	for (int i = 0; i < room_count; i++) {
		Room *r = &rooms[i];

		// Place wall torches on north wall of rooms (if intact north wall)
		if (cur_map[r->y][r->x + 2] == TILE_NORTH_WALL) {
			cur_map[r->y][r->x + 2] = TILE_TORCH_0;
		}
		if (r->w >= 6 && cur_map[r->y][r->x + r->w - 3] == TILE_NORTH_WALL) {
			cur_map[r->y][r->x + r->w - 3] = TILE_TORCH_0;
		}

		if (i == 0) {
			// Room 0 is Spawn room
			p->x = (r->cx) * TILE_SIZE;
			p->y = (r->cy) * TILE_SIZE;
		} else if (i == room_count - 1) {
			// Final Room has Exit Ladder
			cur_map[r->cy][r->cx] = TILE_LADDER;

			// Elite/Boss Spawner next to ladder
			r->has_spawner = true;
			r->spawner_x = r->cx - 1;
			r->spawner_y = r->cy;
			cur_map[r->spawner_y][r->spawner_x] = TILE_SPAWNER;
			r->spawns_left = 6 + floor * 2;
			r->spawn_timer = 1.0f;
		} else {
			// Middle Rooms have Enemy Spawners
			r->has_spawner = true;
			r->spawner_x = r->cx;
			r->spawner_y = r->cy;
			cur_map[r->spawner_y][r->spawner_x] = TILE_SPAWNER;
			r->spawns_left = 5 + floor * 2;
			r->spawn_timer = 1.0f;

			// Occasional chest in room 1
			if (i == 1) {
				cur_map[r->y + 1][r->x + 1] = TILE_CHEST_CLOSED;
			}
			// Occasional hazard lava pool in room 2 or 3
			if (i == 2 && r->w >= 6 && r->h >= 5) {
				cur_map[r->y + 1][r->x + r->w - 2] = TILE_LAVA_0;
				cur_map[r->y + 2][r->x + r->w - 2] = TILE_LAVA_0;
			}
		}
	}

	// 5. Pre-spawn active enemies immediately across all rooms!
	for (int i = 0; i < room_count; i++) {
		Room *r = &rooms[i];
		int count = (i == 0) ? 2 : 3;
		for (int k = 0; k < count; k++) {
			Zombie *z = zombie_alloc();
			if (!z) break;
			float ox = (k == 0) ? 14.0f : -14.0f;
			float oy = (k == 0) ? -10.0f : 10.0f;
			if (i == 0) {
				ox = 18.0f;
				oy = (k == 0) ? -10.0f : 10.0f;
			}
			float zx = r->cx * TILE_SIZE + ox;
			float zy = r->cy * TILE_SIZE + oy;
			if (dungeon_check_collision(zx, zy, 4.0f)) {
				zx = r->cx * TILE_SIZE;
				zy = r->cy * TILE_SIZE;
			}
			z->x = zx;
			z->y = zy;
			z->type = (rand() % 10 < 3 && floor >= 2) ? 1 : 0;
			z->speed = (z->type == 1) ? 44.0f : 32.0f;
			z->max_hp = (z->type == 1) ? 35 : 45 + floor * 5;
			z->hp = z->max_hp;
		}
	}
}

void dungeon_update(float dt, Player *p)
{
	(void)p;

	// Update room spawners
	for (int i = 0; i < room_count; i++) {
		Room *r = &rooms[i];
		if (!r->has_spawner || r->spawns_left <= 0)
			continue;

		r->spawn_timer -= dt;
		if (r->spawn_timer <= 0) {
			r->spawn_timer = 2.5f + ((float)rand() / (float)RAND_MAX) * 1.5f;

			// Try allocating a zombie from pool (respects hard cap MAX_ACTIVE_ZOMBIES)
			Zombie *z = zombie_alloc();
			if (z) {
				// Offset spawn around spawner
				float ox = ((rand() % 3) - 1) * 12.0f;
				float oy = ((rand() % 3) - 1) * 12.0f;
				z->x = r->spawner_x * TILE_SIZE + 4 + ox;
				z->y = r->spawner_y * TILE_SIZE + 4 + oy;

				// Zombie types: Walker, Runner, Brute
				int roll = rand() % 10;
				if (current_floor >= 2 && roll < 3) {
					z->type = 1; // Fast Ghoul/Runner
					z->speed = 46.0f + current_floor * 2.0f;
					z->max_hp = 35 + current_floor * 5;
				} else if (current_floor >= 3 && roll < 5) {
					z->type = 2; // Armored Brute
					z->speed = 26.0f;
					z->max_hp = 90 + current_floor * 15;
				} else {
					z->type = 0; // Standard Walker
					z->speed = 34.0f + current_floor * 1.5f;
					z->max_hp = 50 + current_floor * 8;
				}
				z->hp = z->max_hp;

				r->spawns_left--;
				// Spawn purple summoning particle burst
				spawn_particles(z->x + 4, z->y + 6, 6, PURPLE, 30.0f);
			}
		}
	}
}

void dungeon_render_tiles(float cam_x, float cam_y, float anim_timer)
{
	int torch_anim = ((int)(anim_timer * 4.0f)) & 1;
	int lava_anim = ((int)(anim_timer * 2.5f)) & 1;

	// Calculate visible tile range
	int min_tx = (int)(cam_x / TILE_SIZE) - 1;
	int max_tx = (int)((cam_x + TFT_WIDTH) / TILE_SIZE) + 1;
	int min_ty = (int)(cam_y / TILE_SIZE) - 1;
	int max_ty = (int)((cam_y + TFT_HEIGHT) / TILE_SIZE) + 1;

	if (min_tx < 0) min_tx = 0;
	if (max_tx >= cur_map_cols) max_tx = cur_map_cols - 1;
	if (min_ty < 0) min_ty = 0;
	if (max_ty >= cur_map_rows) max_ty = cur_map_rows - 1;

	for (int ty = min_ty; ty <= max_ty; ty++) {
		for (int tx = min_tx; tx <= max_tx; tx++) {
			uint8_t t = cur_map[ty][tx];
			if (t == EMPTY)
				continue;

			// Check animated tiles
			int tile_idx = t - 1;
			if (t == TILE_TORCH_0 && torch_anim) {
				tile_idx = TILE_TORCH_1 - 1;
			} else if (t == TILE_LAVA_0 && lava_anim) {
				tile_idx = TILE_LAVA_1 - 1;
			}

			sdk_draw_tile(tx * TILE_SIZE, ty * TILE_SIZE, &ts_tiles_16x16_png, tile_idx);
		}
	}
}
