#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120

#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 10
#define GROUND_Y (SCREEN_HEIGHT - 10)
#define GRAVITY 0.5f
#define JUMP_STRENGTH -5.5f
#define MOVE_SPEED 2.0f
#define MAX_SPEED 3.0f
#define FRICTION 0.95f
#define SPEED_SCALE 100.0f // Scale factor to adjust movement speed
#define ENEMY_SPEED 20.0f  // Enemy speed in pixels per second

#define NUM_PLATFORMS 5
#define NUM_ENEMIES 2

struct player {
	float x, y;
	float dx, dy;
	int on_ground;
	int alive;
	int won;
};

struct platform {
	int x, y, width, height;
};

struct enemy {
	float x, y;
	int width, height;
	int direction;
};

static struct player mario;
static struct platform platforms[NUM_PLATFORMS];
static struct enemy enemies[NUM_ENEMIES];

static uint16_t tones[256];
static const char tune[] =
	" ggahC g C g C g CChag D g D g D ggahC g C g C g CCDEF C F C F C FFEDC g C g C g CChag D g D g D";
static bool play_music = true;

void game_start(void)
{
	sdk_set_output_gain_db(6);
	tft_palette[1] = rgb_to_rgb565(255, 0, 0);   // Mario color
	tft_palette[2] = rgb_to_rgb565(0, 255, 0);   // Platform color
	tft_palette[3] = rgb_to_rgb565(255, 255, 0); // Win screen color
	tft_palette[4] = rgb_to_rgb565(0, 0, 255);   // Enemy color

	mario.x = 10;
	mario.y = GROUND_Y - PLAYER_HEIGHT;
	mario.dx = 0;
	mario.dy = 0;
	mario.on_ground = 1;
	mario.alive = 1;
	mario.won = 0;

	// Platforms as specified:
	platforms[0] = (struct platform){ 30, 90, 40, 5 };
	platforms[1] = (struct platform){ 80, 70, 40, 5 };
	platforms[2] = (struct platform){ 50, 50, 30, 5 };
	platforms[3] = (struct platform){ 100, 30, 30, 5 };
	platforms[4] = (struct platform){ 120, 20, 20, 5 };

	// Enemies
	enemies[0] = (struct enemy){ 60, GROUND_Y - 10, 10, 10, 1 };
	enemies[1] = (struct enemy){ 110, 50, 10, 10, -1 };

	// Initialize tone frequencies for the song:
	tones['c'] = 131;
	tones['d'] = 147;
	tones['e'] = 165;
	tones['f'] = 175;
	tones['g'] = 196;
	tones['a'] = 220;
	tones['h'] = 247;
	tones['C'] = 261;
	tones['D'] = 293;
	tones['E'] = 329;
	tones['F'] = 349;
	tones['G'] = 392;
	tones['A'] = 440;
	tones['H'] = 494;
}

void game_audio(int nsamples)
{
	static int elapsed = 0;
	static int tone_pos = 0;
	if (!play_music) {
		tone_pos = 0;
		elapsed = 0;
		for (int s = 0; s < nsamples; s++)
			sdk_write_sample(0);
		return;
	}
	for (int s = 0; s < nsamples; s++) {
		if (elapsed > SDK_AUDIO_RATE / 4) {
			tone_pos++;
			elapsed = 0;
		}
		if (!tune[tone_pos]) {
			tone_pos = 0;
		}
		int freq = tones[(unsigned)tune[tone_pos]];
		if (freq) {
			int period = SDK_AUDIO_RATE / freq;
			int half_period = 2 * elapsed / period;
			int modulo = half_period & 1;
			sdk_write_sample(4000 * (modulo ? 1 : -1));
		} else {
			sdk_write_sample(0);
		}
		elapsed++;
	}
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f; // Convert microseconds to seconds
	if (!mario.alive || mario.won) {
		if (sdk_inputs.start) {
			game_start();
		}
		return;
	}

	// Horizontal movement input:
	if (sdk_inputs.x) {
		mario.dx -= MOVE_SPEED * dt;
	}
	if (sdk_inputs.a) {
		mario.dx += MOVE_SPEED * dt;
	}
	// Jump input:
	if (sdk_inputs.y && mario.on_ground) {
		mario.dy = JUMP_STRENGTH;
		mario.on_ground = 0;
	}

	// Apply friction to horizontal velocity:
	mario.dx *= FRICTION;
	if (mario.dx > MAX_SPEED)
		mario.dx = MAX_SPEED;
	if (mario.dx < -MAX_SPEED)
		mario.dx = -MAX_SPEED;

	// Update Mario's position using time-based movement:
	mario.x += mario.dx * dt * SPEED_SCALE;
	mario.dy += GRAVITY * dt * SPEED_SCALE;
	mario.y += mario.dy * dt * SPEED_SCALE;

	// Ground collision:
	if (mario.y >= GROUND_Y - PLAYER_HEIGHT) {
		mario.y = GROUND_Y - PLAYER_HEIGHT;
		mario.dy = 0;
		mario.on_ground = 1;
	}

	// Platform collision:
	for (int i = 0; i < NUM_PLATFORMS; i++) {
		struct platform p = platforms[i];
		if (mario.x + PLAYER_WIDTH > p.x && mario.x < p.x + p.width &&
		    mario.y + PLAYER_HEIGHT >= p.y && mario.y + PLAYER_HEIGHT <= p.y + p.height) {
			mario.y = p.y - PLAYER_HEIGHT;
			mario.dy = 0;
			mario.on_ground = 1;
		}
	}

	// Update enemy positions (time-based):
	for (int i = 0; i < NUM_ENEMIES; i++) {
		enemies[i].x += enemies[i].direction * ENEMY_SPEED * dt;
		if (enemies[i].x < 50 || enemies[i].x > 120) {
			enemies[i].direction *= -1;
		}
		// Check enemy collision with Mario:
		if (mario.x + PLAYER_WIDTH > enemies[i].x &&
		    mario.x < enemies[i].x + enemies[i].width &&
		    mario.y + PLAYER_HEIGHT > enemies[i].y &&
		    mario.y < enemies[i].y + enemies[i].height) {
			mario.alive = 0;
		}
	}

	// Win condition: reaching the right side of the screen
	if (mario.x > SCREEN_WIDTH - 15) {
		mario.won = 1;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	// Draw win/lose screens:
	if (!mario.alive) {
		// Lose screen (red rectangle)
		tft_draw_rect(30, 50, 110, 70, 1);
		return;
	}
	if (mario.won) {
		// Win screen (green rectangle)
		tft_draw_rect(30, 50, 110, 70, 2);
		return;
	}

	// Draw ground:
	tft_draw_rect(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 2);

	// Draw platforms:
	for (int i = 0; i < NUM_PLATFORMS; i++) {
		tft_draw_rect(platforms[i].x, platforms[i].y, platforms[i].x + platforms[i].width,
			      platforms[i].y + platforms[i].height, 2);
	}

	// Draw enemies:
	for (int i = 0; i < NUM_ENEMIES; i++) {
		tft_draw_rect(enemies[i].x, enemies[i].y, enemies[i].x + enemies[i].width,
			      enemies[i].y + enemies[i].height, 4);
	}

	// Draw Mario:
	tft_draw_rect(mario.x, mario.y, mario.x + PLAYER_WIDTH, mario.y + PLAYER_HEIGHT, 1);

	// Draw finish flag:
	tft_draw_rect(SCREEN_WIDTH - 10, GROUND_Y - 20, SCREEN_WIDTH - 5, GROUND_Y, 3);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = 3,
	};
	sdk_main(&config);
}
