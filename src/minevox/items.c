#include "minevox.h"

#include <string.h>

/*
 * ===========================================================================
 * Items: the inventory, things lying on the ground, and crafting
 * ===========================================================================
 *
 * An item id is either a block id, which can be placed in the world, or an
 * id at or above ITEM_BASE, which only exists in the inventory. Using one
 * numbering for both means the hotbar, the inventory grid and the recipe
 * table never need to ask which kind of thing they are holding.
 */

/* clang-format off */
const struct item_info item_info[ITEM_COUNT] = {
	/*                                name          stack tool        speed dmg heal colour */
	[ITEM_STICK      - ITEM_BASE] = { "Stick",      64, TOOL_NONE, 16, 0, 0, rgb_to_rgb565(150, 112, 62) },
	[ITEM_COAL       - ITEM_BASE] = { "Coal",       64, TOOL_NONE, 16, 0, 0, rgb_to_rgb565(38, 38, 40) },
	[ITEM_IRON_INGOT - ITEM_BASE] = { "Iron",       64, TOOL_NONE, 16, 0, 0, rgb_to_rgb565(216, 216, 220) },
	[ITEM_GOLD_INGOT - ITEM_BASE] = { "Gold",       64, TOOL_NONE, 16, 0, 0, rgb_to_rgb565(238, 206, 92) },
	[ITEM_PORKCHOP   - ITEM_BASE] = { "Porkchop",   16, TOOL_NONE, 16, 0, 6, rgb_to_rgb565(226, 142, 140) },
	[ITEM_WOOD_PICK  - ITEM_BASE] = { "Wood Pick",   1, TOOL_PICK, 32, 1, 0, rgb_to_rgb565(168, 132, 78) },
	[ITEM_STONE_PICK - ITEM_BASE] = { "Stone Pick",  1, TOOL_PICK, 56, 2, 0, rgb_to_rgb565(146, 146, 148) },
	[ITEM_IRON_PICK  - ITEM_BASE] = { "Iron Pick",   1, TOOL_PICK, 96, 3, 0, rgb_to_rgb565(222, 222, 226) },
	[ITEM_SWORD      - ITEM_BASE] = { "Sword",       1, TOOL_NONE, 16, 5, 0, rgb_to_rgb565(198, 208, 226) },
};

/*
 * Recipes.
 *
 * Two ingredients is enough for everything we make, and it keeps the
 * crafting screen readable on a 160 pixel wide display. The ones that
 * take coal stand in for smelting: there is no furnace, so burning coal
 * at the workbench is how ore becomes metal.
 */
const struct recipe recipes[] = {
	/* out                    in[0]            n   in[1]           n   out  table */
	{ { BLOCK_LOG,        0 },            { 1, 0 }, BLOCK_PLANKS,      4, false },
	{ { BLOCK_PLANKS,     0 },            { 2, 0 }, ITEM_STICK,        4, false },
	{ { BLOCK_PLANKS,     0 },            { 4, 0 }, BLOCK_CRAFTING,    1, false },
	{ { BLOCK_PLANKS,     ITEM_STICK },   { 3, 2 }, ITEM_WOOD_PICK,    1, true  },
	{ { BLOCK_COBBLE,     ITEM_STICK },   { 3, 2 }, ITEM_STONE_PICK,   1, true  },
	{ { ITEM_IRON_INGOT,  ITEM_STICK },   { 3, 2 }, ITEM_IRON_PICK,    1, true  },
	{ { BLOCK_COBBLE,     ITEM_STICK },   { 2, 1 }, ITEM_SWORD,        1, true  },
	{ { BLOCK_IRON_ORE,   ITEM_COAL },    { 1, 1 }, ITEM_IRON_INGOT,   1, true  },
	{ { BLOCK_GOLD_ORE,   ITEM_COAL },    { 1, 1 }, ITEM_GOLD_INGOT,   1, true  },
	{ { BLOCK_SAND,       ITEM_COAL },    { 1, 1 }, BLOCK_GLASS,       1, true  },
	{ { ITEM_IRON_INGOT,  0 },            { 9, 0 }, BLOCK_IRON_BLOCK,  1, false },
	{ { ITEM_GOLD_INGOT,  0 },            { 9, 0 }, BLOCK_GOLD_BLOCK,  1, false },
	{ { ITEM_IRON_INGOT,  ITEM_GOLD_INGOT }, { 6, 3 }, BLOCK_REACTOR,  1, true  },
};
/* clang-format on */

const int num_recipes = (int)(sizeof(recipes) / sizeof(recipes[0]));

struct inv_slot inventory[INVENTORY_SLOTS];

/*
 * Dropped items live in a pool. See mem.h for why we do not use malloc.
 * The live ones are threaded onto a singly linked list so that updating
 * them does not have to walk over empty slots.
 */
#define MAX_DROPS 48

static struct mem_pool drop_pool;
struct drop *drops;

const char *item_name(uint16_t item)
{
	if (item_is_block(item))
		return block_info[item].name;

	if (item >= ITEM_BASE && item < NUM_ITEM_IDS)
		return item_info[item - ITEM_BASE].name;

	return "";
}

uint8_t item_stack_size(uint16_t item)
{
	if (item_is_block(item))
		return 64;

	if (item >= ITEM_BASE && item < NUM_ITEM_IDS)
		return item_info[item - ITEM_BASE].stack;

	return 1;
}

uint8_t item_tool_kind(uint16_t item)
{
	if (item >= ITEM_BASE && item < NUM_ITEM_IDS)
		return item_info[item - ITEM_BASE].tool;

	return TOOL_NONE;
}

void items_init(void)
{
	mem_pool_init(&drop_pool, "drops", sizeof(struct drop), MAX_DROPS);
	drops = NULL;
}

void items_reset(void)
{
	/* Hand every live drop back before forgetting the list head. */
	while (drops) {
		struct drop *next = drops->next;
		mem_pool_free(&drop_pool, drops);
		drops = next;
	}
}

/*
 * ---------------------------------------------------------------------------
 * The inventory
 * ---------------------------------------------------------------------------
 */
void inventory_clear(void)
{
	memset(inventory, 0, sizeof(inventory));
}

bool inventory_add(uint16_t item, int count)
{
	if (!item || count <= 0)
		return false;

	int stack = item_stack_size(item);

	/*
	 * Top up existing stacks first. Filling partial stacks before
	 * taking a fresh slot is what stops an inventory from filling up
	 * with three of this and four of that.
	 */
	for (int i = 0; i < INVENTORY_SLOTS && count > 0; i++) {
		if (inventory[i].item != item)
			continue;

		int room = stack - inventory[i].count;

		if (room <= 0)
			continue;

		int take = count < room ? count : room;
		inventory[i].count += take;
		count -= take;
	}

	for (int i = 0; i < INVENTORY_SLOTS && count > 0; i++) {
		if (inventory[i].item)
			continue;

		int take = count < stack ? count : stack;
		inventory[i].item = item;
		inventory[i].count = take;
		count -= take;
	}

	/* False means we could not fit all of it. */
	return count == 0;
}

bool inventory_take(uint16_t item, int count)
{
	if (inventory_count(item) < count)
		return false;

	for (int i = 0; i < INVENTORY_SLOTS && count > 0; i++) {
		if (inventory[i].item != item)
			continue;

		int take = inventory[i].count < count ? inventory[i].count : count;
		inventory[i].count -= take;
		count -= take;

		if (!inventory[i].count)
			inventory[i].item = 0;
	}

	return true;
}

int inventory_count(uint16_t item)
{
	int total = 0;

	for (int i = 0; i < INVENTORY_SLOTS; i++)
		if (inventory[i].item == item)
			total += inventory[i].count;

	return total;
}

void inventory_drop_all(float x, float y, float z)
{
	for (int i = 0; i < INVENTORY_SLOTS; i++) {
		if (!inventory[i].item)
			continue;

		items_spawn(x, y, z + 0.5f, inventory[i].item, inventory[i].count);
		inventory[i].item = 0;
		inventory[i].count = 0;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Drops
 * ---------------------------------------------------------------------------
 */
void items_spawn(float x, float y, float z, uint16_t item, int count)
{
	if (!item || count <= 0)
		return;

	/*
	 * Merge into a nearby drop of the same kind if we can. Mining a
	 * vein of coal otherwise produces a little pile of separate drops
	 * and drains the pool for no benefit.
	 */
	for (struct drop *d = drops; d; d = d->next) {
		if (d->item != item)
			continue;

		float dx = d->x - x, dy = d->y - y, dz = d->z - z;

		if (dx * dx + dy * dy + dz * dz < 1.0f) {
			d->count += count;
			return;
		}
	}

	struct drop *d = mem_pool_alloc(&drop_pool);

	if (!d) {
		/*
		 * Out of slots. Losing the item is better than stalling the
		 * game, and it only happens if dozens are already waiting.
		 */
		return;
	}

	d->x = x;
	d->y = y;
	d->z = z;
	d->vz = 1.5f;
	d->item = item;
	d->count = count;
	d->age = 0.0f;
	d->delay = 0.4f; /* Grace period, so thrown items are not re-eaten. */

	d->next = drops;
	drops = d;
}

/* True when this position is inside something you cannot fall through. */
static bool solid_here(float x, float y, float z)
{
	return block_is_solid(world_get((int)floorf(x), (int)floorf(y), (int)floorf(z)));
}

void items_update(float dt)
{
	/*
	 * Removing from a singly linked list while walking it needs a
	 * pointer to the link that points at the current node, so that we
	 * can rewrite it. Keeping a pointer to a pointer avoids a separate
	 * "previous" variable and the special case for the head.
	 */
	struct drop **link = &drops;

	while (*link) {
		struct drop *d = *link;

		d->age += dt;

		if (d->delay > 0.0f)
			d->delay -= dt;

		/* Fall until something solid is underfoot. */
		d->vz -= 20.0f * dt;
		float nz = d->z + d->vz * dt;

		if (solid_here(d->x, d->y, nz)) {
			d->vz = 0.0f;
		} else {
			d->z = nz;

			if (d->z < 0.0f)
				d->z = 0.0f;
		}

		/* Drift towards the player once they are close enough. */
		float dx = player.x - d->x;
		float dy = player.y - d->y;
		float dz = (player.z + 0.6f) - d->z;
		float dist2 = dx * dx + dy * dy + dz * dz;

		if (d->delay <= 0.0f && dist2 < 2.2f * 2.2f) {
			float dist = sqrtf(dist2);

			if (dist > 0.001f) {
				float pull = 3.5f * dt / dist;
				d->x += dx * pull;
				d->y += dy * pull;
				d->z += dz * pull;
			}

			if (dist < 0.7f) {
				if (inventory_add(d->item, d->count)) {
					sfx_play("/i:sine (ppp) >e");
					*link = d->next;
					mem_pool_free(&drop_pool, d);
					continue;
				}
			}
		}

		link = &d->next;
	}
}

int items_live_count(void)
{
	return mem_pool_used(&drop_pool);
}

/*
 * ---------------------------------------------------------------------------
 * Crafting
 * ---------------------------------------------------------------------------
 */

/* Is there a workbench within reach? Some recipes insist on one. */
static bool table_nearby(void)
{
	int px = (int)floorf(player.x);
	int py = (int)floorf(player.y);
	int pz = (int)floorf(player.z);

	for (int z = pz - 1; z <= pz + 2; z++)
		for (int y = py - 3; y <= py + 3; y++)
			for (int x = px - 3; x <= px + 3; x++)
				if (world_get(x, y, z) == BLOCK_CRAFTING)
					return true;

	return false;
}

bool recipe_can_make(int index)
{
	if (index < 0 || index >= num_recipes)
		return false;

	const struct recipe *r = &recipes[index];

	if (r->needs_table && !table_nearby())
		return false;

	for (int i = 0; i < CRAFT_INPUTS; i++)
		if (r->in[i] && inventory_count(r->in[i]) < r->in_count[i])
			return false;

	return true;
}

bool recipe_make(int index)
{
	if (!recipe_can_make(index))
		return false;

	const struct recipe *r = &recipes[index];

	for (int i = 0; i < CRAFT_INPUTS; i++)
		if (r->in[i])
			inventory_take(r->in[i], r->in_count[i]);

	if (!inventory_add(r->out, r->out_count)) {
		/* No room: put it on the floor rather than destroying it. */
		items_spawn(player.x, player.y, player.z + 0.5f, r->out, r->out_count);
	}

	sfx_play("/i:square (p) ce");
	return true;
}
