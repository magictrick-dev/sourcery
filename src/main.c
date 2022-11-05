#include <stdio.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/memory/memutils.h>

/**
 * Loads a text source from a file and places it within a memory arena.
 */
internal char*
load_text_source(mem_arena* arena, const char* file)
{
	// Attempt to open the file.
	fhandle file_handle = {0};
	if (!platform_open_file(&file_handle, file))
	{
		printf("Error: Unable to open the file %s for reading.\n", file);
		exit(1);
	}

	// Once the file is open, determine how large the text file is and add one
	// byte for null-termination to create the buffer using the memory arena.
	size_t file_size = platform_filesize(&file_handle) + 1;
	char* file_buffer = arena_push_array_zero(arena, char, file_size);

	// Read the file into the buffer.
	size_t bytes_read = platform_read_file(&file_handle, file_buffer, file_size);

	// Close the file handle.
	platform_close_file(&file_handle);

	return file_buffer;
}

/**
 * Virtually allocates n-bytes from the heap times the number of executing threads.
 * Used for partitioning areas of the heap to a particular thread. The size provided
 * may be rounded up to nearest granular page boundary supported by the operating system.
 * 
 * @param num_threads The number of threads being used. Use 1 if the application is
 * not using threads.
 * @param pre_thread_size The size, in bytes, that should be virtually allocated for
 * each thread.
 * @param final_size The size, in bytes, that was actually virtually allocated.
 * 
 * @returns A pointer to the virtually allocated heap.
 */
internal void*
allocate_heap(uint32 num_threads, size_t pre_thread_size, size_t* final_size)
{
	void* v_heap_ptr = NULL;

	/**
	 * Using a memory offset during debugging will be useful for maintaining
	 * allocation positions that persist between loads.
	 */
#if defined(SOURCERY_DEBUG)
	size_t offset = TERABYTES(2);
#else
	size_t offset = 0;
#endif

	size_t request_size = pre_thread_size * num_threads; // Preserve the request amount for debugging purposes.
	size_t v_heap_size = request_size;
	if (!virtual_allocate(&v_heap_ptr, &v_heap_size, offset))
	{
		printf("Error: Unable to allocated the necessary amount of heap %zu to run.\n", v_heap_size);
		exit(1);
	}

#if defined(SOURCERY_DEBUG)
	// Print some useful information about the allocation if debug mode is enabled.
	printf("0x%llX is the base address.\n", (size_t)v_heap_ptr);
	printf("The size of the allocation is: %llu bytes with %llu committed.\n", request_size, v_heap_size);
#endif

	// If the user set final size, we should update it.
	if (final_size != NULL)
		*final_size = v_heap_size;

	return v_heap_ptr;
}

int
main(int argc, char** argv)
{

	// Determine if there are file(s) to open. Multiple files may be provided.
	if (argc < 2)
	{
		printf("Error: Supply the file-name(s) to process.\n");
		exit(1);
	}

	// Initialize the application memory space we will need to run the application.
	// Since this is currently a single-threaded application, we will reserve 64MB
	// for the main thread. Text files aren't very large, so this should suffice.
	size_t virtual_heap_size = 0;
	void* virtual_heap_ptr = allocate_heap(1, MEGABYTES(64), &virtual_heap_size);
	mem_arena application_memory_heap = {0};
	arena_allocate(virtual_heap_ptr, virtual_heap_size, &application_memory_heap);

	// Determine the number of sources provided as CLI arguments. Create a text source
	// array that we can use to store each text source as.
	int sources_size = argc-1;
	char** text_source_array = arena_push_array_zero(&application_memory_heap, char*, sources_size);

	// Load each text source into memory.
	for (int i = 1; i < argc; ++i)
	{
		text_source_array[i-1] = load_text_source(&application_memory_heap, argv[i]);
	}

	// Print each text source.
	for (int i = 0; i < sources_size; ++i)
	{
		printf("\n*** START OF SOURCE %d ***\n%s\n*** END OF SOURCE ***\n", i+1, text_source_array[i]);
	}

	/**
	 * The operating system will reclaim memory once the application closes, invoking
	 * virtual free in the current state of the program is redundant work.
	 */
	return 0;
}

