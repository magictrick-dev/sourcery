#include <sourcery/generics.h>

/**
 * -----------------------------------------------------------------------------
 * Target System: Windows
 * -----------------------------------------------------------------------------
 */
#if defined(PLATFORM_WINDOWS)

// Thank you, Windows, very cool!
#pragma warning(suppress : 5105)
#	include <windows.h>
#pragma warning(disable : 5105)

#include <sourcery/filehandle.h>

bool
platform_open_file(fhandle* file_handle, const char* file_path)
{

	/**
	 * The file handle that the user passes in may contain an existing, in-use handle
	 * and therefore may cause a memory leak. We aren't going to check for this because
	 * it is the user's responsibility to ensure that this doesn't happen.
	 */

	bool open_status = false;

	// Opens a file.
	HANDLE os_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// If the file couldn't be opened, then set open_status.
	if (os_handle != INVALID_HANDLE_VALUE)
	{
		// If the file was opened, then we should set the file_handle struct
		// that the user passed in.
		file_handle->_platform_handle_size = sizeof(HANDLE);
		file_handle->_platform_handle_ptr = malloc(sizeof(HANDLE));

		// Set the platform file handle ptr to the OS's handle.
		*((HANDLE*)file_handle->_platform_handle_ptr) = os_handle;

		open_status = true; // Success.
	}

	return open_status;
}

bool
platform_close_file(fhandle* file_handle)
{
	bool close_status = false;

	// Attempt to close the handle.
	HANDLE* os_handle = (HANDLE*)file_handle->_platform_handle_ptr;
	BOOL os_close_status = CloseHandle(*os_handle);

	// Non-zero result is a successful close.
	if (os_close_status)
	{
		// Free handle from memory.
		free(file_handle->_platform_handle_ptr);
		
		// Zero the file_handle struct the user passed in.
		file_handle->_platform_handle_ptr = NULL;
		file_handle->_platform_handle_size = 0;

		close_status = true;
	}

	return close_status;
}

size_t
platform_filesize(fhandle* file_handle)
{

	// Retrieve the file size.
	HANDLE* os_handle = (HANDLE*)file_handle->_platform_handle_ptr;
	
	LARGE_INTEGER fsize = {0};
	bool filesize_status = GetFileSizeEx(*os_handle, &fsize);

	// Set the file size.
	size_t file_size = 0;
	if (filesize_status) file_size = fsize.QuadPart;

	// Return the file size.
	return file_size;

}

size_t
platform_read_file(fhandle* file_handle, void* buffer, size_t buffer_size)
{

	/**
	 * Since CreateFile is designed to accept, at most, a DWORD's worth of buffer
	 * at a time (4GB or the max unsigned 32-bit integer), the buffer_size must
	 * be under that size for this current implementation which simply casts it
	 * down to a DWORD.
	 * 
	 * This limitation can be worked around by reading in chunks... and generally
	 * speaking, loading files over the 4GB mark is generally not a scenario to be
	 * expected.
	 */

	assert(buffer_size < 0xFFFFFFFF);

	HANDLE* os_handle = (HANDLE*)file_handle->_platform_handle_ptr;

	// Read from the file.
	DWORD bytes_read = 0;;
	BOOL read_status = ReadFile(*os_handle, buffer, (DWORD)buffer_size, &bytes_read, NULL);

	return bytes_read;
}

#endif