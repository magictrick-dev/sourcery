#include <stdio.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/memory/memutils.h>
#include <sourcery/string/string_utils.h>

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

	// Print some useful information about the allocation if debug mode is enabled.
#if defined(SOURCERY_DEBUG)
	printf("0x%llX is the base address.\n", (size_t)v_heap_ptr);
	printf("The size of the allocation is: %llu bytes with %llu committed.\n", request_size, v_heap_size);
#endif

	// If the user set final size, we should update it.
	if (final_size != NULL)
		*final_size = v_heap_size;

	return v_heap_ptr;
}

/**
 * Searches for the first token within a string.
 * 
 * @param token The token to search for.
 * @param string The string to search in.
 * @param offset The offset to which to begin searching for a token.
 * 
 * @returns The starting index position of the token, or -1 if the
 * token was not found within the string.
 */
int
find_token_string(const char* token, const char* string, int offset)
{

	// Search the string until we reach the null-terminator.
	int c_index = offset;
	while (string[c_index] != '\0')
	{

		// If the first character of the token matches the first character
		// of the string, then begin checking for the rest of the characters.
		int t_index = 0;
		if (string[c_index] == token[t_index])
		{

			// Assume the search is valid.
			bool valid = true;

			// Bump the indexes up.
			c_index++;
			t_index++;

			// As long as we're not at the end of either strings,
			// match for tokens.
			while (string[c_index] != '\0' && token[t_index] != '\0')
			{
				if (string[c_index] != token[t_index])
				{
					break;
				}
				c_index++;
				t_index++;
			}

			// The loop might fall out because we reached the end of the string
			// rather than the token. We can check for this by seeing if t_index
			// reached the string's length.
			if (t_index != string_length(token)) valid = false;

			// The position is simply c_index - t_index, which places the index
			// where the token begins.
			if (valid == true)
				return c_index-t_index;

		}

		c_index++;
	}

	return -1;

}

/**
 * Processes a file, handling directives, and then performing
 * any actions that the directives require. An arena is required
 * for storing the file and should be sized appropriately.
 * 
 * @param arena The memory arena to perform dynamic storage allocations with.
 * @param file_name The path to the file to process.
 */
void
process_file(mem_arena* arena, const char* file_name)
{

	// Stash the current position of the arena offset pointer.
	size_t stash_point = arena_stash(arena);

	// Get the text source.
	char* text_source = load_text_source(arena, file_name);

	printf("Loaded the file: %s\n", file_name);

	// Process each line.
	int offset = 0;
	while (offset != -1)
	{

		// Determine the size of the line, then create a buffer to fit the line.
		size_t line_size = get_line_size(text_source, offset) + 1;
		char* line_buffer = arena_push_array_zero(arena, char, line_size);

		// Get the line.
		offset = get_line_string(line_buffer, 256, text_source, offset);

		// Analyze the line and determine what behavior must be done on it.
		// For now, print the line.
		int directive_location = find_token_string("#!", line_buffer, 0);
		if (directive_location != -1)
		{
			printf("Search: %d Size: %llu Line: %s\n", directive_location, line_size-1, line_buffer);
		}

		// Pop the line.
		arena_pop(arena, line_size);

	}

	// Restore the arena back to its last position.
	arena_restore(arena, stash_point);

	// For debugging purposes, add space.
	printf("\n");

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

	// Begin processing the files.
	// In the future, we will want to collect our text sources, either provided by
	// the CLI as a list, or as a recursive directory search and delegate using
	// the respective platform's multi-threading API. For now, have the main thread
	// perform all the work.
	for (int i = 0; i < argc-1; ++i)
		process_file(&application_memory_heap, argv[i+1]);


	/**
	 * The operating system will reclaim memory once the application closes, invoking
	 * virtual free in the current state of the program is redundant work.
	 */
	return 0;
}

