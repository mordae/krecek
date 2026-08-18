#include "mem.h"

#include <string.h>

/*
 * The one and only block of memory we manage.
 *
 * It is a plain static array, so the linker reserves it at build time and
 * we can see its cost in the firmware size report. Nothing here can ever
 * ask the system for more memory, which means the game either fits or
 * fails to link. That is exactly the property you want on a console.
 */
static uint8_t arena[MEM_ARENA_SIZE];

/* How far into the arena we have handed memory out. */
static size_t arena_offset;

/*
 * Round `value` up to the next multiple of `align`.
 *
 * The bit trick works because our alignments are always powers of two.
 * Adding align - 1 pushes us past the boundary, and masking off the low
 * bits drops back down to it. For align = 4:
 *
 *     5 + 3 = 8, 8 & ~3 = 8
 *     8 + 3 = 11, 11 & ~3 = 8   (already aligned, stays put)
 */
static size_t align_up(size_t value, size_t align)
{
	return (value + align - 1) & ~(align - 1);
}

void mem_init(void)
{
	arena_offset = 0;

	/*
	 * Wiping the arena is not strictly necessary, but it makes bugs
	 * repeatable: memory always starts as zeroes rather than as
	 * whatever the last world left behind.
	 */
	memset(arena, 0, sizeof(arena));
}

void *mem_arena_alloc(size_t size)
{
	if (!size)
		return NULL;

	/*
	 * Align to 4 bytes. The Cortex-M0+ in this console cannot load a
	 * 32 bit word from an unaligned address at all, it faults, so this
	 * is not an optimisation but a correctness requirement.
	 */
	size_t start = align_up(arena_offset, 4);
	size_t end = start + size;

	if (end > MEM_ARENA_SIZE)
		return NULL;

	arena_offset = end;
	return &arena[start];
}

size_t mem_arena_used(void)
{
	return arena_offset;
}

size_t mem_arena_total(void)
{
	return MEM_ARENA_SIZE;
}

/* Address of slot number `index` inside the pool. */
static uint8_t *slot_at(const struct mem_pool *pool, uint16_t index)
{
	return pool->storage + (size_t)index * pool->item_size;
}

/*
 * Read and write the "next free slot" link that lives inside a free slot.
 *
 * We copy it in and out with memcpy rather than casting the slot to a
 * uint16_t pointer, because the compiler is allowed to assume that a
 * uint8_t array and a uint16_t never overlap. memcpy of two bytes costs
 * nothing after optimisation and keeps the code strictly correct.
 */
static uint16_t link_get(const struct mem_pool *pool, uint16_t index)
{
	uint16_t next;
	memcpy(&next, slot_at(pool, index), sizeof(next));
	return next;
}

static void link_set(struct mem_pool *pool, uint16_t index, uint16_t next)
{
	memcpy(slot_at(pool, index), &next, sizeof(next));
}

bool mem_pool_init(struct mem_pool *pool, const char *name, size_t item_size, uint16_t capacity)
{
	memset(pool, 0, sizeof(*pool));

	pool->name = name;
	pool->free_head = MEM_NONE;

	if (!capacity || capacity >= MEM_NONE)
		return false;

	/* Every slot must be able to hold the free list link. */
	if (item_size < sizeof(uint16_t))
		item_size = sizeof(uint16_t);

	item_size = align_up(item_size, 4);

	uint8_t *storage = mem_arena_alloc(item_size * (size_t)capacity);

	if (!storage)
		return false;

	pool->storage = storage;
	pool->item_size = item_size;
	pool->capacity = capacity;

	/*
	 * Thread every slot onto the free list, in order, so that the first
	 * allocations come back in a predictable order. Slot 0 links to 1,
	 * 1 links to 2, and the last one ends the chain.
	 */
	for (uint16_t i = 0; i + 1 < capacity; i++)
		link_set(pool, i, i + 1);

	link_set(pool, capacity - 1, MEM_NONE);
	pool->free_head = 0;

	return true;
}

void *mem_pool_alloc(struct mem_pool *pool)
{
	/* An empty or uninitialised pool has no head to pop. */
	if (pool->free_head == MEM_NONE)
		return NULL;

	uint16_t index = pool->free_head;
	pool->free_head = link_get(pool, index);
	pool->used++;

	uint8_t *item = slot_at(pool, index);
	memset(item, 0, pool->item_size);

	return item;
}

void mem_pool_free(struct mem_pool *pool, void *item)
{
	if (!item || !pool->storage)
		return;

	/*
	 * Work out which slot this is, and refuse anything that does not
	 * land exactly on a slot boundary inside our storage. A stray free
	 * would otherwise corrupt the free list, and that kind of bug
	 * surfaces thousands of frames later somewhere completely
	 * unrelated.
	 */
	uint8_t *bytes = item;

	if (bytes < pool->storage)
		return;

	size_t offset = (size_t)(bytes - pool->storage);

	if (offset % pool->item_size)
		return;

	uint16_t index = (uint16_t)(offset / pool->item_size);

	if (index >= pool->capacity)
		return;

	/*
	 * Guard against freeing the same slot twice. Walking the free list
	 * is O(n), but frees are rare compared to everything else we do in
	 * a frame, and a double free is worth catching.
	 */
	for (uint16_t i = pool->free_head; i != MEM_NONE; i = link_get(pool, i))
		if (i == index)
			return;

	link_set(pool, index, pool->free_head);
	pool->free_head = index;
	pool->used--;
}

uint16_t mem_pool_used(const struct mem_pool *pool)
{
	return pool->used;
}

uint16_t mem_pool_capacity(const struct mem_pool *pool)
{
	return pool->capacity;
}
