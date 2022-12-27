#ifndef SOURCERY_MAIN_H
#define SOURCERY_MAIN_H
#include <sourcery/generics.h>
#include <sourcery/memory/alloc.h>
#include <sourcery/structures/node_trunk.h>

/**
 * -----------------------------------------------------------------------------
 * Line Source & Enumerations
 * -----------------------------------------------------------------------------
 */

#define DIRECTIVE_NONE 				0
#define DIRECTIVE_UNDEFINED 		1
#define DIRECTIVE_MAKEFILE 			2
#define DIRECTIVE_MAKEDIR 			3
#define DIRECTIVE_COMMAND 			4
#define DIRECTIVE_HEADER 			5
#define DIRECTIVE_VARIABLE 			6
#define DIRECTIVE_MACROINLINE 		7
#define DIRECTIVE_MACROFUNCTION		8

typedef struct line_source
{
	char* 	stringPtr;
	size_t 	stringLength;

	uint32 	lineNumber;
	uint32 	lineDirectiveType;
} line_source;

/**
 * -----------------------------------------------------------------------------
 * CLI Parsing, Arguments, etc. & Enumerations
 * -----------------------------------------------------------------------------
 */

#define ARGTYPE_TOKEN 		0
#define ARGTYPE_FLAG 		1
#define ARGTYPE_PARAMETER 	2

/**
 * A bitfield union which maps a uint64 numeric to
 * its individual bit components.
 */
typedef union argflag_bitfield
{
	struct
	{
		// Lower-case alphas.
		bool a : 1;
		bool b : 1;
		bool c : 1;
		bool d : 1;
		bool e : 1;
		bool f : 1;
		bool g : 1;
		bool h : 1;
		bool i : 1;
		bool j : 1;
		bool k : 1;
		bool l : 1;
		bool m : 1;
		bool n : 1;
		bool o : 1;
		bool p : 1;
		bool q : 1;
		bool r : 1;
		bool s : 1;
		bool t : 1;
		bool u : 1;
		bool v : 1;
		bool w : 1;
		bool x : 1;
		bool y : 1;
		bool z : 1;

		// Upper-case alphas.
		bool A : 1;
		bool B : 1;
		bool C : 1;
		bool D : 1;
		bool E : 1;
		bool F : 1;
		bool G : 1;
		bool H : 1;
		bool I : 1;
		bool J : 1;
		bool K : 1;
		bool L : 1;
		bool M : 1;
		bool N : 1;
		bool O : 1;
		bool P : 1;
		bool Q : 1;
		bool R : 1;
		bool S : 1;
		bool T : 1;
		bool U : 1;
		bool V : 1;
		bool W : 1;
		bool X : 1;
		bool Y : 1;
		bool Z : 1;
	};

	uint64 __field;
} argflag_bitfield;

/**
 * Defines each argument passed in from the CLI interface.
 * 
 * Tokens are not flags or parameters and therefore is treated as raw data. They
 * are stored as a plain c-string.
 * 
 * Flags may be combined by the user, such as '-rux', or may be passed individually.
 * Since flags may be referenced in parameters like '--config -r myconf.txt', we must respect
 * their positioning. However, we can assume that a set of flags that appear in series
 * are referring to the same parameter and therefore can be joined during parse-time.
 * Flags are stored as 64-bit integers, bits 0-25 are lowercase flags, bits 26-51 are
 * uppercase flags.
 * 
 * Parameters are additional behaviors which may or may not refer to proceeding
 * tokens and flags. This behavior is largely dependent on the application, so validation
 * must be performed in the parseproc defined by the user.
 */
typedef struct argument_properties
{
	void* 		argumentPtr;
	size_t 		argumentSize;

	uint32_t 	argumentType;
	uint32_t 	argumentIndex;
} argument_properties;

/**
 * The serialized command line arguments for use by the application. The list of
 * arguments may not be a 1:1 representation of the original argument count. The
 * invocation parameter is a string containing the calling directory of the application
 * as set by C standard and therefore always exists.
 */
typedef struct cliargs
{
	node_trunk* 	argumentTree;
	char* 			invocationParameter;
} cliargs;

/**
 * A defined procedure to determine if the CLI are formatted to
 * the specifications of the application. This procedure should return
 * true if arguments are formatted to the specification, false if not.
 */
typedef bool (*parseproc)(mem_arena*, cliargs*);



#endif