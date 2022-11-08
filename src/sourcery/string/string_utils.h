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
size_t get_line_size(char*, int);

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
int32 get_line_string(char*, size_t, char*, int);

#endif