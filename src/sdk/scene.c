#include <stddef.h>
#include <sdk.h>

sdk_scene_t *sdk_scene_stack = NULL;

static sdk_scene_t *prepare_shadow_stack(void)
{
	sdk_scene_t *root = sdk_scene_stack;

	for (sdk_scene_t *iter = root; iter; iter = iter->parent) {
		iter->shadow = iter->parent;
		iter->on_stack = true;
	}

	return root;
}

static void scene_paint_r(sdk_scene_t *scene, float dt, int depth)
{
	if (NULL == scene)
		return;

	scene_paint_r(scene->shadow, dt, depth + 1);

	if (scene->on_stack && scene->paint)
		scene->paint(dt, depth);
}

void sdk_scene_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	sdk_scene_t *root = prepare_shadow_stack();
	scene_paint_r(root, dt, 0);
}

static bool scene_handle_r(sdk_scene_t *scene, sdk_event_t event, int depth)
{
	if (NULL == scene)
		return false;

	if (scene->handle && scene->on_stack) {
		if (scene->handle(event, depth))
			return true;
	}

	return scene_handle_r(scene->shadow, event, depth + 1);
}

void sdk_scene_handle(void)
{
	sdk_scene_t *root = prepare_shadow_stack();

	scene_handle_r(root, SDK_READ_JOYSTICK, 0);
	scene_handle_r(root, SDK_READ_LBRACK, 0);
	scene_handle_r(root, SDK_READ_RBRACK, 0);

	if (sdk_inputs_delta.vertical < 0)
		scene_handle_r(root, SDK_TICK_NORTH, 0);

	if (sdk_inputs_delta.vertical > 0)
		scene_handle_r(root, SDK_TICK_SOUTH, 0);

	if (sdk_inputs_delta.horizontal < 0)
		scene_handle_r(root, SDK_TICK_WEST, 0);

	if (sdk_inputs_delta.horizontal > 0)
		scene_handle_r(root, SDK_TICK_EAST, 0);

	if (sdk_inputs_delta.hps > 0)
		scene_handle_r(root, SDK_INSERTED_HPS, 0);

	if (sdk_inputs_delta.hps < 0)
		scene_handle_r(root, SDK_REMOVED_HPS, 0);

	if (sdk_inputs_delta.select > 0)
		scene_handle_r(root, SDK_PRESSED_SELECT, 0);

	if (sdk_inputs_delta.select < 0)
		scene_handle_r(root, SDK_RELEASED_SELECT, 0);

	if (sdk_inputs_delta.start > 0)
		scene_handle_r(root, SDK_PRESSED_START, 0);

	if (sdk_inputs_delta.start < 0)
		scene_handle_r(root, SDK_RELEASED_START, 0);

	if (sdk_inputs_delta.a > 0)
		scene_handle_r(root, SDK_PRESSED_A, 0);

	if (sdk_inputs_delta.a < 0)
		scene_handle_r(root, SDK_RELEASED_A, 0);

	if (sdk_inputs_delta.b > 0)
		scene_handle_r(root, SDK_PRESSED_B, 0);

	if (sdk_inputs_delta.b < 0)
		scene_handle_r(root, SDK_RELEASED_B, 0);

	if (sdk_inputs_delta.x > 0)
		scene_handle_r(root, SDK_PRESSED_X, 0);

	if (sdk_inputs_delta.x < 0)
		scene_handle_r(root, SDK_RELEASED_X, 0);

	if (sdk_inputs_delta.y > 0)
		scene_handle_r(root, SDK_PRESSED_Y, 0);

	if (sdk_inputs_delta.y < 0)
		scene_handle_r(root, SDK_RELEASED_Y, 0);

	if (sdk_inputs_delta.vol_up > 0)
		scene_handle_r(root, SDK_PRESSED_VOL_UP, 0);

	if (sdk_inputs_delta.vol_up < 0)
		scene_handle_r(root, SDK_RELEASED_VOL_UP, 0);

	if (sdk_inputs_delta.vol_down > 0)
		scene_handle_r(root, SDK_PRESSED_VOL_DOWN, 0);

	if (sdk_inputs_delta.vol_down < 0)
		scene_handle_r(root, SDK_RELEASED_VOL_DOWN, 0);

	if (sdk_inputs_delta.vol_sw > 0)
		scene_handle_r(root, SDK_PRESSED_VOL_SW, 0);

	if (sdk_inputs_delta.vol_sw < 0)
		scene_handle_r(root, SDK_RELEASED_VOL_SW, 0);

	for (int i = 0; i < 8; i++) {
		if (sdk_inputs_delta.aux[i] > 0)
			scene_handle_r(root, SDK_PRESSED_AUX0 + 2 * i, 0);

		if (sdk_inputs_delta.aux[i] < 0)
			scene_handle_r(root, SDK_RELEASED_AUX0 + 2 * i, 0);
	}
}

static bool scene_inbox_r(sdk_scene_t *scene, sdk_message_t msg, int depth)
{
	if (NULL == scene)
		return false;

	if (scene->inbox && scene->on_stack) {
		if (scene->inbox(msg, depth))
			return true;
	}

	return scene_inbox_r(scene->shadow, msg, depth + 1);
}

void sdk_scene_inbox(sdk_message_t msg)
{
	sdk_scene_t *root = prepare_shadow_stack();
	scene_inbox_r(root, msg, 0);
}

void sdk_scene_push(sdk_scene_t *scene)
{
	if (NULL == scene)
		return;

	scene->parent = sdk_scene_stack;
	sdk_scene_stack = scene;

	if (scene->parent && scene->parent->obscured)
		scene->parent->obscured();

	if (scene->pushed)
		scene->pushed();
}

sdk_scene_t *sdk_scene_pop(void)
{
	if (NULL == sdk_scene_stack)
		return NULL;

	sdk_scene_t *scene = sdk_scene_stack;
	sdk_scene_stack = scene->parent;
	scene->parent = NULL;

	scene->on_stack = false;

	if (scene->popped)
		scene->popped();

	if (sdk_scene_stack && sdk_scene_stack->revealed)
		sdk_scene_stack->revealed();

	return scene;
}

sdk_scene_t *sdk_scene_swap(sdk_scene_t *scene)
{
	sdk_scene_t *popped = sdk_scene_pop();
	sdk_scene_push(scene);
	return popped;
}
