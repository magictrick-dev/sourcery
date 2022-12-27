#include <stdio.h>
#include <main.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/memory/memutils.h>
#include <sourcery/process/process.h>
#include <sourcery/string/string_utils.h>
#include <sourcery/structures/node_trunk.h>

/**
 * 
 * Application Runtime Order:
 * 		1. 	Run the CLI-parser.
 * 				This will collect all the necessary run-time details we need to run.
 * 				The CLI-parser will also ensure that the minimum requirements are
 * 				needed to run. This means that the files provided to Sourcery exist
 * 				and that all the directories have been scanned.
 * 
 * 		2. 	Initialize the symbols table.
 * 				We need to create a symbols table with pre-defined globals. This table
 * 				is constructed with hard-defaults in the event that no configuration
 * 				file is found.
 * 
 * 		3. 	Load the configuration file(s).
 * 				Once we have the symbols table created, we need to load all the config
 * 				files. These config files may define new variables and macros or overwrite
 * 				existing ones.
 * 
 * 		4. 	Preprocessor Compiler Pass One
 * 				The first pass scans each file and builds the local symbols table. All variables
 * 				and macros are local to their file and may not modify the globals table.
 * 				
 *  			Magic constants are created for that Source file. Magic constants
 * 				define the file name, the file path, whether or not the file is a script.
 * 
 * 				Some directives may be processed at this stage, which are usually macro
 * 				definitions, variable definitions, header definitions, directory, and
 * 				file generations.
 * 
 * 		5a. Preprocessor Compiler Pass Two
 * 				This is where the meat and potatoes are at. Once each file is properly
 * 				constructed and the symbols table is built, we can begin consuming all
 * 				of the macro commands.
 * 
 * 				Since the symbols table are already constructed, we can blast through
 * 				each file with multi-threading. Create a work-queue, pull from that
 * 				work-queue when a thread isn't busy, process, repeat. Each thread
 * 				can have its own heap memory block which will prevent any race-conditions.
 * 				
 * 		5b. Optional File Modification
 * 				The second pass will generate a second source file in memory, running
 * 				procedurally, until EOF. A second file will be generated in-memory as
 * 				this happens, constructing a directive-stripped version of each file.
 * 				
 * 				If Sourcery is designated to do modifications and the file isn't in
 * 				script-mode, the second-pass will dump these changes to disk after
 * 				storing a backup in ".sourcery".
 * 
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

internal uint32
getDirectiveType(char directive_character)
{
	switch(directive_character)
	{
		case '#':
			return (uint32)DIRECTIVE_HEADER;
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

internal node_trunk*
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
internal void
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

/**
 * Sourcery Useage:
 * 		r: 	Recursive search on any directories provided.
 * 		u: 	Allows the modification of source files that are not marked as a
 * 			script by stripping the preprocessor directives.
 * 
 * 		sourcery [OPT:(-r)(-u)] [file(s) or directory(s)]
 * 			Runs the preprocessor on the selected files and directories. This is
 * 			not a recursive process and will only run on the provided root directories.
 * 			Providing the "-r" flag will allow the recursive search of directories.
 * 			Any and all source files will, by default, not be modified. Therefore,
 * 			the flag "-u" is required to allow this behavior. Text files that are
 * 			set to "script mode" will not be modified regardless of this flag's presence.
 * 
 * TBI CLI Features:
 * 		sourcery [OPT:--config (config_file)] [OPT:(-r)(-u)] [file(s) or directory(s)]
 * 			A configuration file is its own Sourcery script which defines default
 * 			behaviors in the symbol table. These scripts are loaded in this order:
 * 				1. Executable Directory (Global Defaults)
 * 				2. Calling Directory (Project Defaults)
 * 				3. CLI-passed Configs (User Specified at Runtime)
 * 			This means that global defaults are overwritten by project defaults
 * 			and project defaults are overwritten by CLI-passed configs. CLI-passed
 * 			configs honor the order they passed in, which means file_a, file_b,
 * 			and file_c are 4, 5, and 6 in the load chain.
 * 
 * 		sourcery --rollback
 * 			In the event that a macro doesn't go as planned, Sourcery will store
 * 			copies of the project prior to the last usage of "sourcery -u". Rollbacks
 * 			are stored in a private directory in the root calling directory. This
 * 			directory is called ".sourcery" which contains the meta files necessary
 * 			to perform the rollbacks.
 * 
 */

/**
 * A helper function which sets an array of at least 52 bools to the bit status.
 * 
 * @param flagsArray An array of at least 52 bools to store the flag's state.
 * @param flagsBit A pointer to the flags bit from argument_properties.
 */
void
setCLIFlagsArray(bool flagsArray[52], uint64* flagsBit)
{
	for (size_t flagIndex = 0; flagIndex < 52; ++flagIndex)
		flagsArray[flagIndex] = ((*flagsBit >> flagIndex) & 0x1);
}

/**
 * Validates the parse CLI arguments.
 * 
 * @param arena The memory arena that can be allocated to.
 * @param arguments The cliargs structure.
 * 
 * @returns True if the validation succeeded, false if not.
 */
bool
validateParsedCLI(mem_arena* arena, cliargs* arguments)
{

	node_branch* currentBranch = arguments->argumentTree->next;
	while (currentBranch != NULL)
	{
		argument_properties* argument = (argument_properties*)currentBranch->branch;
		if (argument->argumentType == ARGTYPE_TOKEN)
		{
			char* argumentString = (char*)argument->argumentPtr;
			printf("TOKEN : Index %d: %s\n", argument->argumentIndex, argumentString);
		}
		else if (argument->argumentType == ARGTYPE_FLAG)
		{
			// Collect the flags.
			bool flags[52];
			setCLIFlagsArray(flags, (uint64*)argument->argumentPtr);

			printf("FLAGS : Index %d: -", argument->argumentIndex);

			// Lowers first.
			for (size_t flagIndex = 0; flagIndex < 26; ++flagIndex)
			{
				if (flags[flagIndex] == true)
					printf("%c", (int)('a' + flagIndex));
			}

			// Uppers next.
			for (size_t flagIndex = 26; flagIndex < 52; ++flagIndex)
			{
				if (flags[flagIndex] == true)
					printf("%c", (int)('A' + (flagIndex - 26)));
			}

			printf("\n");

		}
		else // Parameters.
		{
			char* argumentString = (char*)argument->argumentPtr;
			printf("PARAM : Index %d: %s\n", argument->argumentIndex, argumentString);
		}

		currentBranch = currentBranch->next;
	}

	return true;
}

/**
 * Parses the command line interface.
 * 
 * @param arena The memory arena to allocate into.
 * @param arguments The cliargs structure to fill out.
 * @param argc Argument count from main.
 * @param argv Argument string vector from main.
 * @param pproc The user-defined parse-checking procedure.
 * 
 * @returns True if the parse was successful, false if not.
 */
bool 
parseCLI(mem_arena* arena, cliargs* arguments, int argc, char** argv, parseproc pproc)
{

	// Set the invocation parameter.
	size_t invocationParamSize = strLength(argv[0])+1;
	arguments->invocationParameter = arena_push_array_zero(arena, char, invocationParamSize);
	strCopy(arguments->invocationParameter, invocationParamSize, argv[0], invocationParamSize);

	// Create an argument tree.
	node_trunk* argumentTree = createLinkedList(arena);
	arguments->argumentTree = argumentTree;
	
	// Create the argument list.
	uint32_t currentArgumentIndex = 0;
	for (size_t index = 1; index < argc; ++index)
	{

		// Determine the size of the string.
		size_t argumentStringLength = strLength(argv[index]);

		// Generate a new node.
		argument_properties* currentArgprops = pushNodeStruct(arena, argumentTree, argument_properties);

		// Set universal properties.
		currentArgprops->argumentIndex = currentArgumentIndex++;

		// Case 1: Tokens.
		if (argv[index][0] != '-')
		{

			// Create and store the string.
			char* argStringPtr = arena_push_array_zero(arena, char, argumentStringLength+1);
			strCopy(argStringPtr, argumentStringLength+1, argv[index], argumentStringLength+1);
			currentArgprops->argumentPtr = argStringPtr;
			currentArgprops->argumentSize = sizeof(char) * (argumentStringLength+1);

			// Set the type.
			currentArgprops->argumentType = ARGTYPE_TOKEN;

		}

		// Case 2: Flags.
		else if (strLength(argv[index]) > 1 &&
			argv[index][0] == '-' && argv[index][1] != '-')
		{

			// Create a structure which we can store the flags.
			uint64_t* flags = arena_push_zero(arena, 8);
			currentArgprops->argumentPtr = flags;

			// Set the type.
			currentArgprops->argumentType = ARGTYPE_FLAG;
			currentArgprops->argumentSize = sizeof(uint64);
			
			// Determine how many flags we need to compile.
			size_t flagCount = 1;
			size_t startingIndex = index;
			while (index+1 < argc)
			{
				// Look ahead.
				if (strLength(argv[index]) > 1 &&
					argv[index+1][0] == '-' && argv[index+1][1] != '-')
				{
					flagCount++;
					index++;
				}
				else
				{
					break; // Exit loop, no additional processing required.
				}
			}

			// Compile the flags into one structure.
			for (size_t flagIndex = 0; flagIndex < flagCount; ++flagIndex)
			{
				char* currentFlagString = argv[startingIndex + flagIndex];
				size_t flagCharacterIndex = 1;
				while (currentFlagString[flagCharacterIndex] != '\0')
				{

					// Ensure the character we are working is an alpha.
					char c = currentFlagString[flagCharacterIndex];
					if (charIsAlpha(c))
					{

						// Get the alpha-position. Bits 0-25 are lower, 26-51 upper.
						uint8 alphaPosition = 0;
						if (charIsLower(c))
							alphaPosition = charLowerAlphaOffset(c);
						else
							alphaPosition = charUpperAlphaOffset(c) + 26;

						// Once we have the position, set the bit to one.
						*flags = (*flags) | ((uint64)1 << alphaPosition);
					}

					flagCharacterIndex++;

				}
			}

		}

		// Case 3: Parameters. Probably.
		else
		{
			
			// Create and store the string.
			char* argStringPtr = arena_push_array_zero(arena, char, argumentStringLength+1);
			strSubstring(argStringPtr, argumentStringLength+1, argv[index], 2, -1);
			currentArgprops->argumentPtr = argStringPtr;
			currentArgprops->argumentSize = sizeof(char) * strLength(argStringPtr)+1;

			// Set the type.
			currentArgprops->argumentType = ARGTYPE_PARAMETER;

		}

	}

	reverseLinkedList(argumentTree);

	return pproc(arena, arguments);

}

int
main(int argc, char** argv)
{

	// Determine if there are file(s) to open. Multiple files may be provided.
	/*if (argc < 2)
	{
		printf("Error: Supply the file-name(s) to process.\n");
		exit(1);
	}*/

	// Initialize the application memory space we will need to run the application.
	// Since this is currently a single-threaded application, we will reserve 64MB
	// for the main thread. Text files aren't very large, so this should suffice.
	size_t virtual_heap_size = 0;
	void* virtual_heap_ptr = allocateHeap(1, MEGABYTES(64), &virtual_heap_size);
	mem_arena application_memory_heap = {0};
	arena_allocate(virtual_heap_ptr, virtual_heap_size, &application_memory_heap);

	cliargs cli_arguments = {0};
	if (!parseCLI(&application_memory_heap, &cli_arguments, argc, argv, &validateParsedCLI))
	{
		printf("Arguments are incorrect.\n");
		return -1;
	}
	else
	{
		printf("Arguments are correct.\n");
	}

	//processSourceFile(&application_memory_heap, argv[1]);

	// Calling virtual free isn't required since the OS will automatically reclaim
	// everything for us. Just exit.
	return 0;
}

