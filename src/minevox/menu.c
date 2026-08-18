#include "minevox.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * ===========================================================================
 * Menus
 * ===========================================================================
 *
 * Every full screen page: the title, the world list, the pause menu, the
 * inventory, the crafting table, the death screen and the controls.
 *
 * TWO WAYS TO DRIVE EVERYTHING
 *
 * The console has a resistive touch panel and the desktop build has a
 * mouse, and menus are far nicer with a pointer than with a stick. But a
 * pointer must never be the only way in: the panel is imprecise, and the
 * game has to stay playable if a finger is busy holding the console. So
 * every page below is fully operable from the buttons, and touch is an
 * alternative rather than a requirement.
 *
 * The pattern that makes this cheap is IMMEDIATE MODE UI: there are no
 * button objects and no callbacks. ui_button() draws a rectangle and, in
 * the same call, returns whether it was just tapped. State lives in the
 * game, not in the interface, which for a screen this small is much less
 * code than the alternative.
 */

#define COL_WHITE rgb_to_rgb565(255, 255, 255)
#define COL_GRAY rgb_to_rgb565(170, 170, 170)
#define COL_DIM rgb_to_rgb565(110, 110, 116)
#define COL_DARK rgb_to_rgb565(30, 30, 34)
#define COL_YELLOW rgb_to_rgb565(255, 220, 90)
#define COL_GREEN rgb_to_rgb565(120, 220, 120)
#define COL_RED rgb_to_rgb565(226, 70, 60)
#define COL_PANEL rgb_to_rgb565(52, 52, 60)

enum game_state game_state = STATE_TITLE;

bool opt_sfx = true;
bool opt_music = false;
bool opt_debug = false;

struct pointer pointer;

static sdk_melody_t *music;
static float toast_timer;
static char toast_text[28];

/* Where the cursor sits on each page. */
static int world_sel;
static int menu_sel;
static int inv_sel;
static int craft_sel;
static int craft_top;
static int pick_sel;
static bool new_survival = true;
static uint32_t new_seed;

static const char *music_tune = "{ /i:sine /bpm:80 (pp) c-e-g-E- g-e-c-g- f-a-C-a- e-g-b-g- }";

void sfx_play(const char *melody)
{
	if (opt_sfx)
		sdk_melody_play(melody);
}

static void music_update(void)
{
	if (opt_music && !music)
		music = sdk_melody_play_get(music_tune);

	if (!opt_music && music) {
		sdk_melody_stop_and_release(music);
		music = NULL;
	}
}

void menu_toast(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(toast_text, sizeof(toast_text), fmt, ap);
	va_end(ap);

	toast_timer = 1.8f;
}

/*
 * ---------------------------------------------------------------------------
 * The pointer
 * ---------------------------------------------------------------------------
 */
void pointer_update(void)
{
	static bool was_down;

	pointer.x = clampi((int)(sdk_inputs.tx * SCREEN_W), 0, SCREEN_W - 1);
	pointer.y = clampi((int)(sdk_inputs.ty * SCREEN_H), 0, SCREEN_H - 1);

	/*
	 * The touch panel reports a pressure rather than a clean up or
	 * down, so we pick a threshold. Too low and the panel triggers on
	 * its own; this value is the one the SDK's own examples use.
	 */
	bool down = sdk_inputs.tp > 0.15f;

	pointer.pressed = down && !was_down;
	pointer.released = !down && was_down;
	pointer.down = down;

	if (pointer.pressed) {
		pointer.press_x = pointer.x;
		pointer.press_y = pointer.y;
	}

	was_down = down;
}

bool ui_hover(int x, int y, int w, int h)
{
	return pointer.x >= x && pointer.x < x + w && pointer.y >= y && pointer.y < y + h;
}

/*
 * A button that draws itself and reports whether it was tapped.
 *
 * The press and the release must both land inside, which is what lets
 * someone slide their finger off to cancel: the same courtesy every
 * touch interface offers.
 */
static bool ui_hit(int x, int y, int w, int h)
{
	if (!pointer.released)
		return false;

	if (pointer.press_x < x || pointer.press_x >= x + w)
		return false;

	if (pointer.press_y < y || pointer.press_y >= y + h)
		return false;

	return ui_hover(x, y, w, h);
}

bool ui_button(int x, int y, int w, int h, const char *label, bool selected)
{
	bool held = pointer.down && ui_hover(x, y, w, h);

	render_darken(x, y, x + w - 1, y + h - 1, held ? 20 : 10);
	hud_frame(x, y, x + w - 1, y + h - 1, selected ? COL_YELLOW : COL_DIM);

	if (label)
		hud_shadow_center(x + w / 2, y + (h - 16) / 2 + 1,
				  selected ? COL_YELLOW : COL_WHITE, label);

	return ui_hit(x, y, w, h);
}

/*
 * ---------------------------------------------------------------------------
 * Shared input helpers
 * ---------------------------------------------------------------------------
 *
 * sdk_inputs_delta.vertical and .horizontal are the SDK's auto-repeating
 * stick ticks: they fire once when you push and then repeat while held,
 * exactly like a key repeat. That is what a menu wants, as opposed to the
 * raw analog value.
 */
static int nav_vertical(void)
{
	return sdk_inputs_delta.vertical;
}

static int nav_horizontal(void)
{
	return sdk_inputs_delta.horizontal;
}

static bool nav_confirm(void)
{
	return sdk_inputs_delta.a > 0;
}

static bool nav_cancel(void)
{
	return sdk_inputs_delta.b > 0 || sdk_inputs_delta.start > 0;
}

/* A hint on the bottom line of a page. */
static void menu_hint(const char *text)
{
	hud_shadow_center(SCREEN_W / 2, SCREEN_H - 16, COL_GRAY, text);
}

void menu_open(enum game_state state)
{
	game_state = state;

	switch (state) {
	case STATE_WORLDS:
		save_scan();
		break;

	case STATE_INVENTORY:
		inv_sel = HOTBAR_SLOTS;
		break;

	case STATE_CRAFT:
		craft_sel = 0;
		craft_top = 0;
		break;

	case STATE_PAUSE:
		menu_sel = 0;
		break;

	case STATE_NEW:
		new_seed = sdk_random();
		new_survival = true;
		break;

	default:
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Title
 * ---------------------------------------------------------------------------
 */
static void paint_title(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 15);

	/* A shelf of blocks instead of a logo we would have to draw. */
	for (int i = 0; i < 5; i++)
		blocks_draw_icon(24 + i * 24, 4, 22, BLOCK_GRASS + i, 32);

	hud_shadow_center(SCREEN_W / 2, 34, COL_WHITE, "M I N E V O X");
	hud_shadow_center(SCREEN_W / 2, 50, COL_GRAY, "survive and build");

	ui_button(30, 70, 100, 18, "PLAY", true);
	hud_shadow_center(SCREEN_W / 2, 94, COL_GRAY, "START: controls");
}

static void input_title(void)
{
	if (nav_confirm() || ui_hit(30, 70, 100, 18)) {
		sfx_play("/i:sine (p) ceg");
		menu_open(STATE_WORLDS);
		return;
	}

	if (sdk_inputs_delta.start > 0)
		menu_open(STATE_HELP);
}

/*
 * ---------------------------------------------------------------------------
 * The world list
 * ---------------------------------------------------------------------------
 *
 * Four slots, because that is what the console gives us, each shown as a
 * card with room for its name, its mode and how long it has been played.
 * See save.c for why a whole world fits in four kilobytes.
 */
#define CARD_TOP 20
#define CARD_H 20
#define CARD_X 6
#define CARD_W (SCREEN_W - 12)

static void paint_worlds(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 9);
	hud_shadow_center(SCREEN_W / 2, 2, COL_YELLOW, "WORLDS");

	for (int i = 0; i < WORLD_SLOTS; i++) {
		const struct world_summary *w = save_summary(i);
		int y = CARD_TOP + i * CARD_H;
		bool selected = i == world_sel;
		bool held = pointer.down && ui_hover(CARD_X, y, CARD_W, CARD_H - 2);

		render_darken(CARD_X, y, CARD_X + CARD_W - 1, y + CARD_H - 3, held ? 22 : 13);
		hud_frame(CARD_X, y, CARD_X + CARD_W - 1, y + CARD_H - 3,
			  selected ? COL_YELLOW : COL_DIM);

		if (!w || !w->used) {
			hud_shadow_text(CARD_X + 6, y + 2, COL_DIM, "- empty -");
			continue;
		}

		hud_shadow_text(CARD_X + 6, y + 2, selected ? COL_YELLOW : COL_WHITE, w->name);

		char info[24];
		snprintf(info, sizeof(info), "%s %dm", w->survival ? "surv" : "crea", w->minutes);
		tft_draw_string_right(CARD_X + CARD_W - 4, y + 2, COL_GRAY, "%s", info);
	}

	const struct world_summary *w = save_summary(world_sel);
	bool used = w && w->used;

	ui_button(6, 102, 74, 16, used ? "PLAY" : "CREATE", false);
	ui_button(84, 102, 70, 16, "DELETE", false);
}

static void input_worlds(void)
{
	int move = nav_vertical();

	if (move > 0)
		world_sel = (world_sel + 1) % WORLD_SLOTS;
	if (move < 0)
		world_sel = (world_sel + WORLD_SLOTS - 1) % WORLD_SLOTS;

	/* Tapping a card selects it. */
	for (int i = 0; i < WORLD_SLOTS; i++)
		if (ui_hit(CARD_X, CARD_TOP + i * CARD_H, CARD_W, CARD_H - 2))
			world_sel = i;

	const struct world_summary *w = save_summary(world_sel);
	bool used = w && w->used;

	if (nav_confirm() || ui_hit(6, 102, 74, 16)) {
		if (used) {
			if (save_load(world_sel)) {
				menu_toast("%s", w->name);
				game_state = STATE_PLAY;
				sfx_play("/i:sine (p) ceg");
			} else {
				menu_toast("Save is damaged");
			}
		} else {
			menu_open(STATE_NEW);
		}

		return;
	}

	if (sdk_inputs_delta.x > 0 || ui_hit(84, 102, 70, 16)) {
		if (used) {
			save_erase(world_sel);
			menu_toast("Deleted");
		}

		return;
	}

	if (nav_cancel())
		menu_open(STATE_TITLE);
}

/*
 * ---------------------------------------------------------------------------
 * Creating a world
 * ---------------------------------------------------------------------------
 */
static void paint_new(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 9);
	hud_shadow_center(SCREEN_W / 2, 2, COL_YELLOW, "NEW WORLD");

	char line[28];
	snprintf(line, sizeof(line), "Slot %d", world_sel + 1);
	hud_shadow_center(SCREEN_W / 2, 22, COL_WHITE, line);

	ui_button(10, 40, 140, 18, new_survival ? "MODE: SURVIVAL" : "MODE: CREATIVE", false);

	snprintf(line, sizeof(line), "SEED %lu", (unsigned long)(new_seed % 100000u));
	ui_button(10, 62, 140, 18, line, false);

	ui_button(10, 86, 66, 18, "CREATE", true);
	ui_button(84, 86, 66, 18, "BACK", false);
}

static void input_new(void)
{
	if (sdk_inputs_delta.x > 0 || ui_hit(10, 40, 140, 18))
		new_survival = !new_survival;

	if (ui_hit(10, 62, 140, 18) || nav_horizontal())
		new_seed = sdk_random();

	if (nav_confirm() || ui_hit(10, 86, 66, 18)) {
		char name[WORLD_NAME_LEN + 1];

		/* The modulo is only there to promise the compiler one digit. */
		snprintf(name, sizeof(name), "World %d", (world_sel % 9) + 1);

		save_new(world_sel, name, new_seed, new_survival);
		game_state = STATE_PLAY;
		menu_toast("%s", new_survival ? "Survival" : "Creative");
		sfx_play("/i:sine (p) ceg");
		return;
	}

	if (nav_cancel() || ui_hit(84, 86, 66, 18))
		menu_open(STATE_WORLDS);
}

/*
 * ---------------------------------------------------------------------------
 * Pause
 * ---------------------------------------------------------------------------
 */
/*
 * The pause list.
 *
 * Eight items is the most that fits: the font is sixteen pixels tall and
 * we have about a hundred to play with. That is why sound and music share
 * one setting that steps off, effects, then everything, rather than each
 * taking a line of its own.
 */
enum menu_item {
	MENU_RESUME = 0,
	MENU_INVENTORY,
	MENU_CRAFT,
	MENU_SAVE,
	MENU_DETAIL,
	MENU_AUDIO,
	MENU_INFO,
	MENU_QUIT,
	NUM_MENU_ITEMS,
};

static void menu_label(int index, char *out, size_t size)
{
	switch (index) {
	case MENU_RESUME:
		snprintf(out, size, "Resume");
		break;
	case MENU_INVENTORY:
		snprintf(out, size, "Inventory");
		break;
	case MENU_CRAFT:
		snprintf(out, size, "Crafting");
		break;
	case MENU_SAVE:
		snprintf(out, size, "Save world");
		break;
	case MENU_DETAIL:
		snprintf(out, size, "Detail   %s",
			 render_scale == 1 ? "full" : (render_scale == 2 ? "half" : "low"));
		break;
	case MENU_AUDIO:
		snprintf(out, size, "Audio    %s", !opt_sfx ? "off" : (opt_music ? "all" : "fx"));
		break;
	case MENU_INFO:
		snprintf(out, size, "Info     %s", opt_debug ? "on" : "off");
		break;
	default:
		snprintf(out, size, "Save & quit");
		break;
	}
}

#define PAUSE_TOP 14
#define PAUSE_STEP 12

static void paint_pause(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 10);
	hud_shadow_center(SCREEN_W / 2, 1, COL_YELLOW, "PAUSED");

	for (int i = 0; i < NUM_MENU_ITEMS; i++) {
		char line[28];
		menu_label(i, line, sizeof(line));

		int y = PAUSE_TOP + i * PAUSE_STEP;
		bool selected = i == menu_sel;

		if (pointer.down && ui_hover(6, y, SCREEN_W - 12, PAUSE_STEP))
			render_darken(6, y, SCREEN_W - 7, y + PAUSE_STEP - 1, 22);

		hud_shadow_text(18, y, selected ? COL_YELLOW : COL_WHITE, line);

		if (selected)
			hud_shadow_text(8, y, COL_YELLOW, ">");
	}
}

static void pause_activate(int item)
{
	switch (item) {
	case MENU_RESUME:
		game_state = STATE_PLAY;
		break;

	case MENU_INVENTORY:
		menu_open(survival_mode ? STATE_INVENTORY : STATE_PICKER);
		break;

	case MENU_CRAFT:
		menu_open(STATE_CRAFT);
		break;

	case MENU_SAVE:
		save_store(current_world);
		game_state = STATE_PLAY;
		menu_toast("Saved");
		break;

	case MENU_DETAIL:
		render_scale = render_scale % 3 + 1;
		break;

	case MENU_AUDIO:
		/* Step around the three states: off, effects, effects plus music. */
		if (!opt_sfx) {
			opt_sfx = true;
			opt_music = false;
		} else if (!opt_music) {
			opt_music = true;
		} else {
			opt_sfx = false;
			opt_music = false;
		}

		music_update();
		break;

	case MENU_INFO:
		opt_debug = !opt_debug;
		break;

	default:
		save_store(current_world);
		menu_toast("Saved");
		menu_open(STATE_TITLE);
		break;
	}
}

static void input_pause(void)
{
	int move = nav_vertical();

	if (move > 0)
		menu_sel = (menu_sel + 1) % NUM_MENU_ITEMS;
	if (move < 0)
		menu_sel = (menu_sel + NUM_MENU_ITEMS - 1) % NUM_MENU_ITEMS;

	for (int i = 0; i < NUM_MENU_ITEMS; i++) {
		if (ui_hit(6, PAUSE_TOP + i * PAUSE_STEP, SCREEN_W - 12, PAUSE_STEP)) {
			menu_sel = i;
			pause_activate(i);
			return;
		}
	}

	if (nav_confirm() || sdk_inputs_delta.x > 0) {
		pause_activate(menu_sel);
		return;
	}

	if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.start > 0)
		game_state = STATE_PLAY;
}

/*
 * ---------------------------------------------------------------------------
 * Inventory
 * ---------------------------------------------------------------------------
 *
 * Nine columns by four rows. The top row is the hotbar, the rest is
 * storage, laid out the same way as the real game.
 *
 * Dragging items around is miserable on a resistive panel, so instead
 * every slot swaps with the hotbar slot you have selected: one tap to move
 * something where you want it, no dragging at all.
 */
#define INV_CELL 16
#define INV_X ((SCREEN_W - INV_CELL * HOTBAR_SLOTS) / 2)
#define INV_Y 24

static void inv_cell_pos(int index, int *x, int *y)
{
	int col = index % HOTBAR_SLOTS;
	int row = index / HOTBAR_SLOTS;

	*x = INV_X + col * INV_CELL;
	*y = INV_Y + row * INV_CELL + (row ? 6 : 0); /* Gap under the hotbar. */
}

static void paint_inventory(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 9);
	hud_shadow_center(SCREEN_W / 2, 1, COL_YELLOW, "INVENTORY  B:close");

	uint8_t bright = hud_icon_bright();

	for (int i = 0; i < INVENTORY_SLOTS; i++) {
		int x, y;
		inv_cell_pos(i, &x, &y);

		render_darken(x, y, x + INV_CELL - 2, y + INV_CELL - 2, 14);
		hud_draw_item(x + 2, y + 2, INV_CELL - 5, inventory[i].item, bright);

		color_t edge = COL_DIM;

		if (i == inv_sel)
			edge = COL_YELLOW;
		else if (i == player.slot)
			edge = COL_WHITE;

		hud_frame(x, y, x + INV_CELL - 2, y + INV_CELL - 2, edge);
	}

	/*
	 * The selected slot is spelled out here rather than as a number in
	 * every cell. At sixteen pixels a cell there is simply no room for
	 * a two digit count that does not collide with its neighbours.
	 */
	char line[32];

	if (inventory[inv_sel].item)
		snprintf(line, sizeof(line), "%s x%d", item_name(inventory[inv_sel].item),
			 inventory[inv_sel].count);
	else
		snprintf(line, sizeof(line), "empty");

	hud_shadow_center(SCREEN_W / 2, 92, COL_WHITE, line);
	menu_hint("A: move   X: drop");
}

static void input_inventory(void)
{
	int move = nav_vertical();
	int side = nav_horizontal();

	if (side > 0)
		inv_sel++;
	if (side < 0)
		inv_sel--;
	if (move > 0)
		inv_sel += HOTBAR_SLOTS;
	if (move < 0)
		inv_sel -= HOTBAR_SLOTS;

	inv_sel = clampi(inv_sel, 0, INVENTORY_SLOTS - 1);

	for (int i = 0; i < INVENTORY_SLOTS; i++) {
		int x, y;
		inv_cell_pos(i, &x, &y);

		if (ui_hit(x, y, INV_CELL - 1, INV_CELL - 1)) {
			inv_sel = i;

			/* Tapping the hotbar picks that slot instead of swapping. */
			if (i < HOTBAR_SLOTS) {
				player.slot = i;
			} else {
				struct inv_slot tmp = inventory[i];
				inventory[i] = inventory[player.slot];
				inventory[player.slot] = tmp;
			}

			return;
		}
	}

	if (nav_confirm()) {
		if (inv_sel < HOTBAR_SLOTS) {
			player.slot = inv_sel;
		} else {
			struct inv_slot tmp = inventory[inv_sel];
			inventory[inv_sel] = inventory[player.slot];
			inventory[player.slot] = tmp;
		}

		sfx_play("/i:sine (ppp) c");
		return;
	}

	if (sdk_inputs_delta.x > 0) {
		/* Throw the selected stack on the floor. */
		if (inventory[inv_sel].item) {
			items_spawn(player.x, player.y, player.z + 1.0f, inventory[inv_sel].item,
				    inventory[inv_sel].count);
			inventory[inv_sel].item = 0;
			inventory[inv_sel].count = 0;
		}

		return;
	}

	if (nav_cancel())
		game_state = STATE_PLAY;
}

/*
 * ---------------------------------------------------------------------------
 * Crafting
 * ---------------------------------------------------------------------------
 */
#define CRAFT_ROWS 4
#define CRAFT_ROW_H 15
#define CRAFT_TOP 20

static void paint_craft(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 9);
	hud_shadow_center(SCREEN_W / 2, 1, COL_YELLOW, "CRAFTING  B:close");

	uint8_t bright = hud_icon_bright();

	for (int row = 0; row < CRAFT_ROWS; row++) {
		int index = craft_top + row;

		if (index >= num_recipes)
			break;

		const struct recipe *r = &recipes[index];
		int y = CRAFT_TOP + row * CRAFT_ROW_H;
		bool selected = index == craft_sel;
		bool can = recipe_can_make(index);

		if (pointer.down && ui_hover(4, y, SCREEN_W - 8, CRAFT_ROW_H - 1))
			render_darken(4, y, SCREEN_W - 5, y + CRAFT_ROW_H - 2, 22);

		hud_frame(4, y, SCREEN_W - 5, y + CRAFT_ROW_H - 2,
			  selected ? COL_YELLOW : COL_DARK);

		/* What it makes, on the left. */
		hud_draw_item(6, y + 1, 12, r->out, bright);

		char line[28];

		if (r->out_count > 1)
			snprintf(line, sizeof(line), "%dx %s", r->out_count, item_name(r->out));
		else
			snprintf(line, sizeof(line), "%s", item_name(r->out));

		hud_shadow_text(21, y, can ? (selected ? COL_YELLOW : COL_WHITE) : COL_DIM, line);
	}

	/* What the highlighted recipe costs. */
	const struct recipe *r = &recipes[craft_sel];
	char cost[32] = "";

	for (int i = 0; i < CRAFT_INPUTS; i++) {
		if (!r->in[i])
			continue;

		char part[18];
		snprintf(part, sizeof(part), "%s%d %s", i && cost[0] ? " + " : "", r->in_count[i],
			 item_name(r->in[i]));
		strncat(cost, part, sizeof(cost) - strlen(cost) - 1);
	}

	hud_shadow_center(SCREEN_W / 2, 84, recipe_can_make(craft_sel) ? COL_GREEN : COL_RED, cost);
	menu_hint(r->needs_table ? "needs a workbench" : "A: craft");
}

static void input_craft(void)
{
	int move = nav_vertical();

	if (move > 0)
		craft_sel++;
	if (move < 0)
		craft_sel--;

	craft_sel = clampi(craft_sel, 0, num_recipes - 1);

	/* Keep the highlighted row on screen. */
	if (craft_sel < craft_top)
		craft_top = craft_sel;
	if (craft_sel >= craft_top + CRAFT_ROWS)
		craft_top = craft_sel - CRAFT_ROWS + 1;

	for (int row = 0; row < CRAFT_ROWS; row++) {
		int index = craft_top + row;

		if (index >= num_recipes)
			break;

		if (ui_hit(4, CRAFT_TOP + row * CRAFT_ROW_H, SCREEN_W - 8, CRAFT_ROW_H - 1)) {
			if (craft_sel == index) {
				if (!recipe_make(index))
					menu_toast("Missing materials");
			}

			craft_sel = index;
			return;
		}
	}

	if (nav_confirm()) {
		if (!recipe_make(craft_sel))
			menu_toast("Missing materials");

		return;
	}

	if (nav_cancel())
		game_state = STATE_PLAY;
}

/*
 * ---------------------------------------------------------------------------
 * Creative block picker
 * ---------------------------------------------------------------------------
 */
#define PICK_COLS 6
#define PICK_CELL 22
#define PICK_X ((SCREEN_W - PICK_COLS * PICK_CELL) / 2)
#define PICK_Y 18

static void paint_picker(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 10);
	hud_shadow_center(SCREEN_W / 2, 1, COL_YELLOW, "PICK A BLOCK");

	for (int i = 0; i < NUM_BLOCKS - 1; i++) {
		int x = PICK_X + (i % PICK_COLS) * PICK_CELL;
		int y = PICK_Y + (i / PICK_COLS) * PICK_CELL;

		blocks_draw_icon(x + 2, y + 2, PICK_CELL - 5, i + 1, 32);

		if (i == pick_sel)
			hud_frame(x, y, x + PICK_CELL - 2, y + PICK_CELL - 2, COL_WHITE);
	}

	hud_shadow_center(SCREEN_W / 2, 92, COL_WHITE, block_info[pick_sel + 1].name);
	menu_hint("A: take   B: back");
}

static void input_picker(void)
{
	int last = NUM_BLOCKS - 2;
	int move = nav_vertical();
	int side = nav_horizontal();

	if (side > 0)
		pick_sel++;
	if (side < 0)
		pick_sel--;
	if (move > 0)
		pick_sel += PICK_COLS;
	if (move < 0)
		pick_sel -= PICK_COLS;

	pick_sel = clampi(pick_sel, 0, last);

	for (int i = 0; i <= last; i++) {
		int x = PICK_X + (i % PICK_COLS) * PICK_CELL;
		int y = PICK_Y + (i / PICK_COLS) * PICK_CELL;

		if (ui_hit(x, y, PICK_CELL - 1, PICK_CELL - 1)) {
			pick_sel = i;
			inventory[player.slot].item = i + 1;
			inventory[player.slot].count = 64;
			game_state = STATE_PLAY;
			return;
		}
	}

	if (nav_confirm() || sdk_inputs_delta.x > 0) {
		inventory[player.slot].item = pick_sel + 1;
		inventory[player.slot].count = 64;
		game_state = STATE_PLAY;
		return;
	}

	if (nav_cancel())
		game_state = STATE_PLAY;
}

/*
 * ---------------------------------------------------------------------------
 * Death and help
 * ---------------------------------------------------------------------------
 */
static void paint_dead(void)
{
	/* Stain the whole screen red: this one deserves to be dramatic. */
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 8);

	hud_shadow_center(SCREEN_W / 2, 38, COL_RED, "YOU DIED");
	hud_shadow_center(SCREEN_W / 2, 58, COL_GRAY, "items dropped here");

	ui_button(38, 78, 84, 18, "RESPAWN", true);
}

static void input_dead(void)
{
	if (nav_confirm() || ui_hit(38, 78, 84, 18)) {
		game_state = STATE_PLAY;
		sfx_play("/i:sine (p) ceg");
	}
}

static void paint_help(void)
{
	render_darken(0, 0, SCREEN_W - 1, SCREEN_H - 1, 6);
	hud_shadow_center(SCREEN_W / 2, 1, COL_YELLOW, "CONTROLS (B: back)");

	static const char *const lines[] = {
		"stick  walk & turn", "L / R  look up/down",	  "A      jump",
		"B      mine / hit",  "X      use / place / eat", "Y      next slot",
		"hold Y inventory",
	};

	for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++)
		hud_shadow_text(4, 20 + i * 14, COL_WHITE, lines[i]);
}

static void input_help(void)
{
	if (nav_confirm() || nav_cancel() || pointer.released)
		menu_open(STATE_TITLE);
}

/*
 * ---------------------------------------------------------------------------
 * Dispatch
 * ---------------------------------------------------------------------------
 */
void menu_paint(float dt)
{
	switch (game_state) {
	case STATE_TITLE:
		paint_title();
		break;
	case STATE_WORLDS:
		paint_worlds();
		break;
	case STATE_NEW:
		paint_new();
		break;
	case STATE_PAUSE:
		paint_pause();
		break;
	case STATE_INVENTORY:
		paint_inventory();
		break;
	case STATE_CRAFT:
		paint_craft();
		break;
	case STATE_PICKER:
		paint_picker();
		break;
	case STATE_DEAD:
		paint_dead();
		break;
	case STATE_HELP:
		paint_help();
		break;
	default:
		break;
	}

	/*
	 * The toast floats above every page, including the game itself. It
	 * sits on the bottom line in menus and just above the hotbar while
	 * playing, with a dark bar behind it so it reads over anything.
	 */
	if (toast_timer > 0.0f) {
		toast_timer -= dt;

		/*
		 * While playing it sits just above the hotbar. On a menu it
		 * goes to the top, over the page title: the title is static
		 * and the toast is news, so that is the right thing to hide.
		 */
		int y = game_state == STATE_PLAY ? SCREEN_H - 34 : 1;

		render_darken(0, y, SCREEN_W - 1, y + 15, 7);
		hud_shadow_center(SCREEN_W / 2, y, COL_WHITE, toast_text);
	}

	/*
	 * Draw the pointer itself. The touch panel has no cursor of its
	 * own, and without this it is impossible to tell whether the game
	 * noticed your finger.
	 */
	if (pointer.down && game_state != STATE_PLAY) {
		tft_draw_rect(pointer.x - 3, pointer.y, pointer.x + 3, pointer.y, COL_YELLOW);
		tft_draw_rect(pointer.x, pointer.y - 3, pointer.x, pointer.y + 3, COL_YELLOW);
	}
}

void menu_input(float dt)
{
	(void)dt;

	switch (game_state) {
	case STATE_TITLE:
		/* Drift the camera, so the title has something to show. */
		player.yaw += dt * 0.12f;
		input_title();
		break;
	case STATE_WORLDS:
		input_worlds();
		break;
	case STATE_NEW:
		input_new();
		break;
	case STATE_PAUSE:
		input_pause();
		break;
	case STATE_INVENTORY:
		input_inventory();
		break;
	case STATE_CRAFT:
		input_craft();
		break;
	case STATE_PICKER:
		input_picker();
		break;
	case STATE_DEAD:
		input_dead();
		break;
	case STATE_HELP:
		input_help();
		break;
	default:
		break;
	}
}
