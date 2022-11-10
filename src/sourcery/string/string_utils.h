#ifndef SOURCERY_STRING_UTILS_H
#define SOURCERY_STRING_UTILS_H
#include <sourcery/generics.h>

/**
 * Returns the length of a string.
 * 
 * @param string The string the get the length of.
 * 
 * @returns The length of the length string.
 */
string_length(const char*);

/**
 * Gets the size of the line starting at a given offset, in bytes, up
 * to the next line.
 * 
 * @param string The string to get the size of the line.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The size, in bytes, of the line.
 */
size_t string_get_line_length(char*, int);

/**
 * Retrieves a line from a provided string and copies it to the buffer starting
 * from an offset. The value returned is the offset to the next line otherwise
 * returns -1.
 * 
 * @param buffer The buffer to copy the line into.
 * @param buffer_size The size of the buffer.
 * @param string The string to pull the line from.
 * @param offset The offset into the string to begin looking for a line.
 * 
 * @returns The offset to the next line otherwise -1.
 */
int string_get_line(char*, size_t, char*, int);

/**
 * Searches for a matching token among a list of tokens within a string.
 * 
 * @param tokens A list of tokens to search with.
 * @param list_size The size of the tokens list.
 * @param string The string to search within.
 * @param offset The offset to start searching within.
 */
int
string_find_token_from_list(char** tokens, int list_size, const char* string, int offset);

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
string_find_token(const char* token, const char* string, int offset);

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
char* string_copy(char* dest, size_t dest_size, const char* source, size_t source_size);

#endif