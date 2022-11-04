#ifndef SOURCERY_FILEHANDLE_H
#define SOURCERY_FILEHANDLE_H
#include <sourcery/generics.h>

/**
 * A user-facing file handle to the platform's file handle used in file I/O.
 */
typedef struct fhandle
{
	void* _platform_handle_ptr;
	size_t _platform_handle_size;
} fhandle;

/**
 * -----------------------------------------------------------------------------
 * Platform Specific Definitions
 * -----------------------------------------------------------------------------
 * You will find the platform-specific implementations in
 * the platform/[target-os]/[target-os]_filehandle.c
 */

/**
 * Opens a file using the platform's file I/O API.
 * 
 * @param file_handle A pointer to a fhandle struct to modify.
 * @param file_path The path to the file to open.
 * 
 * @returns True if the file was successfully opened, false if not.
 */
bool platform_open_file(fhandle* file_handle, const char* file_path);

/**
 * Closes a file using the platform's file I/O API.
 * 
 * @param file_handle A pointer to the fhandle struct to modify.
 * 
 * @returns True if the file was successfully closed, false if not.
 */
bool platform_close_file(fhandle* file_handle);

/**
 * Gets the size of the file using the platform's file I/O API.
 * 
 * @param file_handle A pointer to the fhandle struct.
 * 
 * @returns The size of the file, in bytes.
 */
size_t platform_filesize(fhandle* file_handle);

/**
 * Reads the contents of a file up to buffer size using the platform's file I/O API.
 * 
 * @param file_handle A pointer to the fhandle struct.
 * @param buffer The buffer to read the file contents to.
 * @param buffer_size The size of the buffer.
 * 
 * @returns The numbers of bytes that were read into the buffer.
 */
size_t platform_read_file(fhandle* file_handle, void* buffer, size_t buffer_size);

#endif