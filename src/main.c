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
 * Gets the size of the line starting at a given offset, in bytes, up
 * to the next line.
 * 
 * @param string The string to get the size of the line.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The size, in bytes, of the line.
 */
size_t
get_line_size(char* string, int offset)
{

	char* current_char = string + offset;
	size_t line_size = 0;
	while (current_char[line_size] != '\0')
	{

		// Since Windows uses carriage returns along with the newline,
		// we need to ensure that we account for that.
#if defined(PLATFORM_WINDOWS)
		if (current_char[line_size] == '\r' && current_char[line_size+1] == '\n')
			break;
#else
		if (current_char == '\n')
			break;
#endif

		line_size++;
	}

	return line_size;

}

/**
 * Retrieves a line from a provided string and copies it to the buffer starting
 * from an offset. The value returned is the offset to the next line otherwise
 * returns -1.
 * 
 * @param buffer The buffer to copy the line into.
 * @param buffer_size The size of the buffer.
 * @param string The string to pull the line from.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The offset to the next line otherwise -1.
 */
int
get_line_string(char* buffer, size_t buffer_size, char* string, int offset)
{

	// Ensure that line fits within the buffer.
	if (get_line_size(string, offset) + 1 > buffer_size)
	{
		// Assertion for debugging, otherwise just return -1.
		assert(!"Unable to fit in buffer.");
		return -1;
	}

	// Determine the pointer using the provided offset.
	char* current_char = string + offset;
	size_t line_size = 0;
	while (current_char[line_size] != '\0')
	{
		// Since Windows uses carriage returns along with the newline,
		// we need to ensure that we account for that.
#if defined(PLATFORM_WINDOWS)
		if (current_char[line_size] == '\r' && current_char[line_size+1] == '\n')
			break;
#else
		if (current_char == '\n')
			break;
#endif
		buffer[line_size] = current_char[line_size];
		line_size++;
	}
	buffer[line_size] = '\0'; // Null terminate.

	// If its the end of the string, return -1 since there are no other lines.
	if (current_char[line_size] == '\0') return -1;
	else
	{
		// Once again, Windows is weird so we need to account for that.
#if defined(PLATFORM_WINDOWS)
		return offset + line_size + 2;
#else
		return offset + line_size + 1;
#endif
	}
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

	// Retrieve each line from each file and print it.
	char line_buffer[256];
	for (int source_index = 0; source_index < sources_size; ++source_index)
	{
		int offset = 0;
		int line_index = 0;
		while (offset != -1)
		{
			// Get the line.
			offset = get_line_string(line_buffer, 256, text_source_array[source_index], offset);

			// Print the line.
			printf("File %d, Line %d: %s\n", source_index, ++line_index, line_buffer);
		}
		if (source_index < sources_size-1) printf("\n");
	}


	/**
	 * The operating system will reclaim memory once the application closes, invoking
	 * virtual free in the current state of the program is redundant work.
	 */
	return 0;
}

