#include <stdio.h>
#include <sourcery/filehandle.h>
#include <sourcery/memory/memutils.h>

int
main(int argc, char** argv)
{

	printf("Hello, world\n");

	// Attempt to open the file.
	fhandle test_handle = {0};
	if (!platform_open_file(&test_handle, "./testfile.txt"))
	{
		printf("The file couldn't be opened.\n");
		exit(1);
	}

	// Get the size of the file.
	size_t file_size = platform_filesize(&test_handle);
	printf("The size of the file is: %d\n", (int)file_size);

	// Get the contents of the file.
	char string_buffer[512];
	memory_set(string_buffer, 512, 0x00);
	
	size_t bytes_read = platform_read_file(&test_handle, (void*)string_buffer, 512);
	printf("The file was opened. Here is the contents:\n%s\n", string_buffer);

	// Attempt to close the file.
	if (!platform_close_file(&test_handle))
	{
		printf("Unable to close the file.\n");
		exit(1);
	}

	return 0;
}

