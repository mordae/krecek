#define _GNU_SOURCE

#include <sdk.h>
#include <tft.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <SDL.h>
#include <unistd.h>

#define SCALE 4

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

struct sdk_config sdk_config = {};

uint64_t sdk_device_id = 0;

/* From audio.c */
void sdk_audio_init(void);
void sdk_audio_start(void);
void sdk_audio_task(void);
void sdk_audio_flush(void);

/* From video.c */
void sdk_video_init(void);

/* From input.c */
void sdk_input_init(void);
void sdk_input_handle(const SDL_Event *event);
void sdk_input_commit(uint32_t dt);

void __attribute__((__noreturn__, __format__(printf, 1, 2))) sdk_panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	puts("");

	fflush(stdout);
	exit(1);
}

static void sdl_perror(const char *prefix)
{
	fprintf(stderr, "%s: %s\n", prefix, SDL_GetError());
}

static void sdl_error(const char *prefix)
{
	sdl_perror(prefix);
	exit(1);
}

void __noreturn sdk_main(const struct sdk_config *conf)
{
	// Attempt to chdir to source directory of our game for sdcard data.
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "src/%s", basename(program_invocation_name));
	chdir(path);

	sdk_config = *conf;
	sdk_device_id = time_us_64();

	if (!sdk_config.backlight)
		sdk_config.backlight = SDK_BACKLIGHT_STD;

	if (SDL_Init(SDL_INIT_VIDEO))
		sdl_error("SDL_Init");

	SDL_Window *window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED,
					      SDL_WINDOWPOS_CENTERED, TFT_WIDTH * SCALE,
					      TFT_HEIGHT * SCALE,
					      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!window)
		sdl_error("SDL_CreateWindow");

	SDL_SetWindowMinimumSize(window, TFT_WIDTH, TFT_HEIGHT);

	SDL_Renderer *renderer = SDL_CreateRenderer(
		window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!renderer)
		sdl_error("SDL_CreateRenderer");

	if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE))
		sdl_error("SDL_SetRenderDrawBlendMode");

	if (SDL_RenderSetLogicalSize(renderer, TFT_WIDTH, TFT_HEIGHT))
		sdl_error("SDL_RenderSetLogicalSize");

	SDL_Rect viewport = { 0, 0, TFT_WIDTH, TFT_HEIGHT };
	SDL_RenderSetViewport(renderer, &viewport);

	static uint16_t *pixels;
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
						 SDL_TEXTUREACCESS_STREAMING, TFT_WIDTH,
						 TFT_HEIGHT);

	sdk_input_init();
	sdk_audio_init();
	sdk_video_init();

	game_start();
	game_reset();
	sdk_audio_start();

	SDL_Event event;
	bool done = false;

	uint64_t last_frame = time_us_64();

	while (!done) {
		// Track ellapsed time.
		uint64_t this_frame = time_us_32();
		uint32_t dt = this_frame - last_frame;
		last_frame = this_frame;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN &&
			    event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}

			if (event.type == SDL_QUIT) {
				done = true;
			}

			sdk_input_handle(&event);
		}

		// Let the game know.
		sdk_input_commit(dt);

		// Enqueue audio samples.
		sdk_audio_task();
		sdk_audio_flush();

		// Clear to gray
		SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
		SDL_RenderClear(renderer);

		// Clear active area to black
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &viewport);

		// Let the game paint into it's buffer.
		game_paint(dt);

		// Swap the buffers. Eh.
		tft_swap_buffers();
		tft_sync();

		// Prepare to paint the texture.
		int pitch;
		if (SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch))
			sdl_error("SDL_LockTexture");

		// Copy game's buffer into the texture and translate the axes,
		// because SDL2 has them the other way round.
		for (int y = 0; y < TFT_HEIGHT; y++) {
			for (int x = 0; x < TFT_WIDTH; x++) {
				pixels[y * TFT_WIDTH + x] = tft_active[x][y];
			}
		}

		// Render the texture and be done.
		SDL_UnlockTexture(texture);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	exit(0);
}

void sdk_yield_every_us(uint32_t us)
{
	(void)us;
}

__weak void game_start(void)
{
}

__weak void game_reset(void)
{
}

__weak void game_audio(int nsamples)
{
	for (int i = 0; i < nsamples; i++) {
		int16_t left, right;
		sdk_melody_sample(&left, &right);
		sdk_write_sample(left, right);

		sdk_read_sample(&left, &right);
		sdk_decode_ir(left);
	}
}

__weak void game_input(unsigned __unused dt)
{
}

__weak void game_paint(unsigned __unused dt)
{
}

__weak void game_inbox(sdk_message_t msg)
{
	(void)msg;
}

uint32_t sdk_random()
{
	return time_us_32();
}

void sdk_turn_off(void)
{
	exit(0);
}

void sdk_reboot_into_slot(unsigned slot)
{
	printf("Cannot reboot into slot %u on PC.\n", slot);
	exit(0);
}
