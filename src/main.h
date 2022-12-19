#ifndef SOURCERY_MAIN_H
#define SOURCERY_MAIN_H
#include <sourcery/generics.h>

#define DIRECTIVE_NONE 		0
#define DIRECTIVE_UNDEFINED 1
#define DIRECTIVE_MAKEFILE 	2
#define DIRECTIVE_MAKEDIR 	3
#define DIRECTIVE_COMMAND 	4

typedef struct line_source
{

	char* 	stringPtr;
	size_t 	stringLength;

	uint32 	lineNumber;
	uint32 	lineDirectiveType;

} line_source;

#endif