#include <sdk.h>    // Explicitly include sdk.h
#include "common.h" // Includes sdk.h, tft.h, and common game states/structs
#include <stdio.h>  // For snprintf
#include <stdlib.h> // For rand
#include <math.h>   // For sinf, cosf, asinf, M_PI
#include <string.h> // For strlen

// --- Local utility function for string width (since tft_measure_string_width might not be linked) ---
// Assuming a fixed-width font. A common character width for basic fonts is 6 pixels.
#define DEFAULT_CHAR_WIDTH 6
static int measure_string_width_local(const char *str)
{
	if (str == NULL) {
		return 0;
	}
	return strlen(str) * DEFAULT_CHAR_WIDTH;
}

// --- Game Constants (Copied from user's main.c) ---
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT (int)(TFT_HEIGHT / 3)
// #define PADDLE_MID (PADDLE_HEIGHT / 2) // Not directly used in current logic, but kept for context
#define BALL_WIDTH 5
#define BALL_HEIGHT 5
#define PADDLE_SPEED 100

#define BALL_SPEEDUP 1.05f
#define BALL_SPEED 100
#define BALL_SPEED_DIAG 70

// Colors are already defined in common.h: WHITE, BLUE, BLACK, LGRAY, GREEN
#ifndef GREEN
#define GREEN rgb_to_rgb565(0, 191, 0)
#endif
#ifndef GRAY
#define GRAY rgb_to_rgb565(63, 63, 63)
#endif

// --- Game Structures (Copied from user's main.c) ---
struct paddle {
	float y;
	int score;
	uint16_t color; // Added color to paddle struct
};

struct ball {
	float x, y;
	float dx, dy;
	uint16_t color;
	float speed;
};

// --- Static Game State Variables (These are local to round.c) ---
static struct paddle paddle1, paddle2;
static struct ball ball;

// Static variables to hold the last transmission times for rate limiting
static uint32_t last_tx_ball_state_time = 0;
static uint32_t last_tx_round_countdown_time = 0;
static uint32_t last_tx_paddle_state_time = 0;	      // For host's paddle state updates
static uint32_t last_tx_client_paddle_state_time = 0; // New: For client's paddle state updates

// --- Helper Functions (Copied from user's main.c) ---

/**
 * @brief Checks if two rectangles overlap.
 * @param x0, y0 Top-left corner of rect 1.
 * @param x1, y1 Bottom-right corner of rect 1.
 * @param a0, b0 Top-left corner of rect 2.
 * @param a1, b1 Bottom-right corner of rect 2.
 * @return True if rectangles overlap, false otherwise.
 */
static bool rects_overlap(int x0, int y0, int x1, int y1, int a0, int b0, int a1, int b1)
{
	int tmp;

	if (x0 > x1)
		tmp = x1, x1 = x0, x0 = tmp;
	if (a0 > a1)
		tmp = a1, a1 = a0, a0 = tmp;
	if (y0 > y1)
		tmp = y1, y1 = y0, y0 = tmp;
	if (b0 > b1)
		tmp = b1, b1 = b0, b0 = tmp;

	if (x1 < a0)
		return false;
	if (x0 > a1)
		return false;
	if (y1 < b0)
		return false;
	if (y0 > b1)
		return false;

	return true;
}

/**
 * @brief Resets the ball and paddle positions for a new round.
 * Host also explicitly sets dx/dy for initial movement.
 * Client only resets position and waits for host's dx/dy.
 */
static void new_round(void)
{
	// Paddles start at PADDLE_HEIGHT * 1 from the top, as in user's pong main.c
	paddle1.y = PADDLE_HEIGHT * 1;
	paddle2.y = PADDLE_HEIGHT * 1;

	// Assign colors to paddles directly
	paddle1.color = BLUE;  // P1 paddle is always BLUE
	paddle2.color = GREEN; // P2 paddle is always GREEN

	// Ball starts centered, as in user's pong main.c
	ball.x = TFT_WIDTH / 2.0f;
	ball.y = TFT_HEIGHT / 2.0f;

	ball.speed = BALL_SPEED; // Reset speed

	bool is_p1_local = (game_our_id == players[0].id);
	if (is_p1_local) {
		// Host sets initial random direction for dx and dy, using BALL_SPEED_DIAG for both
		ball.dx = (rand() % 2 == 0) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;
		ball.dy = (rand() % 2 == 0) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;
		printf("DEBUG: Host new_round: ball.dx=%.1f, ball.dy=%.1f.\n", ball.dx, ball.dy);
	} else {
		// Client only resets position. dx/dy will be updated by MSG_BALL_STATE from host.
		// Client starts with zero velocity until it receives an update.
		ball.dx = 0;
		ball.dy = 0;
		printf("DEBUG: Client new_round: ball position reset. dx/dy initialized to (0,0).\n");
	}

	ball.color = WHITE;
	printf("DEBUG: Device %04x: Ball position reset for new round countdown. Ball at (%.1f, %.1f).\n",
	       game_our_id, ball.x, ball.y);
}

/**
 * @brief Resets the game score and starts a new round.
 * This function is globally declared in sdk/game.h, so no 'static' keyword here.
 */
void game_reset(void)
{ // This definition is correct as it's declared extern in SDK
	paddle1.score = 0;
	paddle2.score = 0;
	new_round(); // Reset ball and paddle positions and colors
	printf("DEBUG: Device %04x: Game reset. Scores to 0.\n", game_our_id);
}

/**
 * @brief Sends the current ball state (x, y, dx, dy) over RF.
 * Floats are scaled to int16_t for transmission.
 * @param force_send If true, sends immediately bypassing rate limit. Used for critical initial sync.
 */
static void tx_ball_state(bool force_send)
{
	uint32_t now = time_us_32();

	if (!force_send && (now - last_tx_ball_state_time < BALL_STATE_TX_INTERVAL_US)) {
		return;
	}
	last_tx_ball_state_time = now;

	// Scale floats to int16_t for transmission (multiply by 100)
	int16_t x_scaled = (int16_t)(ball.x * 100);
	int16_t y_scaled = (int16_t)(ball.y * 100);
	int16_t dx_scaled = (int16_t)(ball.dx * 100);
	int16_t dy_scaled = (int16_t)(ball.dy * 100);

	uint8_t msg[9];
	msg[0] = MSG_BALL_STATE;
	msg[1] = (x_scaled >> 8) & 0xFF;  // High byte of x
	msg[2] = x_scaled & 0xFF;	  // Low byte of x
	msg[3] = (y_scaled >> 8) & 0xFF;  // High byte of y
	msg[4] = y_scaled & 0xFF;	  // Low byte of y
	msg[5] = (dx_scaled >> 8) & 0xFF; // High byte of dx
	msg[6] = dx_scaled & 0xFF;	  // Low byte of dx
	msg[7] = (dy_scaled >> 8) & 0xFF; // High byte of dy
	msg[8] = dy_scaled & 0xFF;	  // Low byte of dy

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent BALL_STATE (x:%.1f, y:%.1f, dx:%.1f, dy:%.1f)\n",
	       game_our_id, ball.x, ball.y, ball.dx, ball.dy);
}

/**
 * @brief Sends the current ball state (x, y, dx, dy) over RF when a bounce occurs.
 * This function always forces the send.
 */
static void tx_ball_bounce(void)
{
	// Scale floats to int16_t for transmission (multiply by 100)
	int16_t x_scaled = (int16_t)(ball.x * 100);
	int16_t y_scaled = (int16_t)(ball.y * 100);
	int16_t dx_scaled = (int16_t)(ball.dx * 100);
	int16_t dy_scaled = (int16_t)(ball.dy * 100);

	uint8_t msg[9];
	msg[0] = MSG_BALL_BOUNCE;	  // Use the new message type
	msg[1] = (x_scaled >> 8) & 0xFF;  // High byte of x
	msg[2] = x_scaled & 0xFF;	  // Low byte of x
	msg[3] = (y_scaled >> 8) & 0xFF;  // High byte of y
	msg[4] = y_scaled & 0xFF;	  // Low byte of y
	msg[5] = (dx_scaled >> 8) & 0xFF; // High byte of dx
	msg[6] = dx_scaled & 0xFF;	  // Low byte of dx
	msg[7] = (dy_scaled >> 8) & 0xFF; // High byte of dy
	msg[8] = dy_scaled & 0xFF;	  // Low byte of dy

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent BALL_BOUNCE (x:%.1f, y:%.1f, dx:%.1f, dy:%.1f)\n",
	       game_our_id, ball.x, ball.y, ball.dx, ball.dy);
}

/**
 * @brief Sends the host's paddle state (y position and color) over RF.
 */
static void tx_paddle_state(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_paddle_state_time < PADDLE_STATE_TX_INTERVAL_US) {
		return;
	}
	last_tx_paddle_state_time = now;

	// Scale y position to int16_t for transmission (multiply by 100)
	int16_t y_scaled = (int16_t)(paddle1.y * 100);

	// Message format: MSG_PADDLE_STATE (1 byte) + y_scaled (2 bytes) + color (2 bytes) = 5 bytes
	uint8_t msg[5];
	msg[0] = MSG_PADDLE_STATE;
	msg[1] = (y_scaled >> 8) & 0xFF;      // High byte of y
	msg[2] = y_scaled & 0xFF;	      // Low byte of y
	msg[3] = (paddle1.color >> 8) & 0xFF; // High byte of color
	msg[4] = paddle1.color & 0xFF;	      // Low byte of color

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent PADDLE_STATE (y:%.1f, color:%04x)\n", game_our_id,
	       paddle1.y, paddle1.color);
}

/**
 * @brief Sends the client's paddle state (y position) over RF.
 */
static void tx_client_paddle_state(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_client_paddle_state_time < CLIENT_PADDLE_STATE_TX_INTERVAL_US) {
		return;
	}
	last_tx_client_paddle_state_time = now;

	// Scale y position to int16_t for transmission (multiply by 100)
	int16_t y_scaled = (int16_t)(paddle2.y * 100);

	// Message format: MSG_CLIENT_PADDLE_STATE (1 byte) + y_scaled (2 bytes) = 3 bytes
	uint8_t msg[3];
	msg[0] = MSG_CLIENT_PADDLE_STATE;
	msg[1] = (y_scaled >> 8) & 0xFF; // High byte of y
	msg[2] = y_scaled & 0xFF;	 // Low byte of y

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent CLIENT_PADDLE_STATE (y:%.1f)\n", game_our_id, paddle2.y);
}

/**
 * @brief Sends a message to signal the start of the round countdown.
 * Includes current scores and the host's current timer value.
 */
static void tx_round_start_countdown(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_round_countdown_time < ROUND_START_COUNTDOWN_TX_INTERVAL_US) {
		return;
	}
	last_tx_round_countdown_time = now;

	// Message format: MSG_ROUND_START_COUNTDOWN (1 byte) + score1 (1 byte) + score2 (1 byte)
	// + game_state_timer (4 bytes, uint32_t) = 7 bytes
	uint8_t msg[7];
	msg[0] = MSG_ROUND_START_COUNTDOWN;
	msg[1] = (uint8_t)paddle1.score;
	msg[2] = (uint8_t)paddle2.score;
	msg[3] = (game_state_timer >> 24) & 0xFF; // MSB
	msg[4] = (game_state_timer >> 16) & 0xFF;
	msg[5] = (game_state_timer >> 8) & 0xFF;
	msg[6] = game_state_timer & 0xFF; // LSB

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent MSG_ROUND_START_COUNTDOWN (P1:%d, P2:%d, Timer:%d)\n",
	       game_our_id, paddle1.score, paddle2.score, game_state_timer);
}

/**
 * @brief Host-side function to initiate the 2-second round reset countdown.
 */
static void start_new_round_countdown(void)
{
	printf("DEBUG: Device %04x: Host initiating new round countdown.\n", game_our_id);
	game_state_change(STATE_ROUND_RESET); // Enter the countdown state (resets timer to 0)
	new_round(); // Reset ball and paddle positions and set initial velocity
		     // Host sends countdown message during tick
}

// --- Scene Functions ---

/**
 * @brief Scene paint function: Draws the Pong game elements.
 * @param dt Delta time (unused in this simple paint function).
 * @param depth Current rendering depth (only draw if depth is 0).
 */
static void round_paint(float dt, int depth)
{
	(void)dt;    // Suppress unused parameter warning
	(void)depth; // Suppress unused parameter warning

	if (0 != depth) {
		return; // Only paint for the base depth
	}

	tft_fill(BLACK); // Clear the screen

	// Draw center dashed line
	for (int i = 0; i < TFT_HEIGHT; i += 20) {
		// tft_draw_rect(x0, y0, x1, y1, color)
		tft_draw_rect(TFT_WIDTH / 2 - 1, i + 5, TFT_WIDTH / 2 + 1, i + 10 + 5, GRAY);
	}

	// Draw paddles
	// Left paddle (P1) - use paddle1.color
	tft_draw_rect(0, paddle1.y, PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT - 1, paddle1.color);
	// Right paddle (P2) - use paddle2.color
	tft_draw_rect(TFT_WIDTH - PADDLE_WIDTH, paddle2.y, TFT_WIDTH - 1,
		      paddle2.y + PADDLE_HEIGHT - 1, paddle2.color);

	/* draw ball */
	tft_draw_rect(ball.x, ball.y, ball.x + BALL_WIDTH - 1, ball.y + BALL_HEIGHT - 1,
		      ball.color);

	// Draw scores
	tft_draw_string(TFT_WIDTH / 4 - 5, 0, BLUE, "%i", paddle1.score); // P1 score
	tft_draw_string_right(TFT_WIDTH - (TFT_WIDTH / 4) + 5, 0, GREEN, "%i",
			      paddle2.score); // P2 score

	// Display countdown if in STATE_ROUND_RESET
	if (game_state == STATE_ROUND_RESET) {
		char countdown_str[10];
		int seconds_left = 2 - (game_state_timer / 1000000);
		if (seconds_left < 0)
			seconds_left = 0; // Clamp to 0
		snprintf(countdown_str, sizeof(countdown_str), "%d", seconds_left);
		// Center the countdown text using the local helper
		tft_draw_string(TFT_WIDTH / 2 - (measure_string_width_local(countdown_str) / 2),
				TFT_HEIGHT / 2 - 10, WHITE, countdown_str);
	}
}

/**
 * @brief Scene handle function: Processes discrete input events for paddle movement.
 * @param event The SDK event that occurred (e.g., button press).
 * @param depth Current scene depth (only handle if depth is 0).
 * @return True if the event was handled, false otherwise.
 */
static bool round_handle(sdk_event_t event, int depth)
{
	(void)event; // Suppress unused parameter warning
	(void)depth; // Suppress unused parameter warning

	// Input is only active during PLAYING state
	if (game_state != STATE_PLAYING) {
		return false;
	}

	// No discrete button presses for paddle movement anymore,
	// as joystick provides continuous input in round_tick.
	return false; // Event not handled by this function
}

/**
 * @brief Scene tick function: Updates continuous game logic (ball movement, collisions, scoring, and paddle movements).
 * @param jiffies Microseconds elapsed since the last tick.
 * @param depth Current scene depth (only tick if depth is 0).
 */
static void round_tick(int jiffies, int depth)
{
	(void)depth;			 // Suppress unused parameter warning
	float dt = jiffies / 1000000.0f; // Convert jiffies to seconds

	// Determine if this device is P1 (host) or P2 (client)
	bool is_p1_local = (game_our_id == players[0].id);
	bool is_p2_local = (game_our_id == players[1].id);

	// Handle P1 paddle movement using joystick (continuous input)
	if (is_p1_local && game_state == STATE_PLAYING) { // Only move paddle during PLAYING state
		if (sdk_inputs.joy_y > 500) {		  // Joystick down (positive Y)
			paddle1.y += PADDLE_SPEED * dt;
		} else if (sdk_inputs.joy_y < -500) { // Joystick up (negative Y)
			paddle1.y -= PADDLE_SPEED * dt;
		}
		// Clamp P1 paddle position
		if (paddle1.y < 0)
			paddle1.y = 0;
		else if (paddle1.y > TFT_HEIGHT - PADDLE_HEIGHT)
			paddle1.y = TFT_HEIGHT - PADDLE_HEIGHT;

		// Host sends its own paddle state periodically
		tx_paddle_state();
		// Host sends ball state periodically ONLY during PLAYING
		tx_ball_state(false); // Do not force send, use rate limit (now 0.25 seconds)
	}

	// Handle P2 paddle movement using joystick (continuous input)
	if (is_p2_local && game_state == STATE_PLAYING) { // Only move paddle during PLAYING state
		if (sdk_inputs.joy_y > 500) {		  // Joystick down (positive Y)
			paddle2.y += PADDLE_SPEED * dt;
		} else if (sdk_inputs.joy_y < -500) { // Joystick up (negative Y)
			paddle2.y -= PADDLE_SPEED * dt;
		}
		// Clamp P2 paddle position
		if (paddle2.y < 0)
			paddle2.y = 0;
		else if (paddle2.y > TFT_HEIGHT - PADDLE_HEIGHT)
			paddle2.y = TFT_HEIGHT - PADDLE_HEIGHT;

		// Client sends its own paddle state periodically
		tx_client_paddle_state(); // New: Client sends its paddle state
	}

	// Ball movement (executed on BOTH host and client if in PLAYING state)
	// Client will use received dx/dy to predict movement between updates
	if (game_state == STATE_PLAYING) {
		ball.x += ball.dx * dt;
		ball.y += ball.dy * dt;
		printf("DEBUG: Device %04x: ball.x=%.1f, y=%.1f, dx=%.1f, dy=%.1f\n", game_our_id,
		       ball.x, ball.y, ball.dx, ball.dy);
	}

	// Ball physics and scoring (executed ONLY on host if in PLAYING state)
	if (is_p1_local && game_state == STATE_PLAYING) {
		/* ball can bounce from top and bottom */
		if (ball.y < 0) {
			ball.dy *= -1;
			ball.y = 0;
			tx_ball_bounce(); // Send bounce update
		} else if (ball.y > TFT_BOTTOM - BALL_HEIGHT) {
			ball.dy *= -1;
			ball.y = TFT_BOTTOM - BALL_HEIGHT;
			tx_ball_bounce(); // Send bounce update
		}

		// Collision with left paddle (P1's paddle)
		if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT, 0,
				  paddle1.y, PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT)) {
			ball.color = BLUE;
			float mid = paddle1.y + PADDLE_HEIGHT / 2.0f;
			float ballmid = ball.y + BALL_HEIGHT / 2.0f;
			float impact =
				(mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
			float refl = asinf(impact);

			ball.speed *= BALL_SPEEDUP;
			ball.dx = cosf(refl) * ball.speed;
			ball.dy = -sinf(refl) * ball.speed;
			ball.x = PADDLE_WIDTH +
				 1; // Move ball slightly past paddle to prevent sticking
			printf("DEBUG: Device %04x: Ball hit P1 paddle. New speed: %.1f\n",
			       game_our_id, ball.speed);
			tx_ball_bounce(); // Send bounce update
		}
		// Ball missed left paddle (P1 side)
		else if (ball.x < PADDLE_WIDTH - 1) {
			paddle2.score++; // P2 scores
			printf("DEBUG: Device %04x: P1 missed. P2 score: %d\n", game_our_id,
			       paddle2.score);
			start_new_round_countdown(); // Initiate countdown after score
		}

		// Collision with right paddle (P2's paddle)
		if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT,
				  TFT_WIDTH - PADDLE_WIDTH, paddle2.y, TFT_WIDTH - 1,
				  paddle2.y + PADDLE_HEIGHT)) {
			ball.color = GREEN;
			float mid = paddle2.y + PADDLE_HEIGHT / 2.0f;
			float ballmid = ball.y + BALL_HEIGHT / 2.0f;
			float impact =
				(mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
			float refl = asinf(impact) + M_PI; // Reflect angle for right paddle

			ball.speed *= BALL_SPEEDUP;
			ball.dx = cosf(refl) * ball.speed;
			ball.dy = sinf(refl) * ball.speed;
			ball.x = TFT_WIDTH - PADDLE_WIDTH - BALL_WIDTH -
				 1; // Move ball slightly past paddle
			printf("DEBUG: Device %04x: Ball hit P2 paddle. New speed: %.1f\n",
			       game_our_id, ball.speed);
			tx_ball_bounce(); // Send bounce update
		}
		// Ball missed right paddle (P2 side)
		else if (ball.x + BALL_WIDTH > TFT_WIDTH - PADDLE_WIDTH + 1) {
			paddle1.score++; // P1 scores
			printf("DEBUG: Device %04x: P2 missed. P1 score: %d\n", game_our_id,
			       paddle1.score);
			start_new_round_countdown(); // Initiate countdown after score
		}
	}

	// Handle STATE_ROUND_RESET countdown
	if (game_state == STATE_ROUND_RESET) {
		if (game_state_timer >= 2000000) { // 2 seconds elapsed
			printf("DEBUG: Device %04x: Round reset countdown finished. Transitioning to PLAYING.\n",
			       game_our_id);
			game_state_change(STATE_PLAYING);
			// Host sends initial ball state immediately after countdown
			if (is_p1_local) {
				tx_ball_state(true); // Force send the initial ball state
			}
		} else {
			// During countdown, host sends round start countdown message
			bool is_p1_local = (game_our_id == players[0].id);
			if (is_p1_local) {
				tx_round_start_countdown(); // Host keeps sending score/timer sync during countdown
			}
		}
	}
}

/**
 * @brief Scene pushed function: Called once when the 'round' scene becomes active.
 * Initializes game-specific variables here.
 */
static void round_pushed(void)
{
	printf("Round scene pushed! Stopping menu music.\n");
	// Stop menu music when game scene is pushed
	if (menu_melody != NULL) {
		sdk_melody_stop_and_release(menu_melody);
		menu_melody = NULL;
	}
	// game_reset() is now called from root_inbox for clients when MSG_GAME_START is received.
	// Host calls game_reset() from root_handle when all players are ready.
}

/**
 * @brief Scene revealed function: Called when the 'round' scene is revealed
 * (e.g., if another scene was temporarily pushed on top and then popped).
 */
static void round_revealed(void)
{
	printf("Round scene revealed!\n");
	// No specific action needed here beyond what game_reset() does on initial push
}

/**
 * @brief Scene inbox function: Processes incoming radio messages.
 * @param msg The received SDK message.
 * @param depth Current scene depth (only process if depth is 0 and it's an RF message).
 * @return True if the message was handled, false otherwise.
 */
static bool round_inbox(sdk_message_t msg, int depth)
{
	(void)depth; // Suppress unused parameter warning

	if (msg.type != SDK_MSG_RF) {
		return false; // Only process RF messages
	}

	switch (msg.rf.data[0]) {
	case MSG_BALL_STATE:
	case MSG_BALL_BOUNCE:		  // Handle MSG_BALL_BOUNCE the same way as MSG_BALL_STATE
		if (msg.rf.length == 9) { // Message + 4 x int16_t
			// Unpack int16_t values
			int16_t x_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];
			int16_t y_scaled = (msg.rf.data[3] << 8) | msg.rf.data[4];
			int16_t dx_scaled = (msg.rf.data[5] << 8) | msg.rf.data[6];
			int16_t dy_scaled = (msg.rf.data[7] << 8) | msg.rf.data[8];

			// Convert back to float (divide by 100) and overwrite client's state
			ball.x = (float)x_scaled / 100.0f;
			ball.y = (float)y_scaled / 100.0f;
			ball.dx = (float)dx_scaled / 100.0f;
			ball.dy = (float)dy_scaled / 100.0f;

			printf("DEBUG: Device %04x: Client received BALL_%s. New x:%.1f, y:%.1f, dx:%.1f, dy:%.1f\n",
			       game_our_id, (msg.rf.data[0] == MSG_BALL_STATE ? "STATE" : "BOUNCE"),
			       ball.x, ball.y, ball.dx, ball.dy);
			return true;
		}
		return false;		// Invalid message length
	case MSG_ROUND_START_COUNTDOWN: // Client receives this from host to start countdown
		// Expected length: MSG_ROUND_START_COUNTDOWN (1) + score1 (1) + score2 (1) + timer (4) = 7 bytes
		if (msg.rf.length == 7) {
			paddle1.score = (int)msg.rf.data[1];
			paddle2.score = (int)msg.rf.data[2];
			// Extract the timer value
			uint32_t received_timer = (msg.rf.data[3] << 24) | (msg.rf.data[4] << 16) |
						  (msg.rf.data[5] << 8) | msg.rf.data[6];

			printf("DEBUG: Device %04x: Received MSG_ROUND_START_COUNTDOWN (P1:%d, P2:%d, Host Timer:%d). Starting countdown.\n",
			       game_our_id, paddle1.score, paddle2.score, received_timer);

			game_state_change(
				STATE_ROUND_RESET); // Client enters countdown state (resets timer to 0)
			game_state_timer =
				received_timer; // Immediately set client's timer to host's value
			new_round(); // Client calls new_round to reset ball position (dx/dy will be 0 initially)
			return true;
		}
		return false;  // Invalid message length
	case MSG_PADDLE_STATE: // Client receives host's paddle state (P1)
		// Expected length: MSG_PADDLE_STATE (1) + y_scaled (2) + color (2) = 5 bytes
		if (msg.rf.length == 5) {
			int16_t y_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];
			uint16_t received_color = (msg.rf.data[3] << 8) | msg.rf.data[4];

			paddle1.y = (float)y_scaled / 100.0f;
			// Client's paddle1 (left) will take the color from the host's paddle1
			paddle1.color = received_color;

			printf("DEBUG: Device %04x: Client received PADDLE_STATE (y:%.1f, color:%04x).\n",
			       game_our_id, paddle1.y, paddle1.color);
			return true;
		}
		return false;	      // Invalid message length
	case MSG_CLIENT_PADDLE_STATE: // Host receives client's paddle state (P2)
		// Expected length: MSG_CLIENT_PADDLE_STATE (1) + y_scaled (2) = 3 bytes
		if (msg.rf.length == 3) {
			int16_t y_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];

			paddle2.y = (float)y_scaled / 100.0f;

			printf("DEBUG: Device %04x: Host received CLIENT_PADDLE_STATE (y:%.1f).\n",
			       game_our_id, paddle2.y);
			return true;
		}
		return false; // Invalid message length
	default:
		return false; // Message not handled
	}
	return false; // Message not handled
}

// --- SDK Scene Definition for the Game Round ---
// This structure defines the functions that make up the 'round' (game) scene.
sdk_scene_t scene_round = {
	.paint = round_paint,
	.handle = round_handle,
	.tick = round_tick,
	.pushed = round_pushed,
	.revealed = round_revealed,
	.inbox = round_inbox,
};
