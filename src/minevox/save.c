#include "minevox.h"

#include <stdio.h>
#include <string.h>

/*
 * ===========================================================================
 * Saved worlds
 * ===========================================================================
 *
 * THE PROBLEM
 *
 * The console gives each game four save slots of 4 KiB. Our world is
 * 75 KiB. Even the best general purpose compressor is not going to fit
 * eighteen times its own size, so storing the blocks is off the table.
 *
 * THE TRICK
 *
 * The world is not really data, it is the output of a function. Terrain
 * generation is completely deterministic: the same seed always produces
 * the same island, the same caves, the same trees. So we do not have to
 * store the world at all. We store:
 *
 *     seed  +  the list of blocks the player changed
 *
 * and rebuild the rest on load by running the generator again and then
 * replaying the changes on top. That costs about a fifth of a second of
 * computation and turns 75 KiB into a few hundred bytes for a world that
 * has been lightly built in.
 *
 * This is the same idea as an event log or a journal in a database, and
 * it is worth remembering: when data is expensive to store but cheap to
 * recompute, store the recipe instead of the result.
 *
 * THE LIMIT
 *
 * The obvious catch is that the edit list is finite. We keep room for
 * 1024 changes; once a builder passes that, the oldest edits would have
 * to go. Rather than silently losing someone's house we stop recording
 * and tell them, which is at least honest.
 */

#define SAVE_MAGIC "MVX2"

/* Edits we can hold in memory, and therefore also on disk. */
#define MAX_EDITS 1024

/*
 * One edit squeezed into three bytes.
 *
 *   bits  0..5   x            (0..47, needs 6)
 *   bits  6..11  y            (0..47, needs 6)
 *   bits 12..17  z            (0..39, needs 6)
 *   bits 18..22  block id     (0..24, needs 5)
 *
 * Twenty three bits, so three bytes each. Going to four bytes would cost
 * us a third of the world's edit capacity, which is why it is worth the
 * small amount of shifting.
 */
static_assert(WORLD_X <= 64 && WORLD_Y <= 64 && WORLD_Z <= 64, "coordinates need 6 bits each");
static_assert(NUM_BLOCKS <= 32, "block ids need 5 bits");

static uint32_t edits[MAX_EDITS];
static int num_edits;
static bool edits_full_warned;

int current_world = 0;
bool survival_mode = true;
float world_play_time = 0.0f;

static struct world_summary summaries[WORLD_SLOTS];

static inline uint32_t edit_pack(int x, int y, int z, uint8_t block)
{
	return (uint32_t)x | ((uint32_t)y << 6) | ((uint32_t)z << 12) | ((uint32_t)block << 18);
}

/*
 * What actually lands in a save slot.
 *
 * Written and read with memcpy into a byte buffer rather than by pointing
 * a struct at the buffer, because the buffer has no alignment guarantees
 * and the Cortex-M0+ faults on unaligned word access.
 */
struct save_header {
	char magic[4];
	uint32_t seed;
	char name[WORLD_NAME_LEN + 1];
	uint8_t survival;
	uint8_t flying;
	uint8_t hotbar_slot;
	uint8_t health;
	float x, y, z;
	float yaw, pitch;
	float time_of_day;
	float play_time;
	float respawn_x, respawn_y, respawn_z;
	uint16_t num_edits;
	struct inv_slot inventory[INVENTORY_SLOTS];
};

/* One buffer, shared by loading and saving. 4 KiB is worth not doubling. */
static uint8_t buffer[SDK_SAVE_SLOT_SIZE];

/* How many edits fit in a slot once the header has taken its share. */
static int edit_capacity(void)
{
	int room = (SDK_SAVE_SLOT_SIZE - (int)sizeof(struct save_header)) / 3;
	return room < MAX_EDITS ? room : MAX_EDITS;
}

void save_note_edit(int x, int y, int z, uint8_t block)
{
	uint32_t packed = edit_pack(x, y, z, block);

	/*
	 * If this block was changed before, overwrite that entry instead of
	 * appending. Otherwise digging and refilling the same hole would
	 * eat the budget. The scan is linear, but it only runs when a
	 * player places or breaks something, which is rare next to the
	 * thousands of rays we cast every frame.
	 */
	const uint32_t position_mask = 0x3ffff; /* The low 18 bits: x, y, z. */

	for (int i = 0; i < num_edits; i++) {
		if ((edits[i] & position_mask) == (packed & position_mask)) {
			edits[i] = packed;
			return;
		}
	}

	if (num_edits < edit_capacity()) {
		edits[num_edits++] = packed;
		return;
	}

	if (!edits_full_warned) {
		edits_full_warned = true;
		menu_toast("World memory full");
	}
}

int save_edit_count(void)
{
	return num_edits;
}

void save_forget_edits(void)
{
	num_edits = 0;
	edits_full_warned = false;
}

/* Read a slot's header, returning false when the slot is empty. */
static bool read_header(int slot, struct save_header *hdr)
{
	if (slot < 0 || slot >= WORLD_SLOTS)
		return false;

	sdk_save_read(slot, buffer, sizeof(*hdr));
	memcpy(hdr, buffer, sizeof(*hdr));

	if (memcmp(hdr->magic, SAVE_MAGIC, 4))
		return false;

	if (hdr->num_edits > edit_capacity())
		return false;

	/* Never trust a name from storage to be terminated. */
	hdr->name[WORLD_NAME_LEN] = 0;
	return true;
}

void save_scan(void)
{
	for (int i = 0; i < WORLD_SLOTS; i++) {
		struct save_header hdr;

		memset(&summaries[i], 0, sizeof(summaries[i]));

		if (!read_header(i, &hdr))
			continue;

		summaries[i].used = true;
		memcpy(summaries[i].name, hdr.name, WORLD_NAME_LEN + 1);
		summaries[i].seed = hdr.seed;
		summaries[i].edits = hdr.num_edits;
		summaries[i].minutes = (uint16_t)(hdr.play_time / 60.0f);
		summaries[i].health = hdr.health;
		summaries[i].survival = hdr.survival;
	}
}

const struct world_summary *save_summary(int slot)
{
	if (slot < 0 || slot >= WORLD_SLOTS)
		return NULL;

	return &summaries[slot];
}

void save_store(int slot)
{
	if (slot < 0 || slot >= WORLD_SLOTS)
		return;

	struct save_header hdr;
	memset(&hdr, 0, sizeof(hdr));

	memcpy(hdr.magic, SAVE_MAGIC, 4);
	hdr.seed = world_seed;

	/* Keep whatever name the world already had. */
	if (summaries[slot].used)
		memcpy(hdr.name, summaries[slot].name, WORLD_NAME_LEN + 1);
	else
		snprintf(hdr.name, sizeof(hdr.name), "World %d", slot + 1);

	hdr.survival = survival_mode;
	hdr.flying = player.flying;
	hdr.hotbar_slot = player.slot;
	hdr.health = player.health;
	hdr.x = player.x;
	hdr.y = player.y;
	hdr.z = player.z;
	hdr.yaw = player.yaw;
	hdr.pitch = player.pitch;
	hdr.time_of_day = world_time;
	hdr.play_time = world_play_time;
	hdr.respawn_x = player.respawn_x;
	hdr.respawn_y = player.respawn_y;
	hdr.respawn_z = player.respawn_z;
	hdr.num_edits = num_edits;
	memcpy(hdr.inventory, inventory, sizeof(hdr.inventory));

	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, &hdr, sizeof(hdr));

	uint8_t *out = buffer + sizeof(hdr);

	for (int i = 0; i < num_edits; i++) {
		out[i * 3 + 0] = edits[i] & 0xff;
		out[i * 3 + 1] = (edits[i] >> 8) & 0xff;
		out[i * 3 + 2] = (edits[i] >> 16) & 0xff;
	}

	sdk_save_write(slot, buffer, sizeof(buffer));

	current_world = slot;
	save_scan();
}

bool save_load(int slot)
{
	struct save_header hdr;

	if (slot < 0 || slot >= WORLD_SLOTS)
		return false;

	/* Pull the whole slot in, we need the edits as well as the header. */
	sdk_save_read(slot, buffer, sizeof(buffer));
	memcpy(&hdr, buffer, sizeof(hdr));

	if (memcmp(hdr.magic, SAVE_MAGIC, 4))
		return false;

	if (hdr.num_edits > edit_capacity())
		return false;

	hdr.name[WORLD_NAME_LEN] = 0;

	/* Step one: rebuild the world the seed describes. */
	survival_mode = hdr.survival;
	world_generate(hdr.seed);

	/* Step two: replay what the player changed. */
	const uint8_t *in = buffer + sizeof(hdr);

	for (int i = 0; i < hdr.num_edits; i++) {
		uint32_t packed = in[i * 3 + 0] | (in[i * 3 + 1] << 8) | (in[i * 3 + 2] << 16);

		int x = packed & 63;
		int y = (packed >> 6) & 63;
		int z = (packed >> 12) & 63;
		uint8_t block = (packed >> 18) & 31;

		if (world_inside(x, y, z) && block < NUM_BLOCKS)
			world_put_raw(x, y, z, block);

		edits[i] = packed;
	}

	num_edits = hdr.num_edits;
	edits_full_warned = false;

	/* Step three: the derived data has to be rebuilt from scratch. */
	world_top = 0;

	for (int x = 0; x < WORLD_X; x++)
		for (int y = 0; y < WORLD_Y; y++)
			world_update_height(x, y);

	world_rebuild_lights();

	/* Step four: put the player back, clamped in case a save is odd. */
	player_reset(true);
	player.x = clampf(hdr.x, 1.0f, WORLD_X - 1.0f);
	player.y = clampf(hdr.y, 1.0f, WORLD_Y - 1.0f);
	player.z = clampf(hdr.z, 1.0f, WORLD_Z - 2.0f);
	player.yaw = hdr.yaw;
	player.pitch = clampf(hdr.pitch, -1.5f, 1.5f);
	player.flying = hdr.flying;
	player.slot = clampi(hdr.hotbar_slot, 0, HOTBAR_SLOTS - 1);
	player.health = clampi(hdr.health, 1, PLAYER_MAX_HEALTH);
	player.respawn_x = hdr.respawn_x;
	player.respawn_y = hdr.respawn_y;
	player.respawn_z = hdr.respawn_z;

	world_time = hdr.time_of_day;
	world_play_time = hdr.play_time;

	memcpy(inventory, hdr.inventory, sizeof(inventory));

	/* Anything that lived in the old world is gone. */
	items_reset();
	mobs_reset();
	nether_reset();
	mobs_populate();

	current_world = slot;
	save_scan();
	return true;
}

void save_erase(int slot)
{
	if (slot < 0 || slot >= WORLD_SLOTS)
		return;

	/* Wiping the magic is enough to make the slot read as empty. */
	memset(buffer, 0, sizeof(buffer));
	sdk_save_write(slot, buffer, sizeof(buffer));
	save_scan();
}

void save_new(int slot, const char *name, uint32_t seed, bool survival)
{
	if (slot < 0 || slot >= WORLD_SLOTS)
		return;

	survival_mode = survival;
	world_play_time = 0.0f;
	world_time = 90.0f; /* Mid morning, so a new world starts in daylight. */

	save_forget_edits();
	world_generate(seed);

	inventory_clear();
	items_reset();
	mobs_reset();
	nether_reset();

	player_reset(false);
	mobs_populate();

	/* Record the name before the first store so the slot is labelled. */
	summaries[slot].used = true;
	snprintf(summaries[slot].name, sizeof(summaries[slot].name), "%s", name);

	current_world = slot;
	save_store(slot);
}
