#include <sourcery/memory/alloc.h>

void
arena_allocate(void* region, size_t region_size, mem_arena* arena)
{
	arena->buffer = region;
	arena->size = region_size;
	arena->offset = 0;

	return;
}

void
arena_release(mem_arena* arena)
{

	arena->buffer = NULL;
	arena->size = 0;
	arena->offset = 0;

	return;

}


void*
arena_push(mem_arena* arena, size_t size)
{

	/**
	 * We should ensure that our pushes are aligned... but for now we will leave
	 * as is for simplicity purposes.
	 * 
	 * TODO(Chris): Ensure memory alignment.
	 */

	// Ensure that we can fit the allocation.
	assert(arena->offset + size < arena->size);

	// Push onto the arena stack.
	void* buffer = (uint8*)arena->buffer + arena->offset;
	arena->offset += size;

	return buffer;

}

void*
arena_push_zero(mem_arena* arena, size_t size)
{

	// We are forwarding off to the standard arena_push() and then memory setting to zero.
	void* buffer = arena_push(arena, size);
	memory_set(buffer, size, 0x00);

	return buffer;

}

void
arena_pop(mem_arena* arena, size_t size)
{

	arena->offset -= size;
	if (arena->offset < 0) arena->offset = 0;

}

void
arena_clear(mem_arena* arena)
{

	arena->offset = 0;

}

size_t
arena_stash(mem_arena* arena)
{
	return arena->offset;
}

void
arena_restore(mem_arena* arena, size_t stash_offset)
{
	arena->offset = stash_offset;
}
