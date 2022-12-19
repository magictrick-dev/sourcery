#include <stdio.h>
#include <main.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/memory/memutils.h>
#include <sourcery/process/process.h>
#include <sourcery/string/string_utils.h>
#include <sourcery/structures/node_trunk.h>

/**
 * Some interesting resources:
 * https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa
 * 
 * For process joining, we can use this to wait for it to exit.
 * WaitForSingleObject()...
 */

/**
 * Loads a text source from a file and places it within a memory arena.
 */
internal char*
loadSource(mem_arena* arena, const char* file)
{
	// Attempt to open the file.
	filehandle fh = {0};
	if (!platformOpenFile(&fh, file, PLATFORM_FILECONTEXT_EXISTING, PLATFORM_FILEMODE_READONLY))
	{
		printf("Error: Unable to open the file %s for reading.\n", file);
		exit(1);
	}

	// Once the file is open, determine how large the text file is and add one
	// byte for null-termination to create the buffer using the memory arena.
	size_t file_size = fh.file_size + 1;
	char* file_buffer = arena_push_array_zero(arena, char, file_size);

	// Read the file into the buffer.
	size_t bytes_read = platformReadFile(&fh, file_buffer, file_size);

	// Close the file handle.
	platformCloseFile(&fh);

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
allocateHeap(uint32 num_threads, size_t pre_thread_size, size_t* final_size)
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

	// If the user set final size, we should update it.
	if (final_size != NULL)
		*final_size = v_heap_size;

	return v_heap_ptr;
}

uint32
getDirectiveType(char directive_character)
{
	switch(directive_character)
	{
		case '!':
			return (uint32)DIRECTIVE_COMMAND;
		case '%':
			return (uint32)DIRECTIVE_MAKEDIR;
		case '+':
			return (uint32)DIRECTIVE_MAKEFILE;
		default:
			return (uint32)DIRECTIVE_UNDEFINED;
	}
}

node_trunk*
createSourceTree(mem_arena* arena, char* source)
{
	// Generate a tree for each line in the source file.
	node_trunk* sourceTree = createLinkedList(arena);

	// Go through each line and then build the linked list.
	int lineIndex = 0;
	int offset = 0;
	while (offset >= 0)
	{
		
		// Create the line source structure.
		line_source* currentLineSource = pushNodeStruct(arena, sourceTree, line_source);
		
		// Determine the length of the line, allocate a line buffer, then copy
		// over the contents of the string from the text source.
		size_t currentLineLength = strLineLength(source, offset);
		char* lineBuffer = arena_push_array_zero(arena, char, currentLineLength + 1);

		offset = strCopyLine(lineBuffer, currentLineLength + 1, source, offset);

		// Fill out the line source structure. We assume the directive is undefined
		// until line processing begins.
		currentLineSource->lineDirectiveType = DIRECTIVE_UNDEFINED;
		currentLineSource->lineNumber = lineIndex++;
		currentLineSource->stringPtr = lineBuffer;
		currentLineSource->stringLength = currentLineLength;

	}

	// Reverse the source file to be in the proper orientation.
	reverseLinkedList(sourceTree);

	// Return the trunk.
	return sourceTree;
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
processSourceFile(mem_arena* arena, const char* file_name)
{

	// Stash the current position of the arena offset pointer.
	size_t stash_point = arena_stash(arena);

	// Get the text source and then split into a source line tree.
	char* text_source = loadSource(arena, file_name);
	node_trunk* sourceTree = createSourceTree(arena, text_source);

	// Determine each directive type. Once we know what each directive type is,
	// we can then begin processing each directive based on each type.
	node_branch* currentNode = sourceTree->next;
	while (currentNode != NULL)
	{
		line_source* currentLine = (line_source*)currentNode->branch;

		// In the event that the line is shorter than 3 characters, we skip.
		if (currentLine->stringLength > 2)
		{
			// Ensure the directive is at the start.
			int token_location = strSearchToken("#!", currentLine->stringPtr, 0);
			if (token_location == 0)
			{
				// The 3rd character determines the directive type.
				currentLine->lineDirectiveType = getDirectiveType(currentLine->stringPtr[2]);
			}
		}

		currentNode = currentNode->next;
	}

	// Now that we have the directive types defined, we can begin processing each
	// directive as we come across them.
	currentNode = sourceTree->next;
	while (currentNode != NULL)
	{
		line_source* currentLine = (line_source*)currentNode->branch;
		if (currentLine->lineDirectiveType != DIRECTIVE_NONE &&
			currentLine->lineDirectiveType != DIRECTIVE_UNDEFINED)
		{

			// Set a stash point so we can freely allocate per directive.
			size_t directive_stash_point = arena_stash(arena);

			char* directive_buffer = arena_push_array_zero(arena, char, currentLine->stringLength+1);
			strSubstring(directive_buffer, currentLine->stringLength+1, currentLine->stringPtr, 3, -1);

			// Perform the required processes.
			switch(currentLine->lineDirectiveType)
			{
				
				case DIRECTIVE_MAKEDIR:
				{
					// We can now create the directory.
					if (platformCreateDirectory(directive_buffer))
					{
						printf("Directory was created at %s.\n", directive_buffer);
					}
					else
					{
						printf("Directory couldn't be created at %s.\n", directive_buffer);
					}
					break;
				}
				case DIRECTIVE_MAKEFILE:
				{
					// The makefile procedure make be multiline, and therefore we need to
					// account for that by scanning ahead for the contents should that be the case.
					char* new_file_name = directive_buffer;
					char* text_contents = NULL;

					// Seperate the filename from the next.
					size_t text_seperator_location = strSearchToken(":", directive_buffer, 0);
					if (text_seperator_location != -1)
					{

						// We need to pull the file name out.
						new_file_name = arena_push_array_zero(arena, char, currentLine->stringLength+1);
						strSubstring(new_file_name, currentLine->stringLength+1, directive_buffer,
							0, text_seperator_location);

						// Now we need fetch the next contents.
						text_contents = arena_push_array_zero(arena, char, currentLine->stringLength+1);
						strSubstring(text_contents, currentLine->stringLength+1, directive_buffer,
							text_seperator_location+1, -1);

					}

					// In most cases, files are generated using the multiline operator. We need to ensure
					// that we capture all the data properly.
					node_trunk* text_trunk = createLinkedList(arena);
					size_t multiline_location = strSearchToken("<<(", directive_buffer, 0);
					if (multiline_location != -1)
					{

						// Since the first line may contain the ending token, we should set the loop up to
						// check for that. We can allocate a new string on the heap all string data which comes
						// after the multiline operator.
						char* working_line = arena_push_array_zero(arena, char, currentLine->stringLength+1);
						strSubstring(working_line, currentLine->stringLength+1, directive_buffer,
							multiline_location+3, -1);

						// We now need to go through each node in the loop and search for the multiline end operator.
						while (currentNode != NULL)
						{
							size_t multiline_end_location = strSearchToken(")>>", working_line, 0);
							size_t end_location = -1;
							if (multiline_end_location != -1)
								end_location = multiline_end_location;
							
							node_branch* current_branch = pushNode(arena, text_trunk, sizeof(char**));
							char** current_text_ptr = (char**)current_branch->branch;
							*current_text_ptr = arena_push_array_zero(arena, char, currentLine->stringLength+1);
							strSubstring(*current_text_ptr, currentLine->stringLength+1, working_line,
								0, end_location);

							// Exit the loop.
							if (multiline_end_location != -1)
								break;
							else
							{
								currentNode = currentNode->next;
								currentLine = (line_source*)currentNode->branch;
								working_line = currentLine->stringPtr;
							}
						}
					}

					// Even though there isn't a multiline operator used, we should use the linked-list
					// to write to the file rather than utilizing if-statements to perform the same work.
					else
					{
						if (text_contents != NULL)
						{
							node_branch* current_branch = pushNode(arena, text_trunk, sizeof(char**));
							char** current_text_ptr = (char**)current_branch->branch;
							*current_text_ptr = arena_push_array_zero(arena, char, currentLine->stringLength+1);
							strSubstring(*current_text_ptr, currentLine->stringLength+1, directive_buffer,
								text_seperator_location+1, -1);
						}
					}

					// Since the text trunk will be backwards, we need to reverse it.
					reverseLinkedList(text_trunk);

					// Process the linked list of all the strings that we need to write to file.
					filehandle directivefh = {0};
					if (platformOpenFile(&directivefh, new_file_name,
						PLATFORM_FILECONTEXT_ALWAYS, PLATFORM_FILEMODE_TRUNCATE))
					{
						node_branch* current_filetext_branch = text_trunk->next;
						while (current_filetext_branch != NULL)
						{
							char* write_text_ptr = *((char**)current_filetext_branch->branch);
							platformWriteFile(&directivefh, write_text_ptr, strLength(write_text_ptr));
							platformWriteFile(&directivefh, "\n", 1);
							current_filetext_branch = current_filetext_branch->next;
						}
						platformCloseFile(&directivefh);
						printf("File %s was created.\n", new_file_name);
					}
					else
					{
						printf("Unable to create %s.\n", new_file_name);
					}

					break;
				}
				case DIRECTIVE_COMMAND:
				{
					printf("Executing '%s'.\n", directive_buffer);
					platformRunCLIProcess(directive_buffer);
					break;
				}
				default:
				{
					printf("Unrecognized/unimplemented directive on line %4d\n%s\n", currentLine->lineNumber, currentLine->stringPtr);
					break;
				}
			}

			// Restore the stash point back to where it should be.
			arena_restore(arena, directive_stash_point);
		}
		currentNode = currentNode->next;
	}

	// Restore the arena back to its last position.
	arena_restore(arena, stash_point);

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
	void* virtual_heap_ptr = allocateHeap(1, MEGABYTES(64), &virtual_heap_size);
	mem_arena application_memory_heap = {0};
	arena_allocate(virtual_heap_ptr, virtual_heap_size, &application_memory_heap);

	// Begin processing the files.
	// In the future, we will want to collect our text sources, either provided by
	// the CLI as a list, or as a recursive directory search and delegate using
	// the respective platform's multi-threading API. For now, have the main thread
	// perform all the work.
	processSourceFile(&application_memory_heap, argv[1]);

	/**
	 * The operating system will reclaim memory once the application closes, invoking
	 * virtual free in the current state of the program is redundant work.
	 */
	return 0;
}

