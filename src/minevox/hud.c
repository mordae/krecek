#include "minevox.h"

#include <stdio.h>
#include <string.h>

/*
 * ===========================================================================
 * The heads up display
 * ===========================================================================
 *
 * Everything drawn on top of the world while you are playing. The full
 * screen pages live in menu.c; this file only owns the overlay.
 *
 * The whole screen is 160 by 120 pixels, which is less than a tenth of the
 * area of a phone. Every element here had to earn its space, so the rule
 * was: show it only when it is telling you something. Hearts appear only
 * in survival, breath only under water, the item name only just after it
 * changes.
 */

#define COL_WHITE rgb_to_rgb565(255, 255, 255)
#define COL_GRAY rgb_to_rgb565(170, 170, 170)
#define COL_DARK rgb_to_rgb565(30, 30, 34)
#define COL_HEART rgb_to_rgb565(226, 48, 48)
#define COL_HEART_BG rgb_to_rgb565(58, 26, 26)
#define COL_AIR rgb_to_rgb565(120, 200, 255)

#define HOTBAR_SLOT 14
#define HOTBAR_TOP (SCREEN_H - HOTBAR_SLOT - 2)

/* Drawn twice, offset by one pixel, so text survives any background. */
void hud_shadow_text(int x, int y, color_t color, const char *text)
{
	tft_draw_string(x + 1, y + 1, COL_DARK, "%s", text);
	tft_draw_string(x, y, color, "%s", text);
}

void hud_shadow_center(int x, int y, color_t color, const char *text)
{
	tft_draw_string_center(x + 1, y + 1, COL_DARK, "%s", text);
	tft_draw_string_center(x, y, color, "%s", text);
}

void hud_frame(int x0, int y0, int x1, int y1, color_t color)
{
	tft_draw_rect(x0, y0, x1, y0, color);
	tft_draw_rect(x0, y1, x1, y1, color);
	tft_draw_rect(x0, y0, x0, y1, color);
	tft_draw_rect(x1, y0, x1, y1, color);
}

/* Icons follow the daylight, so nothing glares at you at night. */
uint8_t hud_icon_bright(void)
{
	float light = 1.0f;
	render_sky_colors(NULL, NULL, &light);
	return (uint8_t)clampi((int)(light * 32.0f), 16, 32);
}

/*
 * A seven by six heart, spelled out as a bitmap.
 *
 * Drawing this with rectangles would take more code than the bitmap and
 * would be harder to adjust. One bit per pixel, one row per byte.
 */
static const uint8_t heart_bits[6] = {
	0b0110110, 0b1111111, 0b1111111, 0b0111110, 0b0011100, 0b0001000,
};

static void draw_heart(int x, int y, int fill)
{
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			if (!((heart_bits[row] >> (6 - col)) & 1))
				continue;

			/*
			 * fill is 0, 1 or 2 for empty, half and full. A half
			 * heart is coloured on its left side only.
			 */
			color_t c = COL_HEART_BG;

			if (fill == 2 || (fill == 1 && col < 3))
				c = COL_HEART;

			tft_draw_pixel(x + col, y + row, c);
		}
	}
}

static void draw_health(void)
{
	if (!survival_mode)
		return;

	/* Ten hearts for twenty points, so each one is worth two. */
	for (int i = 0; i < 10; i++) {
		int remaining = player.health - i * 2;
		int fill = remaining >= 2 ? 2 : (remaining == 1 ? 1 : 0);
		draw_heart(2 + i * 8, 2, fill);
	}

	/* Breath, shown only while it matters. */
	if (player.head_in_water || player.air < 11.9f) {
		int bubbles = (int)(player.air / 12.0f * 10.0f + 0.5f);

		for (int i = 0; i < bubbles; i++) {
			int x = 3 + i * 8;
			tft_draw_rect(x, 11, x + 3, 13, COL_AIR);
			tft_draw_rect(x + 1, 10, x + 2, 14, COL_AIR);
		}
	}
}

static void draw_crosshair(void)
{
	int cx = SCREEN_W / 2;
	int cy = SCREEN_H / 2;

	/* Darken underneath first, so it shows up on a pale sky too. */
	render_darken(cx - 4, cy, cx + 4, cy, 8);
	render_darken(cx, cy - 4, cx, cy + 4, 8);
	tft_draw_rect(cx - 3, cy, cx + 3, cy, COL_WHITE);
	tft_draw_rect(cx, cy - 3, cx, cy + 3, COL_WHITE);
}

/* Draw one inventory item, block or not, at a given size. */
void hud_draw_item(int x, int y, int size, uint16_t item, uint8_t bright)
{
	if (!item)
		return;

	if (item_is_block(item)) {
		blocks_draw_icon(x, y, size, item, bright);
		return;
	}

	if (item < ITEM_BASE || item >= NUM_ITEM_IDS)
		return;

	/*
	 * Things that are not blocks get a simple shape rather than a cube:
	 * a bar for ingots and food, a diagonal for tools. It is crude, but
	 * at twelve pixels across anything more would be mush.
	 */
	color_t c = item_info[item - ITEM_BASE].color;
	int inset = size / 4;

	if (item_info[item - ITEM_BASE].tool != TOOL_NONE || item == ITEM_SWORD) {
		for (int i = 0; i < size - 2; i++) {
			tft_draw_pixel(x + 1 + i, y + size - 2 - i, c);
			tft_draw_pixel(x + 2 + i, y + size - 2 - i, color_mul(c, 22));
		}

		/* A stubby handle at the bottom left. */
		tft_draw_rect(x + 1, y + size - 4, x + 3, y + size - 2, rgb_to_rgb565(120, 88, 48));
		return;
	}

	tft_draw_rect(x + inset, y + inset + 1, x + size - inset - 1, y + size - inset - 1, c);
	hud_frame(x + inset, y + inset + 1, x + size - inset - 1, y + size - inset - 1,
		  color_mul(c, 18));
}

static void draw_hotbar(void)
{
	const int slot = HOTBAR_SLOT;
	const int total = slot * HOTBAR_SLOTS;
	int x0 = (SCREEN_W - total) / 2;
	int y0 = HOTBAR_TOP;
	uint8_t bright = hud_icon_bright();

	render_darken(x0 - 1, y0 - 1, x0 + total, y0 + slot, 12);

	for (int i = 0; i < HOTBAR_SLOTS; i++) {
		int x = x0 + i * slot;

		hud_draw_item(x + 1, y0 + 1, slot - 2, inventory[i].item, bright);

		/*
		 * How many are in the stack.
		 *
		 * Only for the slot in hand: the font is sixteen pixels wide
		 * per character and a slot is fourteen, so two digits in
		 * every slot turns the whole bar into mush. The inventory
		 * page spells out the rest.
		 */
		if (i == player.slot && inventory[i].count > 1) {
			tft_draw_string_right(x + slot, y0 + slot - 9, COL_DARK, "%d",
					      inventory[i].count % 100);
			tft_draw_string_right(x + slot - 1, y0 + slot - 10, COL_WHITE, "%d",
					      inventory[i].count % 100);
		}

		hud_frame(x - 1, y0 - 1, x + slot - 1, y0 + slot,
			  i == player.slot ? COL_WHITE : rgb_to_rgb565(90, 90, 96));
	}
}

/* The block or tool in our hand, swung when we use it. */
static void draw_held_item(void)
{
	uint16_t held = player_held_item();

	if (!held)
		return;

	float swing = player.swing > 0.0f ? player.swing : 0.0f;
	int lift = (int)(sinf(swing * (float)M_PI) * 6.0f);

	int size = 28;
	int x = SCREEN_W - size - 3 + lift / 2;
	int y = HOTBAR_TOP - size - 2 + lift;

	render_darken(x - 1, y - 1, x + size, y + size, 20);
	hud_draw_item(x, y, size, held, hud_icon_bright());
}

/*
 * A red edge when something hurts us.
 *
 * A full screen tint would hide the world at exactly the moment you need
 * to see it, so we only stain the border and leave the middle clear.
 */
static void draw_hurt_flash(void)
{
	if (player.hurt_flash <= 0.0f)
		return;

	int strength = (int)(player.hurt_flash * 40.0f);
	strength = clampi(strength, 0, 14);

	for (int i = 0; i < 6; i++) {
		uint8_t amount = (uint8_t)clampi(32 - strength + i * 2, 8, 32);
		int x1 = SCREEN_W - 1 - i;
		int y1 = SCREEN_H - 1 - i;

		render_darken(i, i, x1, i, amount);
		render_darken(i, y1, x1, y1, amount);
		render_darken(i, i, i, y1, amount);
		render_darken(x1, i, x1, y1, amount);
	}
}

static void draw_debug(void)
{
	char line[36];
	int y = survival_mode ? 20 : 2;

	snprintf(line, sizeof(line), "%d %d %d", (int)player.x, (int)player.y, (int)player.z);
	hud_shadow_text(2, y, rgb_to_rgb565(120, 220, 120), line);

	int hour = (int)(world_time / DAY_LENGTH * 24.0f) % 24;
	snprintf(line, sizeof(line), "%02d:00 m%d i%d", hour, mobs_live_count(),
		 items_live_count());
	hud_shadow_text(2, y + 13, rgb_to_rgb565(120, 220, 120), line);

	snprintf(line, sizeof(line), "mem %d/%d e%d", (int)mem_arena_used(), (int)mem_arena_total(),
		 save_edit_count());
	hud_shadow_text(2, y + 26, rgb_to_rgb565(120, 220, 120), line);
}

/* What we are aiming at, named just under the crosshair. */
static void draw_target_name(void)
{
	if (!player.has_target)
		return;

	uint8_t block = world_get(player.tx, player.ty, player.tz);

	if (!block)
		return;

	hud_shadow_center(SCREEN_W / 2, SCREEN_H / 2 + 8, COL_GRAY, block_info[block].name);
}

void hud_paint(float dt)
{
	(void)dt;

	draw_crosshair();
	draw_held_item();
	draw_hotbar();
	draw_health();
	draw_hurt_flash();

	if (opt_debug) {
		draw_debug();
		draw_target_name();
	}
}
