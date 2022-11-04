#ifndef SOURCERY_GENERICS_H
#define SOURCERY_GENERICS_H
#include <stdint.h>
#include <stddef.h>

/**
 * Below are the list of explicit qualifiers, either functional in use or purely
 * cosmetic. The static qualifiers, for example, are functional and server to be
 * explicit about their use.
 */

/* For functions and variables that are to be contained within the translation unit. */
#define internal static

/* For variables within functions that are defined static and "persist" between each call. */
#define persist static


/**
 * Defines primitives types and inserts boolean functionality for use in C.
 * The base bool type uses a 32-bit integer type as its default. This is the
 * fastest version, but both the signed 32-bit and 64-bit versions are available.
 * The "true" and "false" primitives are defined under an anonymous enum with the
 * values 1 and 0, respectively.
 */

typedef uint8_t 	uint8;
typedef uint16_t 	uint16;
typedef uint32_t 	uint32;
typedef uint64_t 	uint64;

typedef int8_t 		int8;
typedef int16_t 	int16;
typedef int32_t 	int32;
typedef int64_t 	int64;

typedef float 		real32;
typedef double 		real64;

typedef int32_t 	bool;
typedef int32_t 	bool32;
typedef int64_t 	bool64;

enum { false, true };

/**
 * Below are the macros required to determine the platform. While it isn't possible
 * to detect every case, we should throw a pre-processor compilation error if the
 * OS isn't detected in some manner.
 */

#if defined(WIN32) || defined(_WIN32)
#	define PLATFORM_WINDOWS
#elif defined(__unix__) || defined (__linux__)
#	define PLATFORM_UNIX
#elif defined(__APPLE__)
#	define PLATFORM_MACOSX
#else
#	error "Unable to detect platform operating system target."
#endif


#endif