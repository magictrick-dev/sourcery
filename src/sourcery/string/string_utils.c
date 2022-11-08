#include <sourcery/string/string_utils.h>

int
string_length(const char* string)
{
	int n = 0;
	while (string[n] != '\0') n++;
	return n;
}

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