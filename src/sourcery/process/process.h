/**
 * Processes need to be created using OS-specific calls and therefore these interfaces
 * must be defined in their corresponding OS definitions.
 */
#ifndef SOURCERY_MEMORY_ALLOC_H
#define SOURCERY_MEMORY_ALLOC_H
#include <sourcery/generics.h>

/**
 * Creates a new process using the provided
 * 
 * @param invoc The command to invoke on the CLI.
 * 
 * @returns The status of the running function, non-zero values indicate failure.
 */
int platformRunCLIProcess(char* invoc);

#endif