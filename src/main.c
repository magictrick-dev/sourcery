#include <stdio.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/memory/memutils.h>
#include <sourcery/string/string_utils.h>
#include <sourcery/structures/node_trunk.h>

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

#define DIRECTIVE_UNSET -2
#define DIRECTIVE_UNDEFINED -1
#define DIRECTIVE_COMMENT 0
#define DIRECTIVE_VAR 1
#define DIRECTIVE_COMMAND 2

typedef struct source_line
{
	char* source;
	size_t line_size;
	
	int32 position;
	int32 directive_type;
} source_line;

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

	// Generate a token list.
	char** token_list = arena_push_array_zero(arena, char*, 2);
	token_list[0] = arena_push_array_zero(arena, char, 3);
	token_list[1] = arena_push_array_zero(arena, char, 3);

	// Copy the tokens into the token list.
	const char* directive_command_str = "#!";
	token_list[0] = string_copy(token_list[0], 3,
		directive_command_str, string_length(directive_command_str));
	const char* directive_comment_str = "##";
	token_list[1] = string_copy(token_list[1], 3,
		directive_comment_str, string_length(directive_comment_str));

	// Generate a tree for each line in the source file.
	node_trunk* source_file_tree = create_node_tree(arena);
	int offset = 0;
	int line_index = 1;
	while (offset != -1)
	{
		// Determine the size of the line, then create a buffer to fit the line.
		size_t line_size = string_get_line_length(text_source, offset) + 1;
		char* line_buffer = arena_push_array_zero(arena, char, line_size);

		// Get the line.
		offset = string_get_line(line_buffer, line_size, text_source, offset);

		// Create a source line.
		source_line* current_line = push_node_struct(arena, source_file_tree, source_line);
		current_line->position = line_index++;
		current_line->source = line_buffer;
		current_line->line_size = line_size;
		current_line->directive_type = DIRECTIVE_UNSET;
	}

	// The lines will be read in reverse, so we need to reverse the list.
	reverse_node_tree(source_file_tree);

	// Once we have the source lines, we should print them out.
	node_branch* current_branch = source_file_tree->next;
	while (current_branch != NULL)
	{
		source_line* current_line = (source_line*)current_branch->branch;
		//printf("Line %.4d : %s\n", current_line->position, current_line->source);

		int directive_location = string_find_token_from_list(token_list, 2, current_line->source, 0);
		while (directive_location != -1)
		{
			printf("Search: %d Size: %llu Line: %s\n", directive_location, current_line->line_size-1, current_line->source);
			directive_location = string_find_token_from_list(token_list, 2, current_line->source, directive_location+2);
		}

		current_branch = current_branch->next;
	}

	/**
	 * 
	 * We are going to attempt to use a linked list to store the lines.
	 * 
	
	// Process each line.
	int offset = 0;
	while (offset != -1)
	{

		
		// Determine the size of the line, then create a buffer to fit the line.
		size_t line_size = string_get_line_length(text_source, offset) + 1;
		char* line_buffer = arena_push_array_zero(arena, char, line_size);

		// Get the line.
		offset = string_get_line(line_buffer, 256, text_source, offset);

		// Analyze the line and determine what behavior must be done on it.
		// For now, print the line.
		int directive_location = string_find_token_from_list(token_list, 2, line_buffer, 0);
		while (directive_location != -1)
		{
			printf("Search: %d Size: %llu Line: %s\n", directive_location, line_size-1, line_buffer);
			directive_location = string_find_token_from_list(token_list, 2, line_buffer, directive_location+2);
		}

		// Pop the line.
		arena_pop(arena, line_size);

	}

	*/

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

