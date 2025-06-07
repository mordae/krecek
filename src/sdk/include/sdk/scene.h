#pragma once
#include "sdk/game.h"
#include <stdbool.h>

typedef enum sdk_event {
	SDK_READ_JOYSTICK, /* Feel free to read joystick */
	SDK_READ_LBRACK,   /* Feel free to read left bracket */
	SDK_READ_RBRACK,   /* Feel free to read right bracket */
	SDK_TICK_NORTH,	   /* Joystick input indicates north */
	SDK_TICK_SOUTH,	   /* Joystick input indicates south */
	SDK_TICK_WEST,	   /* Joystick input indicates west */
	SDK_TICK_EAST,	   /* Joystick input indicates east */
	SDK_INSERTED_HPS,  /* User inserted headphones */
	SDK_REMOVED_HPS,   /* User removed headphones */
	SDK_PRESSED_SELECT,
	SDK_RELEASED_SELECT,
	SDK_PRESSED_START,
	SDK_RELEASED_START,
	SDK_PRESSED_A,
	SDK_RELEASED_A,
	SDK_PRESSED_B,
	SDK_RELEASED_B,
	SDK_PRESSED_X,
	SDK_RELEASED_X,
	SDK_PRESSED_Y,
	SDK_RELEASED_Y,
	SDK_PRESSED_VOL_UP,
	SDK_RELEASED_VOL_UP,
	SDK_PRESSED_VOL_DOWN,
	SDK_RELEASED_VOL_DOWN,
	SDK_PRESSED_VOL_SW,
	SDK_RELEASED_VOL_SW,
	SDK_PRESSED_AUX0,
	SDK_RELEASED_AUX0,
	SDK_PRESSED_AUX1,
	SDK_RELEASED_AUX1,
	SDK_PRESSED_AUX2,
	SDK_RELEASED_AUX2,
	SDK_PRESSED_AUX3,
	SDK_RELEASED_AUX3,
	SDK_PRESSED_AUX4,
	SDK_RELEASED_AUX4,
	SDK_PRESSED_AUX5,
	SDK_RELEASED_AUX5,
	SDK_PRESSED_AUX6,
	SDK_RELEASED_AUX6,
	SDK_PRESSED_AUX7,
	SDK_RELEASED_AUX7,
} sdk_event_t;

/* Interactive sub-scene of a game. */
typedef struct sdk_scene {
	/*
	 * Paint the scene.
	 * Called from the bottom up.
	 * Upper scenes paint over the bottom ones.
	 * Depth indicates how far is the scene from the top (0).
	 */
	void (*paint)(float dt, int depth);

	/*
	 * Handle event.
	 * Called from the top down.
	 * First handler to return true stops the propagation.
	 */
	bool (*handle)(sdk_event_t event, int depth);

	/*
	 * Handle multiplayer message.
	 * Called from the top down.
	 * First handler to return true stops the propagation.
	 */
	bool (*inbox)(sdk_message_t msg, int depth);

	/* Scene was pushed to the stack. */
	void (*pushed)(void);

	/* Scene was popped from the stack. */
	void (*popped)(void);

	/* Scene was obscured by another on top of it. */
	void (*obscured)(void);

	/* Scene was revealed and is now again at the top. */
	void (*revealed)(void);

	/* Parent scene. Scenes form a stack. */
	struct sdk_scene *parent;

	/*
	 * Secondary stack frozen before paints and handling events.
	 * This allows safe stack operations from inside the callbacks.
	 */
	struct sdk_scene *shadow;

	/*
	 * For the shadow stack walking, indicates whether the scene
	 * is part of the stack and should continue to receive events.
	 */
	bool on_stack;
} sdk_scene_t;

/* Stack of visible game scenes. */
extern sdk_scene_t *sdk_scene_stack;

/* Paint the scene stack. */
void sdk_scene_paint(unsigned dt_usec);

/* Deliver input events to the scene stack. */
void sdk_scene_handle(void);

/* Deliver multiplayer message to the scene stack. */
void sdk_scene_inbox(sdk_message_t msg);

/* Push scene to the top of the stack. */
void sdk_scene_push(sdk_scene_t *scene);

/* Pop scene from the top of the stack. */
sdk_scene_t *sdk_scene_pop(void);

/* Pop scene from the top of the stack and push another one. */
sdk_scene_t *sdk_scene_swap(sdk_scene_t *scene);
