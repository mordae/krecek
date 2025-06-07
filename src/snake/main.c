#include <pico/stdlib.h>

#include <stdbool.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

//#include <cover.png.h>

//sdk_game_info("snake", &image_cover_png);

#define RED rgb_to_rgb565(255, 0, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define SPACE_SIZE 5
#define MOVE_WAIT 150000

static int score = 0;
static int snake_head_x = 5;
static int snake_head_y = 5;
static int snake_delta_x = 0;
static int snake_delta_y = 0;
static bool snake_collided = false;

static int snake_trail_x[200]; // the positions of all parts of the snake (back to front)
static int snake_trail_y[200];

static int fruit_x = 10;
static int fruit_y = 10;
static int fruit_color = RED;
static int fruit_posible_colors[3] = { RED, YELLOW, BLUE };

static int since_last_move = 0;

void game_reset(void)
{
	score = 0;
	snake_head_x = 5;
	snake_head_y = 5;
	snake_delta_x = 0;
	snake_delta_y = 0;
	snake_collided = false;
	fruit_x = 10;
	fruit_y = 10;

	// initialise the snake part possitions to a default value
	for (int i = 0; i < 200; i++) {
		snake_trail_x[i] = 0;
		snake_trail_y[i] = 0;
	}
}

void game_start(void)
{
}

void game_input(unsigned dt_usec)
{
	if (sdk_inputs_delta.b > 0) {
		if (score == 0) {
			snake_delta_x = 1;
			snake_delta_y = 0;
		} else if (snake_head_x + 1 != snake_trail_x[score - 1]) {
			snake_delta_x = 1;
			snake_delta_y = 0;
		}
	}
	if (sdk_inputs_delta.x > 0) {
		if (score == 0) {
			snake_delta_x = -1;
			snake_delta_y = 0;
		} else if (snake_head_x - 1 != snake_trail_x[score - 1]) {
			snake_delta_x = -1;
			snake_delta_y = 0;
		}
	}
	if (sdk_inputs_delta.a > 0) {
		if (score == 0) {
			snake_delta_x = 0;
			snake_delta_y = 1;
		} else if (snake_head_y + 1 != snake_trail_y[score - 1]) {
			snake_delta_x = 0;
			snake_delta_y = 1;
		}
	}
	if (sdk_inputs_delta.y > 0) {
		if (score == 0) {
			snake_delta_x = 0;
			snake_delta_y = -1;
		} else if (snake_head_y - 1 != snake_trail_y[score - 1]) {
			snake_delta_x = 0;
			snake_delta_y = -1;
		}
	}

	if (since_last_move > MOVE_WAIT) { // handle snake movement
		since_last_move -= MOVE_WAIT;
		snake_head_x += snake_delta_x;
		snake_head_y += snake_delta_y;

		for (int snake_cell = 0; snake_cell <= score - 1; snake_cell++) {
			if (snake_trail_x[snake_cell] == snake_head_x &&
			    snake_trail_y[snake_cell] == snake_head_y)
				snake_collided = true;
		}
		if (snake_collided)
			game_reset();
		else {
			if (snake_head_x == fruit_x && snake_head_y == fruit_y) {
				score++;
				fruit_x = rand() % 32;
				fruit_y = rand() % 24;
				fruit_color = fruit_posible_colors[rand() % 3];
			} else if (score > 0) {
				for (int snake_cell = 0; snake_cell <= score - 1; snake_cell++) {
					snake_trail_x[snake_cell] = snake_trail_x[snake_cell + 1];
					snake_trail_y[snake_cell] = snake_trail_y[snake_cell + 1];
				}
			}
		}

		snake_trail_x[score] = snake_head_x;
		snake_trail_y[score] = snake_head_y;
	}

	since_last_move += dt_usec;

	if (snake_head_x < 0)
		game_reset();
	if (snake_head_x > 31)
		game_reset();
	if (snake_head_y < 0)
		game_reset();
	if (snake_head_y > 23)
		game_reset();
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	tft_draw_string(0, 0, RED, "%i", score);

	// draw all snake parts
	for (int snake_cell = 0; snake_cell <= score; snake_cell++) {
		tft_draw_rect(SPACE_SIZE * snake_trail_x[snake_cell],
			      SPACE_SIZE * snake_trail_y[snake_cell],
			      SPACE_SIZE * snake_trail_x[snake_cell] + SPACE_SIZE - 1,
			      SPACE_SIZE * snake_trail_y[snake_cell] + SPACE_SIZE - 1, GREEN);
	}
	// draw fruit
	tft_draw_rect(SPACE_SIZE * fruit_x, SPACE_SIZE * fruit_y,
		      SPACE_SIZE * fruit_x + SPACE_SIZE - 1, SPACE_SIZE * fruit_y + SPACE_SIZE - 1,
		      fruit_color);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
