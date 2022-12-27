#include <sourcery/string/string_utils.h>

void
strSubstring(char* buffer, size_t buffer_size, char* source_string, size_t start, int64 end)
{

	size_t character_index = start;
	size_t ending_index = end;
	size_t buffer_index = 0;
	
	// If string end is zero, then we want everything up to the end of the string.
	if (end == -1)
		end = strLength(source_string) + 1;

	// Copy over the contents, then null terminate.
	while (buffer_index < buffer_size && character_index < end)
		buffer[buffer_index++] = source_string[character_index++];
	buffer[buffer_index] = '\0';

}

size_t
strLength(const char* string)
{
	int n = 0;
	while (string[n] != '\0') n++;
	return n;
}

size_t
strLineLength(char* string, int offset)
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
strCopyLine(char* buffer, size_t buffer_size, char* string, int offset)
{

	// Ensure that line fits within the buffer.
	if (strLineLength(string, offset) + 1 > buffer_size)
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

char*
strCopy(char* dest, size_t dest_size, const char* source, size_t source_size)
{

	/**
	 * The copy process isn't particularly fast, especially for large strings.
	 * A memory copy operation would be a lot faster but for now this will work.
	 * 
	 * TODO(Chris):
	 * 		Implement a memory copy routine for string copy to help speed this
	 * 		operation up tremendously.
	 */

	// If the copy can't fit, return the original destination buffer.
	if (dest_size < source_size) return dest;

	// Copy over the contents of the source buffer.
	int c_index = 0;
	while (c_index < source_size)
	{
		dest[c_index] = source[c_index];
		c_index++;
	}

	return dest;

}

int
strSearchToken(const char* token, const char* string, int offset)
{

	// Search the string until we reach the null-terminator.
	int c_index = offset;
	while (string[c_index] != '\0')
	{

		// If the first character of the token matches the first character
		// of the string, then begin checking for the rest of the characters.
		int t_index = 0;
		if (string[c_index] == token[t_index])
		{

			// Assume the search is valid.
			bool valid = true;

			// Bump the indexes up.
			c_index++;
			t_index++;

			// As long as we're not at the end of either strings,
			// match for tokens.
			while (string[c_index] != '\0' && token[t_index] != '\0')
			{
				if (string[c_index] != token[t_index])
				{
					break;
				}
				c_index++;
				t_index++;
			}

			// The loop might fall out because we reached the end of the string
			// rather than the token. We can check for this by seeing if t_index
			// reached the string's length.
			if (t_index != strLength(token)) valid = false;

			// The position is simply c_index - t_index, which places the index
			// where the token begins.
			if (valid == true)
				return c_index-t_index;

		}

		c_index++;
	}

	return -1;

}

uint8
charLowerAlphaOffset(char c)
{
	return c - 0x61;
}

uint8
charUpperAlphaOffset(char c)
{
	return c - 0x41;
}

bool
charIsLower(char c)
{
	if ((c >= 0x61 && c <= 0x7A))
		return true;
	return false;
}

bool
charIsUpper(char c)
{
	if ((c >= 0x41 && c <= 0x5A))
		return true;
	return false;
}

bool
charIsAlpha(char c)
{
	if (charIsUpper(c) || charIsLower(c))
		return true;
	return false;
}
