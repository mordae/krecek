#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * ===========================================================================
 * Our own memory allocation
 * ===========================================================================
 *
 * WHY NOT JUST USE malloc()?
 *
 * The console has 264 KiB of RAM in total and no memory protection. A
 * general purpose malloc() has three problems for a game like this:
 *
 *   1. It can fail at any moment, in the middle of a frame, and there is
 *      nowhere sensible to put the error handling.
 *   2. It fragments. Allocate and free a few thousand mobs and you end up
 *      with plenty of free bytes but no contiguous run big enough to use.
 *   3. It is slow and its cost is unpredictable, which is awful when you
 *      have a 16 ms budget per frame.
 *
 * So we do what most console and embedded games do: we decide up front how
 * much memory we are allowed to use, take it all as one static array, and
 * hand it out ourselves with allocators that cannot fragment.
 *
 * We build two of them, and they are the two you will meet in almost every
 * game engine:
 *
 *   ARENA (also called a bump or linear allocator)
 *   -------------------------------------------------------------------
 *   Keep one pointer into a block of memory. To allocate, move the pointer
 *   forward and return where it used to be. That is the whole algorithm.
 *
 *       [####used####|                free                 ]
 *                    ^ offset
 *
 *   Allocation is a handful of instructions and can never fragment. The
 *   catch is that you cannot free one item on its own, only reset the
 *   whole arena at once. That is perfect for things that live as long as
 *   the game does, which is what we use it for.
 *
 *   POOL (also called a fixed size block allocator)
 *   -------------------------------------------------------------------
 *   Every item in a pool is exactly the same size, so any free slot fits
 *   any request and fragmentation is impossible by construction. We carve
 *   the pool's storage out of the arena once, then track which slots are
 *   free with a FREE LIST.
 *
 *   The trick worth learning: the free list costs no extra memory. A slot
 *   that is free holds no useful data, so we store the index of the next
 *   free slot inside the slot itself:
 *
 *       slot:   0        1        2        3        4
 *              [used  ] [next=3] [used  ] [next=4] [next=NONE]
 *                        ^free_head
 *
 *   Allocating pops the head of that chain, freeing pushes onto it. Both
 *   are a couple of instructions and take the same time no matter how full
 *   the pool is.
 *
 * This is the file to read first if you want to understand how games keep
 * thousands of moving objects alive without ever calling malloc().
 */

/* Total memory we are willing to spend on dynamic objects. */
#define MEM_ARENA_SIZE 6144

/* Index used to mean "there is no next free slot". */
#define MEM_NONE 0xffff

/*
 * A pool of fixed size items.
 *
 * Do not poke at these fields from outside, use the functions below. They
 * are visible only so that you can put a struct mem_pool in a static
 * variable instead of having to allocate the pool itself.
 */
struct mem_pool {
	const char *name;   /* For the debug screen. */
	uint8_t *storage;   /* First byte of the first slot. */
	uint16_t item_size; /* Bytes per slot, at least 2. */
	uint16_t capacity;  /* Number of slots. */
	uint16_t free_head; /* First free slot, or MEM_NONE when full. */
	uint16_t used;	    /* Slots currently handed out. */
};

/*
 * Reset the arena and forget every pool carved out of it.
 *
 * Call this once at startup, before creating any pool. Calling it again
 * invalidates every pointer the allocators ever returned, so only do that
 * when tearing the whole world down.
 */
void mem_init(void);

/*
 * Take `size` bytes out of the arena, aligned so that any type fits.
 *
 * Returns NULL when the arena is exhausted. There is no matching free,
 * that is the whole point of an arena. Panics are left to the caller so
 * that you can decide what running out of memory means for your game.
 */
void *mem_arena_alloc(size_t size);

/* Bytes handed out so far, and the total, for the debug screen. */
size_t mem_arena_used(void);
size_t mem_arena_total(void);

/*
 * Prepare a pool of `capacity` items of `item_size` bytes each.
 *
 * The storage comes out of the arena, so the pool itself never grows and
 * never gives memory back. Returns false if the arena cannot fit it, in
 * which case the pool is left empty and every allocation from it fails.
 *
 * `item_size` is rounded up to at least 2 bytes and to a multiple of 4, so
 * that the free list index fits and every slot stays aligned.
 */
bool mem_pool_init(struct mem_pool *pool, const char *name, size_t item_size, uint16_t capacity);

/*
 * Take one slot out of the pool.
 *
 * The returned memory is zeroed, which costs a little but removes a whole
 * category of bugs where a recycled mob remembers a field from its past
 * life. Returns NULL when every slot is in use.
 */
void *mem_pool_alloc(struct mem_pool *pool);

/*
 * Give a slot back.
 *
 * Passing NULL is fine and does nothing. Passing a pointer that did not
 * come from this pool, or freeing the same slot twice, corrupts the free
 * list, so the function checks both and simply ignores bad calls rather
 * than scribbling over memory.
 */
void mem_pool_free(struct mem_pool *pool, void *item);

/* How many slots are in use, and how many there are in total. */
uint16_t mem_pool_used(const struct mem_pool *pool);
uint16_t mem_pool_capacity(const struct mem_pool *pool);
