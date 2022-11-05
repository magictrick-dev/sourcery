#ifndef SOURCERY_MEMORY_ALLOC_H
#define SOURCERY_MEMORY_ALLOC_H
#include <sourcery/generics.h>
#include <sourcery/memory/memutils.h>

/**
 * A memory arena is a region of dynamically allocated memory which monotonically
 * grows as a stack and can be pushed/popped as needed.
 */
typedef struct
{
	size_t size;
	size_t offset;
	void* buffer;
} mem_arena;

/**
 * Creates an arena using the provided dynamic storage. It's recommended to use
 * the provided virtual_allocate() function to generate the region of memory for
 * an arena or use existing arenas to sub-partition.
 * 
 * @param region The dynamically allocated region which the arena will use.
 * @param region_size The size of the region.
 * @param arena The arena to initialize.
 */
void
arena_allocate(void* region, size_t region_size, mem_arena* arena);

/**
 * Releases an arena. This will not release the region of dynamically allocated
 * memory since the region of memory that the particular arena may be associated
 * to parent memory arena.
 * 
 * @param arena The memory arena to release.
 */
void
arena_release(mem_arena* arena);

/**
 * Macros which allow for better interfacing with arena_pushes.
 */

#define arena_push_struct(arena, type) (type*)arena_push(arena, sizeof(type))
#define arena_push_struct_zero(arena, type) (type*)arena_push_zero(arena, sizeof(type))
#define arena_push_array(arena, type, count) (type*)arena_push(arena, sizeof(type)*(count))
#define arena_push_array_zero(arena, type, count) (type*)arena_push_zero(arena, sizeof(type)*(count))


/**
 * Pushes to an arena and returns a pointer to usable memory.
 * 
 * @param arena The arena to push onto.
 * @param size The minimum size, in bytes, to push onto the stack.
 * 
 * @returns A pointer to usable heap.
 */
void*
arena_push(mem_arena* arena, size_t size);

/**
 * Pushes to an arena and returns a pointer to usable memory that was zero'd out.
 * 
 * @param arena The arena to push onto.
 * @param size The minimum size, in bytes, to push onto the stack.
 * 
 * @returns A pointer to usable heap.
 */
void*
arena_push_zero(mem_arena* arena, size_t size);

/**
 * Pops bytes from the top of the arena.
 * 
 * @param arena The arena to pop from.
 * @param size The number of bytes to remove the top.
 */
void
arena_pop(mem_arena* arena, size_t size);

/**
 * -----------------------------------------------------------------------------
 * Platform Specific Definitions
 * -----------------------------------------------------------------------------
 * You will find the platform-specific implementations in
 * the platform/[target-os]/[target-os]_alloc.c
 */

/**
 * Allocates a region of space to the nearest granular page addressable by the operating
 * system. The region size will round up to nearest boundary and therefore may be larger
 * than the requested size.
 * 
 * @param region The pointer that will be set to the beginning adress of dynamic storage.
 * @param region_size The size request to allocate which will be updated to the size returned.
 * @param use_base If this value is non-zero, the beginning address of the allocation will
 * attempt to use the provided base location.
 * @param base The starting address of the dyanmic storage to use if use_base is non-zero.
 * 
 * @returns True if the allocation was successful, false if not.
 */
bool virtual_allocate(void** region, size_t* region_size, uint64 base);

/**
 * Frees a region of memory provided by virtual allocate.
 * 
 * @param region The region of memory to use the OS's virtual free.
 * 
 * @returns True if the free was successful, false if not.
 */
bool virtual_free(void** region);

#endif