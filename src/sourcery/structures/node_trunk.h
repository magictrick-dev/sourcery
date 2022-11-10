#ifndef SOURCERY_STRUCTURES_NODE_TRUNK
#define SOURCERY_STRUCTURES_NODE_TRUNK
#include <sourcery/memory/alloc.h>

/**
 * A node trunk represents the underlying the control structure for the linked
 * list. It contains the count and a reference to the first branch.
 */
typedef struct node_trunk
{
	size_t count;
	node_branch* next;
} node_trunk;

/**
 * A node branch is placed within the structure of the object. The branch
 * refers back to the starting address of the object, the next node in the
 * list and a reference back to the trunk.
 */
typedef struct node_branch
{
	void* next;
	void* branch;
	node_trunk* trunk; // Reference back to the trunk.
} node_branch;

/**
 * Creates a node tree and places it within the provided arena.
 * 
 * @param arena The arena to place the node tree on.
 * 
 * @returns A pointer to the node_trunk control structure that was
 * created on the provided memory arena.
 */
inline node_trunk*
create_node_tree(mem_arena* arena);

/**
 * Since the base function, push_node, returns the branch, a macro will be more
 * useful. The push_node_struct will automatically cast down to the struct that
 * was used to create the branch.
 */
#define push_node_struct(arena, trunk, type) (type*)(push_node(arena, trunk, sizeof(type))->branch)

/**
 * Pushes a branch at the top of the tree. Returns a pointer back to the branch
 * that was created. The branch node is first allocated, followed by n-bytes as
 * provided by branch_size.
 * 
 * @param arena The arena to allocate the branch on.
 * @param trunk The trunk to append the next branch node to.
 * @param branch_size The size of the branch, in bytes, to allocate.
 */
inline node_branch*
push_node(mem_arena* arena, node_trunk* trunk, size_t branch_size);

#endif