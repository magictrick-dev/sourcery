/**
 * The memory management API is a monotonic allocator system that
 * fetches dynamic storage from the OS and uses that as a means of
 * storing heap allocated data. The primary advantage is that it is
 * not only fast, but incredibly simple to manage.
 * 
 * The lifetimes are decoupled from malloc/free synchrony. Allocations
 * can either be "pushed" or "popped" at will, bulk "freed", or partitioned
 * into smaller arenas that are governed by parent arenas.
 * 
 * For the most part, the code complexity here isn't particularly dense
 * aside from maintaining memory alignment and any additional extensions that
 * may be added. Implementation is placed in sourcery/memory/alloc.c and
 * platform abstractions are placed in platform/[OS]/[OS]_alloc.c respectively.
 * 
 * The super easy and super simple how-to:
 * 
 * In order to make full use of this allocator, you must first call the function
 * virtual_allocate() to retrieve some large block of n-bytes of dynamic storage.
 * 
 * Once you have your dynamic storage handy, use arena_allocate() with the region
 * acquired to initialize your mem_arena with it. You can repeat this process to
 * create more arenas (which means more calls to virtual_free() to manage) or take
 * one larger block and partition it off use additional calls to arena_allocate()
 * using the parent memory arena.
 * 
 * Using the appropriate push/pop functions to get storage as needed.
 * 
 * Want to do a bunch of general allocations but not sure about how many you'll do
 * but have a definite lifetime? Use arena_stash() and arena_restore(). 
 * 
 * TODO(Chris):
 * 
 * 			Memory Alignment
 * 			--------------------------------------------------------------------
 * 		1. 	Ensure memory alignment for arena pushes. Pushes go up the n-byte
 * 			boundary, and pops go up to the n-byte boundary. The result should
 * 			keep everything in order regardless of user-requests.
 * 
 * 
 * 			Top-Bottom Allocation Extension
 * 			--------------------------------------------------------------------
 * 		2. 	Extend the memory arena to allow for negative pushes, or top-down
 * 			pushes. This can be done by keep tracking of usable bottom-up space
 * 			and separating it from total space. When a negative value is introduced,
 * 			reduce the growth size by n-bytes and ensure that the top never reaches
 * 			the bottom. Such a system allows for greater flexability, particularly
 * 			when general allocation and pooling can be extended as such to utilize
 * 			this extension.
 * 
 * 
 * 			Pooling Extension
 * 			--------------------------------------------------------------------
 * 		3. 	Pooling will be useful for units of fixed size but lack a fixed lifetime.
 * 			Creating a pool requires some additional implementation extensions on
 * 			the memory arena and should be compatible with the Top-Bottom allocation
 * 			extension since it would benefit from the flexability of location.
 * 
 */
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
 * Clears an arena by setting the stack offset back to zero.
 * 
 * @param arena The arena to clear.
 */
void
arena_clear(mem_arena*);

/**
 * Returns the offset pointer which corresponds to the current
 * stack position of the memory arena. Use arena_restore() to
 * return the offset pointer back to the stash offset.
 * 
 * @param arena The memory arena to return the offset from.
 * 
 * @returns The offset pointer.
 */
size_t
arena_stash(mem_arena* arena);

/**
 * Resets the offset pointer to the provided stash offset,
 * effectively popping the stack down n-bytes.
 * 
 * @param arena The memory arena to restore the offset pointer.
 * @param stash_offset The offset pointer.
 */
void
arena_restore(mem_arena* arena, size_t stash_offset);

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