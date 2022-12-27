#ifndef SOURCERY_STRING_UTILS_H
#define SOURCERY_STRING_UTILS_H
#include <sourcery/generics.h>

/**
 * Copies a substring from a string into the provided buffer. The
 * ending value should be the index of the element to stop at, not
 * the index of the last character to copy. A value of -1 for end
 * indicates that everything from the starting value to the last character
 * should be copied.
 * 
 * @param buffer The buffer to copy the source substring into.
 * @param buffer_size The size of the buffer.
 * @param source_string The source string to substring from.
 * @param start The starting index of the substring.
 * @param end The ending index of the substring.
 */
void
strSubstring(char* buffer, size_t buffer_size, char* source_string,
	size_t start, int64 end);

/**
 * Returns the length of a string.
 * 
 * @param string The string the get the length of.
 * 
 * @returns The length of the length string.
 */
size_t strLength(const char* string);

/**
 * Gets the size of the line starting at a given offset, in bytes, up
 * to the next line.
 * 
 * @param string The string to get the size of the line.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The size, in bytes, of the line.
 */
size_t strLineLength(char* string, int offset);

/**
 * Retrieves a line from a provided string and copies it to the buffer starting
 * from an offset. The value returned is the offset to the next line otherwise
 * returns -1.
 * 
 * @param buffer The buffer to copy the line into.
 * @param buffer_size The size of the buffer.
 * @param source_string The string to pull the line from.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The offset to the next line otherwise -1.
 */
int strCopyLine(char* buffer, size_t buffer_size, char* source_string, int offset);

/**
 * Searches for the first token within a string.
 * 
 * @param token The token to search for.
 * @param string The string to search in.
 * @param offset The offset to which to begin searching for a token.
 * 
 * @returns The starting index position of the token, or -1 if the
 * token was not found within the string.
 */
int
strSearchToken(const char* token, const char* string, int offset);

/**
 * Copies a string from source into dest.
 * 
 * @param dest The destination buffer to write to.
 * @param dest_size The size of the destination buffer in bytes.
 * @param source The source buffer to copy from.
 * @param source_size The size of the source buffer.
 * 
 * @returns The destination buffer.
 */
char* strCopy(char* dest, size_t dest_size, const char* source, size_t source_size);

/**
 * Determines the alphabetical position of the character (0-25) of a lower-case
 * alpha character.
 * 
 * @param c The character to check.
 * 
 * @returns The zero-index of the character in the alphabet.
 */
uint8 charLowerAlphaOffset(char c);

/**
 * Determines the alphabetical position of the character (0-25) of a upper-case
 * alpha character.
 * 
 * @param c The character to check.
 * 
 * @returns The zero-index of the character in the alphabet.
 */
uint8 charUpperAlphaOffset(char c);

/**
 * Determines if a character is upper-case.
 * 
 * @param c The character to check.
 * 
 * @returns True if the character is upper-case alpha, false otherwise.
 */
bool charIsUpper(char c);

/**
 * Determines if a character is lower-case.
 * 
 * @param c The character to check.
 * 
 * @returns True if the character is lower-case alpha, false otherwise.
 */
bool charIsLower(char c);

/**
 * Determines if a character is an alpha-chracter.
 * 
 * @param c The character to check.
 * 
 * @returns True if the character is an alpha-character, false otherwise.
 */
bool charIsAlpha(char c);

#endif