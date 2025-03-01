#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>

#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120

#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 10
#define GROUND_Y (SCREEN_HEIGHT - 10)
#define GRAVITY 0.5f
#define JUMP_STRENGTH -5.5f
#define MOVE_SPEED 2

#define NUM_PLATFORMS 5
#define NUM_ENEMIES 2

struct player {
	float x, y;
	float dy;
	int on_ground;
	int alive;
	int won;
};

struct platform {
	int x, y, width, height;
};

struct enemy {
	int x, y, width, height;
	int direction;
};

static struct player mario;
static struct platform platforms[NUM_PLATFORMS];
static struct enemy enemies[NUM_ENEMIES];

void game_start(void)
{
	sdk_set_output_gain_db(6);
	tft_palette[1] = rgb_to_rgb565(255, 0, 0);
	tft_palette[2] = rgb_to_rgb565(0, 255, 0);
	tft_palette[3] = rgb_to_rgb565(255, 255, 0);
	tft_palette[4] = rgb_to_rgb565(0, 0, 255);

	mario.x = 10;
	mario.y = GROUND_Y - PLAYER_HEIGHT;
	mario.dy = 0;
	mario.on_ground = 1;
	mario.alive = 1;
	mario.won = 0;

	platforms[0] = (struct platform){ 30, 90, 40, 5 };
	platforms[1] = (struct platform){ 80, 70, 40, 5 };
	platforms[2] = (struct platform){ 50, 50, 30, 5 };
	platforms[3] = (struct platform){ 100, 30, 30, 5 };
	platforms[4] = (struct platform){ 120, 20, 20, 5 };

	enemies[0] = (struct enemy){ 60, GROUND_Y - 10, 10, 10, 1 };
	enemies[1] = (struct enemy){ 110, 50, 10, 10, -1 };
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned __unused dt_usec)
{
	if (!mario.alive || mario.won) {
		if (sdk_inputs.start) {
			game_reset();
		}
		return;
	}

	if (sdk_inputs.x)
		mario.x -= MOVE_SPEED;
	if (sdk_inputs.a)
		mario.x += MOVE_SPEED;
	if (sdk_inputs.y && mario.on_ground) {
		mario.dy = JUMP_STRENGTH;
		mario.on_ground = 0;
	}

	mario.dy += GRAVITY;
	mario.y += mario.dy;

	if (mario.y >= GROUND_Y - PLAYER_HEIGHT) {
		mario.y = GROUND_Y - PLAYER_HEIGHT;
		mario.dy = 0;
		mario.on_ground = 1;
	}

	for (int i = 0; i < NUM_PLATFORMS; i++) {
		if (mario.x + PLAYER_WIDTH > platforms[i].x &&
		    mario.x < platforms[i].x + platforms[i].width &&
		    mario.y + PLAYER_HEIGHT >= platforms[i].y &&
		    mario.y + PLAYER_HEIGHT <= platforms[i].y + platforms[i].height) {
			mario.y = platforms[i].y - PLAYER_HEIGHT;
			mario.dy = 0;
			mario.on_ground = 1;
		}
	}

	for (int i = 0; i < NUM_ENEMIES; i++) {
		if (mario.x + PLAYER_WIDTH > enemies[i].x &&
		    mario.x < enemies[i].x + enemies[i].width &&
		    mario.y + PLAYER_HEIGHT > enemies[i].y &&
		    mario.y < enemies[i].y + enemies[i].height) {
			mario.alive = 0;
		}
	}

	if (mario.x > SCREEN_WIDTH - 15) {
		mario.won = 1;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	if (!mario.alive) {
		tft_draw_rect(30, 50, 110, 70, 1);
		return;
	}

	if (mario.won) {
		tft_draw_rect(30, 50, 110, 70, 3);
		return;
	}

	tft_draw_rect(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 2);

	for (int i = 0; i < NUM_PLATFORMS; i++) {
		tft_draw_rect(platforms[i].x, platforms[i].y, platforms[i].x + platforms[i].width,
			      platforms[i].y + platforms[i].height, 2);
	}

	for (int i = 0; i < NUM_ENEMIES; i++) {
		enemies[i].x += enemies[i].direction;
		if (enemies[i].x < 50 || enemies[i].x > 120)
			enemies[i].direction *= -1;
		tft_draw_rect(enemies[i].x, enemies[i].y, enemies[i].x + enemies[i].width,
			      enemies[i].y + enemies[i].height, 4);
	}

	tft_draw_rect(mario.x, mario.y, mario.x + PLAYER_WIDTH, mario.y + PLAYER_HEIGHT, 1);

	tft_draw_rect(SCREEN_WIDTH - 10, GROUND_Y - 20, SCREEN_WIDTH - 5, GROUND_Y, 3);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = 3,
	};
	sdk_main(&config);
}
