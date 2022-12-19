#ifndef SOURCERY_FILEHANDLE_H
#define SOURCERY_FILEHANDLE_H
#include <sourcery/generics.h>

#define PLATFORM_FILEOPEN_FAILED 0
#define PLATFORM_FILEOPEN_SUCCESS 1

#define PLATFORM_FILECONTEXT_NEW 1
#define PLATFORM_FILECONTEXT_EXISTING 2
#define PLATFORM_FILECONTEXT_ALWAYS 3

#define PLATFORM_FILEMODE_READONLY 1
#define PLATFORM_FILEMODE_APPEND 2
#define PLATFORM_FILEMODE_TRUNCATE 3

/**
 * Represents the OS's filehandle with additional information useful for file
 * operations. Since Win32 and UNIX both represent their handles as 64-bit identifiers,
 * we can reliably cast them to a 64-bit type and then cast them back for platform
 * specific operations.
 */
typedef struct filehandle
{
	size_t platform_handle_ptr;
	size_t platform_handle_size;

	uint32_t context;
	uint32_t mode;

	size_t file_size;
	size_t read_ptr;
	size_t write_ptr;
} filehandle;

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * Platform Specific Definitions
 * ---------------------------------------------------------------------------------------------------------------------
 * You will find the platform-specific implementations in
 * the platform/[target-os]/[target-os]_filehandle.c
 */

/**
 * Attempts to open a file and, if successful, will set the filehandle struct with
 * the appropriate information. The file context determines how the file should be
 * opened, and the file mode determines how the file should be utilized.
 * 
 * @param fh A pointer to a filehandle struct to be filled out.
 * @param file_path The file to open.
 * @param file_context How the file should be opened.
 * @param file_mode The mode which determines how the file should be handled.
 * 
 * @returns True if the open was successful, false otherwise. On success, the filehandle
 * struct will be properly updated with the necessary information.
 */
bool
platformOpenFile(filehandle* fh, const char* file_path, uint32_t file_context, uint32_t file_mode);

/**
 * Closes a filehandle.
 * 
 * @param fh The filehandle to close.
 */
void
platformCloseFile(filehandle* fh);

/**
 * Reads buffer_size into buffer from the filehandle. The read pointer begins at
 * the start of the file always. Each successive read will then adjust read pointer
 * to the next available location or EOF.
 * 
 * @param fh The filehandle to read from.
 * @param buffer The buffer to which the contents are read into.
 * @param buffer_size The size, in bytes, of how much to read.
 * 
 * @returns The number of bytes that were read.
 */
size_t
platformReadFile(filehandle* fh, void* buffer, size_t buffer_size);

/**
 * Writes buffer_size from buffer into the filehandle. The write pointer may
 * begin at the start of the file if the filemode is truncate or the file is new,
 * otherwise the write pointer will begin at the end of the file always.
 * 
 * @param fh The filehandle to write to.
 * @param buffer The buffer to read from.
 * @param buffer_size The size, in bytes, of how much to write.
 * 
 * @returns The number of bytes written.
 */
size_t
platformWriteFile(filehandle* fh, void* buffer, size_t buffer_size);



/**
 * Attempts to create a directory using the given file path.
 * 
 * @param file_path The file path to attempt to create.
 * 
 * @returns True if the directory was created, false if not.
 */
bool
platformCreateDirectory(const char* file_path);

#endif