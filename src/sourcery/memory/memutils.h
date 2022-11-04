#ifndef SOURCERY_MEMORY_MEMUTILS_H
#define SOURCERY_MEMORY_MEMUTILS_H
#include <sourcery/generics.h>

/**
 * Sets a region of memory to a specifiec character value.
 * 
 * @param buffer The region of memory to set.
 * @param buffer_size The size of the region.
 * @param value The value to set the region.
 */
void memory_set(void* buffer, size_t buffer_size, uint8 value);

#endif