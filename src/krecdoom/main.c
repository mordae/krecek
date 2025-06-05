#include "common.h"
#include "volume.h"
#include <pico/stdlib.h>
#include <sdk.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <tft.h>

float playerX = TILE_SIZE * 1.5f;
float playerY = TILE_SIZE * 1.5f;
float volume = 0.5f;
float playerAngle = (float)M_PI / 2.0f;
bool debug_mode = false;
bool menu_active = false;

bool isWall(int Tile_x, int Tile_y);
void drawMenuMap(void);

extern const TileType maps_map1[MAP_ROWS][MAP_COLS];
TileType (*currentMap)[MAP_COLS] = maps_map1;

void renderGame()
{
	// Raycaster Engine logic

	float rayAngle;

	// Loop through each vertical column (pixel) on the screen to cast a ray
	for (int x = 0; x < SCREEN_WIDTH; x++) {
		// Calculate the current ray angle based on player's angle and FOV
		rayAngle = (playerAngle - FOV_RADIANS / 2.0f) + (x * (FOV_RADIANS / SCREEN_WIDTH));

		// Determine the current map tile the player is in
		int mapX = (int)(playerX / TILE_SIZE);
		int mapY = (int)(playerY / TILE_SIZE);

		// Calculate ray direction components
		float rayDirX = cosf(rayAngle);
		float rayDirY = sinf(rayAngle);

		// Calculate deltaDist: distance the ray needs to travel to cross one full X or Y tile
		float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX) * TILE_SIZE;
		float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY) * TILE_SIZE;

		// Variables for DDA (Digital Differential Analysis) algorithm
		int stepX;
		int stepY;
		float sideDistX; // Distance from current position to next X-side
		float sideDistY; // Distance from current position to next Y-side

		int hit = 0; // Flag to indicate if a wall was hit
		int side;    // 0 for X-side hit, 1 for Y-side hit

		// Calculate step and initial sideDistX
		if (rayDirX < 0.0f) {
			stepX = -1;
			sideDistX = (playerX - mapX * TILE_SIZE) * deltaDistX / TILE_SIZE;
		} else {
			stepX = 1;
			sideDistX = ((mapX + 1) * TILE_SIZE - playerX) * deltaDistX / TILE_SIZE;
		}

		// Calculate step and initial sideDistY
		if (rayDirY < 0.0f) {
			stepY = -1;
			sideDistY = (playerY - mapY * TILE_SIZE) * deltaDistY / TILE_SIZE;
		} else {
			stepY = 1;
			sideDistY = ((mapY + 1) * TILE_SIZE - playerY) * deltaDistY / TILE_SIZE;
		}

		// DDA loop: traverse the grid
		while (hit == 0) {
			// Advance ray to next grid line
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0; // Hit a vertical (X) wall
			} else {
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1; // Hit a horizontal (Y) wall
			}

			// Check if the ray has hit a wall or gone out of bounds
			if (isWall(mapX, mapY)) {
				hit = 1; // Wall found
			}
		}

		// Calculate perpendicular distance to the wall
		// This corrects the "fisheye" effect
		float perpWallDist;
		if (side == 0) {
			perpWallDist =
				(mapX * TILE_SIZE - playerX + (1.0f - stepX) * TILE_SIZE / 2.0f) /
				rayDirX;
		} else {
			perpWallDist =
				(mapY * TILE_SIZE - playerY + (1.0f - stepY) * TILE_SIZE / 2.0f) /
				rayDirY;
		}

		// Prevent division by zero or very small numbers
		if (perpWallDist == 0.0f)
			perpWallDist = 0.001f;

		// Calculate the height of the wall slice to be drawn
		float wallHeight = (TILE_SIZE * PROJECTION_PLANE_DISTANCE / perpWallDist);

		// Calculate start and end pixel for drawing the wall slice
		int drawStart = (int)((SCREEN_HEIGHT / 2.0f) - (wallHeight / 2.0f));
		int drawEnd = (int)((SCREEN_HEIGHT / 2.0f) + (wallHeight / 2.0f));

		// Clamp drawing coordinates to screen boundaries
		if (drawStart < 0)
			drawStart = 0;
		if (drawEnd >= SCREEN_HEIGHT)
			drawEnd = SCREEN_HEIGHT - 1;

		// Determine wall color based on which side was hit (for shading effect)
		uint16_t wallColor;
		if (side == 0) { // X-side (vertical wall)
			wallColor = COLOR_WALL_BRIGHT;
		} else { // Y-side (horizontal wall)
			wallColor = COLOR_WALL_DARK;
		}

		// Draw the wall slice
		for (int y = drawStart; y <= drawEnd; y++) {
			tft_draw_pixel(x, y, wallColor);
		}

		// Draw the ceiling above the wall slice
		for (int y = 0; y < drawStart; y++) {
			tft_draw_pixel(x, y, COLOR_CEILING);
		}
		// Draw the floor below the wall slice
		for (int y = drawEnd + 1; y < SCREEN_HEIGHT; y++) {
			tft_draw_pixel(x, y, COLOR_FLOOR);
		}
	}
}

void drawMinimap_Debug()
{
	// Calculate the top-left position of the minimap for the bottom-right corner.
	int minimap_start_x =
		SCREEN_WIDTH - MINIMAP_WIDTH - MINIMAP_PADDING; // Positioned to the right
	int minimap_start_y =
		SCREEN_HEIGHT - MINIMAP_HEIGHT - MINIMAP_PADDING; // Positioned to the bottom

	// Determine the center tile of the minimap view based on player's position
	int playerMapX = (int)(playerX / TILE_SIZE);
	int playerMapY = (int)(playerY / TILE_SIZE);

	// Calculate the top-left tile index for the minimap window
	// This ensures the minimap is centered around the player, but clamped to map boundaries.
	int map_window_start_x = playerMapX - (MINIMAP_VIEW_TILES / 2);
	int map_window_start_y = playerMapY - (MINIMAP_VIEW_TILES / 2);

	// Clamp the window start to ensure it doesn't go out of bounds on the map (left/top)
	if (map_window_start_x < 0)
		map_window_start_x = 0;
	if (map_window_start_y < 0)
		map_window_start_y = 0;

	// Clamp the window start to ensure it doesn't go out of bounds on the map (right/bottom)
	// This ensures the full MINIMAP_VIEW_TILES x MINIMAP_VIEW_TILES window is always visible
	if (map_window_start_x + MINIMAP_VIEW_TILES > MAP_COLS) {
		map_window_start_x = MAP_COLS - MINIMAP_VIEW_TILES;
		if (map_window_start_x < 0)
			map_window_start_x = 0; // Fallback for very small maps
	}
	if (map_window_start_y + MINIMAP_VIEW_TILES > MAP_ROWS) {
		map_window_start_y = MAP_ROWS - MINIMAP_VIEW_TILES;
		if (map_window_start_y < 0)
			map_window_start_y = 0; // Fallback for very small maps
	}

	// Now, drawing the map tiles for the visible minimap window.
	// Loop over the minimap's pixel grid (MINIMAP_VIEW_TILES x MINIMAP_VIEW_TILES)
	for (int y_minimap_idx = 0; y_minimap_idx < MINIMAP_VIEW_TILES; y_minimap_idx++) {
		for (int x_minimap_idx = 0; x_minimap_idx < MINIMAP_VIEW_TILES; x_minimap_idx++) {
			// Calculate the actual map tile coordinates from the map_window_start
			int map_tile_x = map_window_start_x + x_minimap_idx;
			int map_tile_y = map_window_start_y + y_minimap_idx;

			uint16_t tileColor;
			// Check if the current map tile is a wall or empty.
			// The isWall function handles boundary checks for the world map.
			if (isWall(map_tile_x, map_tile_y)) {
				tileColor = COLOR_MINIMAP_WALL; // White for walls
			} else {
				tileColor = COLOR_MINIMAP_EMPTY; // Black for empty spaces
			}

			// Draw each pixel of the minimap tile
			for (int py = 0; py < MINIMAP_TILE_SIZE; py++) {
				for (int px = 0; px < MINIMAP_TILE_SIZE; px++) {
					tft_draw_pixel(
						minimap_start_x +
							x_minimap_idx * MINIMAP_TILE_SIZE + px,
						minimap_start_y +
							y_minimap_idx * MINIMAP_TILE_SIZE + py,
						tileColor);
				}
			}
		}
	}

	// Now, drawing the player on the minimap.
	// Convert player's world coordinates to pixels relative to the minimap's current window.
	// Then add offset to center the player dot within its tile.
	float player_relative_x = playerX / TILE_SIZE - map_window_start_x;
	float player_relative_y = playerY / TILE_SIZE - map_window_start_y;

	int minimap_player_x = minimap_start_x + (int)(player_relative_x * MINIMAP_TILE_SIZE -
						       (PLAYER_DOT_SIZE / 2.0f));
	int minimap_player_y = minimap_start_y + (int)(player_relative_y * MINIMAP_TILE_SIZE -
						       (PLAYER_DOT_SIZE / 2.0f));

	// Draw a yellow block for the player.
	// Ensure player is drawn within the minimap bounds to prevent drawing outside
	if (minimap_player_x >= minimap_start_x &&
	    minimap_player_x + PLAYER_DOT_SIZE <= minimap_start_x + MINIMAP_WIDTH &&
	    minimap_player_y >= minimap_start_y &&
	    minimap_player_y + PLAYER_DOT_SIZE <= minimap_start_y + MINIMAP_HEIGHT) {
		for (int py = 0; py < PLAYER_DOT_SIZE; py++) {
			for (int px = 0; px < PLAYER_DOT_SIZE; px++) {
				tft_draw_pixel(minimap_player_x + px, minimap_player_y + py,
					       COLOR_MINIMAP_PLAYER);
			}
		}
	}

	// Now, drawing the player's looking direction lines (FOV borders).
	// Calculate the center of the player's dot on the minimap for FOV lines.
	float player_center_x_on_minimap = minimap_player_x + (PLAYER_DOT_SIZE / 2.0f);
	float player_center_y_on_minimap = minimap_player_y + (PLAYER_DOT_SIZE / 2.0f);

	// Define a small step size for tracing the FOV rays
	float step_size = 1.0f; // Step by 1 pixel at a time on the minimap

	// Trace Leftmost FOV ray
	float current_ray_x = player_center_x_on_minimap;
	float current_ray_y = player_center_y_on_minimap;
	float left_ray_angle = playerAngle - (FOV_RADIANS / 2.0f);
	float ray_dir_x = cosf(left_ray_angle);
	float ray_dir_y = sinf(left_ray_angle);

	// Trace the left FOV ray until it hits a wall or goes out of bounds.
	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		// Check if the current pixel is within the minimap boundaries on screen.
		if (map_pixel_x < minimap_start_x ||
		    map_pixel_x >= minimap_start_x + MINIMAP_WIDTH ||
		    map_pixel_y < minimap_start_y ||
		    map_pixel_y >= minimap_start_y + MINIMAP_HEIGHT) {
			break; // Gone out of minimap bounds
		}

		// Convert minimap pixel to world map tile coordinates relative to the full map
		// Need to adjust back to the original map coordinate system
		int tile_x_world =
			map_window_start_x + (map_pixel_x - minimap_start_x) / MINIMAP_TILE_SIZE;
		int tile_y_world =
			map_window_start_y + (map_pixel_y - minimap_start_y) / MINIMAP_TILE_SIZE;

		// Check if hit a wall in the actual world map
		if (isWall(tile_x_world, tile_y_world)) {
			break; // Hit a wall, stop drawing
		}
		tft_draw_pixel(map_pixel_x, map_pixel_y,
			       COLOR_MINIMAP_LINE); // Draw a green pixel for the line.

		current_ray_x += ray_dir_x * step_size; // Move the ray forward
		current_ray_y += ray_dir_y * step_size;
	}

	// Trace Rightmost FOV ray (similar logic)
	current_ray_x = player_center_x_on_minimap;
	current_ray_y = player_center_y_on_minimap;
	float right_ray_angle = playerAngle + (FOV_RADIANS / 2.0f);
	ray_dir_x = cosf(right_ray_angle);
	ray_dir_y = sinf(right_ray_angle);

	while (true) {
		int map_pixel_x = (int)current_ray_x;
		int map_pixel_y = (int)current_ray_y;

		if (map_pixel_x < minimap_start_x ||
		    map_pixel_x >= minimap_start_x + MINIMAP_WIDTH ||
		    map_pixel_y < minimap_start_y ||
		    map_pixel_y >= minimap_start_y + MINIMAP_HEIGHT) {
			break;
		}

		int tile_x_world =
			map_window_start_x + (map_pixel_x - minimap_start_x) / MINIMAP_TILE_SIZE;
		int tile_y_world =
			map_window_start_y + (map_pixel_y - minimap_start_y) / MINIMAP_TILE_SIZE;

		if (isWall(tile_x_world, tile_y_world)) {
			break;
		}

		tft_draw_pixel(map_pixel_x, map_pixel_y, COLOR_MINIMAP_LINE);

		current_ray_x += ray_dir_x * step_size;
		current_ray_y += ray_dir_y * step_size;
	}
}

// New function to draw the full map for the menu screen
void drawMenuMap(void)
{
	// Define the dimensions and padding for the menu map
	const int MENU_MAP_WIDTH = 80;	 // Updated: Menu map width to 80 pixels
	const int MENU_MAP_HEIGHT = 80;	 // Updated: Menu map height to 80 pixels
	const int MENU_MAP_PADDING = 20; // This padding is implicitly handled by centering.

	// Calculate the top-left position of the menu map to center it
	int menu_map_start_x = (SCREEN_WIDTH - MENU_MAP_WIDTH) / 2;
	int menu_map_start_y = (SCREEN_HEIGHT - MENU_MAP_HEIGHT) / 2;

	// Calculate the pixel size of each tile on the menu map
	// This will scale the MAP_COLS x MAP_ROWS map to fit within MENU_MAP_WIDTH x MENU_MAP_HEIGHT pixels
	float tile_pixel_width = (float)MENU_MAP_WIDTH / MAP_COLS;
	float tile_pixel_height = (float)MENU_MAP_HEIGHT / MAP_ROWS;

	// Draw the map tiles
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			uint16_t tileColor;
			if (currentMap[y][x] != EMPTY) {
				tileColor = WHITE; // Walls are white
			} else {
				tileColor = BLACK; // Empty spaces are black
			}

			// Draw the rectangle for each tile
			for (int py = 0; py < tile_pixel_height; py++) {
				for (int px = 0; px < tile_pixel_width; px++) {
					tft_draw_pixel(menu_map_start_x +
							       (int)(x * tile_pixel_width) + px,
						       menu_map_start_y +
							       (int)(y * tile_pixel_height) + py,
						       tileColor);
				}
			}
		}
	}

	// Draw the player on the menu map
	// Convert player's world coordinates to menu map pixel coordinates,
	// then add offset to center the player dot within its tile.
	int menu_player_x = menu_map_start_x + (int)((playerX / TILE_SIZE) * tile_pixel_width -
						     (PLAYER_DOT_SIZE / 2.0f));
	int menu_player_y = menu_map_start_y + (int)((playerY / TILE_SIZE) * tile_pixel_height -
						     (PLAYER_DOT_SIZE / 2.0f));

	// Draw a yellow block for the player.
	// Ensure player is drawn within the menu map bounds
	if (menu_player_x >= menu_map_start_x &&
	    menu_player_x + PLAYER_DOT_SIZE <= menu_map_start_x + MENU_MAP_WIDTH &&
	    menu_player_y >= menu_map_start_y &&
	    menu_player_y + PLAYER_DOT_SIZE <= menu_map_start_y + MENU_MAP_HEIGHT) {
		for (int py = 0; py < PLAYER_DOT_SIZE; py++) {
			for (int px = 0; px < PLAYER_DOT_SIZE; px++) {
				tft_draw_pixel(menu_player_x + px, menu_player_y + py, YELLOW);
			}
		}
	}
}

// Corrected isWall function logic
bool isWall(int Tile_x, int Tile_y)
{
	// Check if the tile coordinates are out of bounds
	if (Tile_x < 0 || Tile_x >= MAP_COLS || Tile_y < 0 || Tile_y >= MAP_ROWS) {
		return true; // Treat out-of-bounds as a wall
	}
	// Return true if the tile type is WALL, false otherwise (e.g., EMPTY)
	return currentMap[Tile_y][Tile_x] == WALL;
}

// New function to handle player movement and collision
void handlePlayerMovement(float dt)
{
	// Store current player position as the previous valid position
	float prevPlayerX = playerX;
	float prevPlayerY = playerY;

	// Calculate potential movement deltas
	float deltaMoveX = 0;
	float deltaMoveY = 0;

	if (sdk_inputs.joy_y > 500) { // Move forward
		deltaMoveX = -MOVE_SPEED * dt * cosf(playerAngle);
		deltaMoveY = -MOVE_SPEED * dt * sinf(playerAngle);
	} else if (sdk_inputs.joy_y < -500) { // Move backward
		deltaMoveX = +MOVE_SPEED * dt * cosf(playerAngle);
		deltaMoveY = +MOVE_SPEED * dt * sinf(playerAngle);
	}

	float player_half_size = TILE_SIZE * 0.4f; // Player collision box half size
	float penetration_limit =
		TILE_SIZE * COLLISION_PENETRATION_FRACTION; // How far player can enter a wall

	// --- X-axis movement and collision ---
	float newPlayerX_candidate = playerX + deltaMoveX;
	bool collisionX = false;

	// Check all four corners of the player's bounding box at the *new X*, but *current Y*
	if (isWall((int)((newPlayerX_candidate - player_half_size) / TILE_SIZE),
		   (int)((playerY - player_half_size) / TILE_SIZE)) ||
	    isWall((int)((newPlayerX_candidate + player_half_size) / TILE_SIZE),
		   (int)((playerY - player_half_size) / TILE_SIZE)) ||
	    isWall((int)((newPlayerX_candidate - player_half_size) / TILE_SIZE),
		   (int)((playerY + player_half_size) / TILE_SIZE)) ||
	    isWall((int)((newPlayerX_candidate + player_half_size) / TILE_SIZE),
		   (int)((playerY + player_half_size) / TILE_SIZE))) {
		collisionX = true;
	}

	if (collisionX) {
		// Find which tile the player is currently occupying (before full movement)
		int currentTileX = (int)(playerX / TILE_SIZE);
		if (deltaMoveX > 0) { // Moving right into a wall
			newPlayerX_candidate = (float)currentTileX * TILE_SIZE + TILE_SIZE -
					       player_half_size - penetration_limit;
		} else if (deltaMoveX < 0) { // Moving left into a wall
			newPlayerX_candidate = (float)(currentTileX + 1) * TILE_SIZE +
					       player_half_size + penetration_limit;
		}
	}
	playerX = newPlayerX_candidate; // Apply the (potentially adjusted) X position

	// --- Y-axis movement and collision ---
	float newPlayerY_candidate = playerY + deltaMoveY;
	bool collisionY = false;

	// Check all four corners of the player's bounding box at the *new Y*, but *new X* (already adjusted)
	if (isWall((int)((playerX - player_half_size) / TILE_SIZE),
		   (int)((newPlayerY_candidate - player_half_size) / TILE_SIZE)) ||
	    isWall((int)((playerX + player_half_size) / TILE_SIZE),
		   (int)((newPlayerY_candidate - player_half_size) / TILE_SIZE)) ||
	    isWall((int)((playerX - player_half_size) / TILE_SIZE),
		   (int)((newPlayerY_candidate + player_half_size) / TILE_SIZE)) ||
	    isWall((int)((playerX + player_half_size) / TILE_SIZE),
		   (int)((newPlayerY_candidate + player_half_size) / TILE_SIZE))) {
		collisionY = true;
	}

	if (collisionY) {
		// Find which tile the player is currently occupying (before full movement)
		int currentTileY = (int)(playerY / TILE_SIZE);
		if (deltaMoveY > 0) { // Moving down into a wall
			newPlayerY_candidate = (float)currentTileY * TILE_SIZE + TILE_SIZE -
					       player_half_size - penetration_limit;
		} else if (deltaMoveY < 0) { // Moving up into a wall
			newPlayerY_candidate = (float)(currentTileY + 1) * TILE_SIZE +
					       player_half_size + penetration_limit;
		}
	}
	playerY = newPlayerY_candidate; // Apply the (potentially adjusted) Y position

	// --- Final Fallback (should be rarely triggered with per-axis sliding) ---
	// This is a safety net to teleport back if, despite axis-specific adjustments,
	// the player still ends up inside a wall (e.g., complex corner cases).
	int finalTileX_tl = (int)((playerX - player_half_size) / TILE_SIZE);
	int finalTileY_tl = (int)((playerY - player_half_size) / TILE_SIZE);
	int finalTileX_tr = (int)((playerX + player_half_size) / TILE_SIZE);
	int finalTileY_tr = (int)((playerY - player_half_size) / TILE_SIZE);
	int finalTileX_bl = (int)((playerX - player_half_size) / TILE_SIZE);
	int finalTileY_bl = (int)((playerY + player_half_size) / TILE_SIZE);
	int finalTileX_br = (int)((playerX + player_half_size) / TILE_SIZE);
	int finalTileY_br = (int)((playerY + player_half_size) / TILE_SIZE);

	if (isWall(finalTileX_tl, finalTileY_tl) || isWall(finalTileX_tr, finalTileY_tr) ||
	    isWall(finalTileX_bl, finalTileY_bl) || isWall(finalTileX_br, finalTileY_br)) {
		// If still in a wall, revert to the last truly safe position
		playerX = prevPlayerX;
		playerY = prevPlayerY;
	}
}

// New function to handle player rotation
void handlePlayerRotation(float dt)
{
	if (sdk_inputs.joy_x > 500 || sdk_inputs.b) {
		playerAngle += ROTATE_SPEED * dt;
	} else if (sdk_inputs.joy_x < -500 || sdk_inputs.x) {
		playerAngle -= ROTATE_SPEED * dt;
	}
}

void game_start(void)
{
	sdk_set_output_gain_db(volume);
	playerX = TILE_SIZE * 1.5f;
	playerY = TILE_SIZE * 1.5f;
	playerAngle = (float)M_PI / 2.0f;
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.a == 1 && sdk_inputs.start) {
		if (debug_mode) {
			debug_mode = false;
		} else
			debug_mode = true;
	}
	if (sdk_inputs_delta.b == 1 && sdk_inputs.start) {
		if (menu_active) {
			menu_active = false;
		} else
			menu_active = true;
	}
	game_handle_audio(dt, volume);
	handlePlayerMovement(dt);
	handlePlayerRotation(dt);
}

void game_paint(unsigned dt_usec)
{
	tft_fill(0);
	renderGame();
	if (menu_active) {
		drawMenuMap();
	}
	if (debug_mode) {
		drawMinimap_Debug();
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = WHITE,
	};
	sdk_main(&config);
	return 0;
}
