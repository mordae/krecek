#include <stddef.h>
#include <sdk.h>

sdk_scene_t *sdk_scene_stack = NULL;

static void scene_paint_r(sdk_scene_t *scene, float dt, int depth)
{
	if (NULL == scene)
		return;

	scene_paint_r(scene->parent, dt, depth + 1);

	if (scene->paint)
		scene->paint(dt, depth);
}

void sdk_scene_paint(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	scene_paint_r(sdk_scene_stack, dt, 0);
}

static bool scene_handle_r(sdk_scene_t *scene, sdk_event_t event)
{
	if (NULL == scene)
		return false;

	if (scene->handle) {
		if (scene->handle(event))
			return true;
	}

	return scene_handle_r(scene->parent, event);
}

void sdk_scene_handle(void)
{
	scene_handle_r(sdk_scene_stack, SDK_READ_JOYSTICK);
	scene_handle_r(sdk_scene_stack, SDK_READ_LBRACK);
	scene_handle_r(sdk_scene_stack, SDK_READ_RBRACK);

	if (sdk_inputs_delta.vertical < 0)
		scene_handle_r(sdk_scene_stack, SDK_TICK_NORTH);

	if (sdk_inputs_delta.vertical > 0)
		scene_handle_r(sdk_scene_stack, SDK_TICK_SOUTH);

	if (sdk_inputs_delta.horizontal < 0)
		scene_handle_r(sdk_scene_stack, SDK_TICK_WEST);

	if (sdk_inputs_delta.horizontal > 0)
		scene_handle_r(sdk_scene_stack, SDK_TICK_EAST);

	if (sdk_inputs_delta.hps > 0)
		scene_handle_r(sdk_scene_stack, SDK_INSERTED_HPS);

	if (sdk_inputs_delta.hps < 0)
		scene_handle_r(sdk_scene_stack, SDK_REMOVED_HPS);

	if (sdk_inputs_delta.select > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_SELECT);

	if (sdk_inputs_delta.select < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_SELECT);

	if (sdk_inputs_delta.start > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_START);

	if (sdk_inputs_delta.start < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_START);

	if (sdk_inputs_delta.a > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_A);

	if (sdk_inputs_delta.a < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_A);

	if (sdk_inputs_delta.b > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_B);

	if (sdk_inputs_delta.b < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_B);

	if (sdk_inputs_delta.x > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_X);

	if (sdk_inputs_delta.x < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_X);

	if (sdk_inputs_delta.y > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_Y);

	if (sdk_inputs_delta.y < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_Y);

	if (sdk_inputs_delta.vol_up > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_VOL_UP);

	if (sdk_inputs_delta.vol_up < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_VOL_UP);

	if (sdk_inputs_delta.vol_down > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_VOL_DOWN);

	if (sdk_inputs_delta.vol_down < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_VOL_DOWN);

	if (sdk_inputs_delta.vol_sw > 0)
		scene_handle_r(sdk_scene_stack, SDK_PRESSED_VOL_SW);

	if (sdk_inputs_delta.vol_sw < 0)
		scene_handle_r(sdk_scene_stack, SDK_RELEASED_VOL_SW);

	for (int i = 0; i < 8; i++) {
		if (sdk_inputs_delta.aux[i] > 0)
			scene_handle_r(sdk_scene_stack, SDK_PRESSED_AUX0 + 2 * i);

		if (sdk_inputs_delta.aux[i] < 0)
			scene_handle_r(sdk_scene_stack, SDK_RELEASED_AUX0 + 2 * i);
	}
}

void sdk_scene_push(sdk_scene_t *scene)
{
	if (NULL == scene)
		return;

	scene->parent = sdk_scene_stack;
	sdk_scene_stack = scene;

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

	if (scene->popped)
		scene->popped();

	return scene;
}

sdk_scene_t *sdk_scene_swap(sdk_scene_t *scene)
{
	sdk_scene_t *popped = sdk_scene_pop();
	sdk_scene_push(scene);
	return popped;
}
