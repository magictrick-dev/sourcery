#include <sourcery/generics.h>
/**
 * -----------------------------------------------------------------------------
 * Target System: Windows
 * -----------------------------------------------------------------------------
 */
#if defined(PLATFORM_WINDOWS)

// Thank you Windows, very cool!
#pragma warning(suppress : 5105)
#	include <windows.h>
#pragma warning(disable : 5105)

#include <sourcery/filehandle.h>

int32
platformOpenFile(filehandle* fh, const char* file_path, uint32_t file_context, uint32_t file_mode)
{

	// Initialize the filehandle struct with the context and mode.
	fh->context = file_context;
	fh->mode = file_mode;

	// Determine the access mode for the file.
	DWORD desired_access = GENERIC_READ;
	if (file_mode != PLATFORM_FILEMODE_READONLY)
		desired_access |= GENERIC_WRITE;

	// Determine how the file should be handled.
	DWORD creation_disposition = 0;
	if (file_context == PLATFORM_FILECONTEXT_NEW)
		creation_disposition = CREATE_NEW;
	else if (file_context == PLATFORM_FILECONTEXT_EXISTING)
		creation_disposition = OPEN_EXISTING;
	else if (file_context == PLATFORM_FILECONTEXT_ALWAYS)
		creation_disposition = OPEN_ALWAYS;
	else if (file_mode == PLATFORM_FILEMODE_TRUNCATE) // NOTE(Chris): Special condition.
		creation_disposition = CREATE_ALWAYS;
	else
		creation_disposition = OPEN_ALWAYS;

	// Attempt to open the file requested. The function will return PLATFORM_FILEOPEN_FAILED
	// if the open failed. Otherwise, set the filehandle abstraction pointer.
	HANDLE win_handle = CreateFileA(file_path, desired_access, FILE_SHARE_READ, NULL, creation_disposition,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (win_handle == INVALID_HANDLE_VALUE)
		return PLATFORM_FILEOPEN_FAILED;

	// Set platform handle pointer and size.
	fh->platform_handle_size = sizeof(HANDLE);
	fh->platform_handle_ptr = (size_t)win_handle;

	// Once we have the file opened, we should capture the file size.
	LARGE_INTEGER file_size = {0};
	assert(GetFileSizeEx(win_handle, &file_size) != 0); // This should never happen.
	fh->file_size = (size_t)file_size.QuadPart;

	// Set the read and write pointers to their respective locations.
	fh->read_ptr = 0;
	fh->write_ptr = (file_mode == PLATFORM_FILEMODE_APPEND) ? file_size.QuadPart : 0;

	// Now that the filehandle is filled out, we can return the respective values.
	return PLATFORM_FILEOPEN_SUCCESS;

}

void
platformCloseFile(filehandle* fh)
{

	if (fh->platform_handle_ptr != 0)
	{
		CloseHandle((HANDLE)fh->platform_handle_ptr);
		fh->platform_handle_ptr = 0;
		fh->platform_handle_size = 0;
	}

}

size_t
platformReadFile(filehandle* fh, void* buffer, size_t buffer_size)
{

	// First, we must set the read pointer to the last known read position.
	LARGE_INTEGER offset_position = {0};
	offset_position.QuadPart = (LONGLONG)fh->read_ptr;
	LARGE_INTEGER ending_position = {0};
	BOOL setfp_status = SetFilePointerEx((HANDLE)fh->platform_handle_ptr, offset_position,
		&ending_position, FILE_BEGIN);
	fh->read_ptr = (size_t)ending_position.QuadPart; // Ensures that the last position is correct.
	assert(setfp_status != 0);

	// Continually read into the buffer until we have reached buffer size or reach EOF.
	DWORD total_read = 0;
	BOOL read_status = TRUE;
	while (read_status && total_read < buffer_size)
	{

		// Read in the bytes into the buffer.
		DWORD bytes_read = 0;
		read_status = ReadFile((HANDLE)fh->platform_handle_ptr,
			buffer, buffer_size - total_read, &bytes_read, NULL);
		total_read += bytes_read;

		// If we read zero bytes, we are probably EOF.
		if (bytes_read == 0)
			break;
	}

	// Update the read position.
	fh->read_ptr += total_read;

	// Return the number of bytes read.
	return total_read;

}

size_t
platformWriteFile(filehandle* fh, void* buffer, size_t buffer_size)
{

	// We need to ensure that our write pointer is at the last known right position.
	LARGE_INTEGER offset_position = {0};
	offset_position.QuadPart = (LONGLONG)fh->write_ptr;
	LARGE_INTEGER ending_position = {0};
	BOOL setfp_status = SetFilePointerEx((HANDLE)fh->platform_handle_ptr, offset_position,
		&ending_position, FILE_BEGIN);
	fh->write_ptr = (size_t)ending_position.QuadPart; // Ensures that the last position is correct.
	assert(setfp_status != 0);

	// Write all the bytes from the buffer.
	DWORD total_written = 0;
	BOOL write_status = TRUE;
	while (write_status && total_written < buffer_size)
	{

		// Write the bytes into the file..
		DWORD bytes_written = 0;
		write_status = WriteFile((HANDLE)fh->platform_handle_ptr,
			buffer, buffer_size - total_written, &bytes_written, NULL);
		total_written += bytes_written;

	}

	// Update the write position.
	fh->write_ptr += total_written;

	// Get the new filesize.
	LARGE_INTEGER file_size = {0};
	assert(GetFileSizeEx((HANDLE)fh->platform_handle_ptr, &file_size) != 0); // This should never happen.
	fh->file_size = (size_t)file_size.QuadPart;

	// Return the number of bytes written.
	return total_written;

}

int
platformCreateDirectory(const char* file_path)
{

	// Short and sweet.
	// TODO(Chris): We need to extend error cases such as directory exists / unable to create.
	BOOL dir_status = CreateDirectoryA(file_path, NULL);
	return (dir_status != 0);

}

#endif