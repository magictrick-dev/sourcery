#include <sourcery/structures/node_trunk.h>

inline node_trunk*
create_node_tree(mem_arena* arena)
{

    // Push the trunk on the arena.
    node_trunk* trunk = arena_push_struct(arena, node_trunk);
    trunk->count = 0;
    trunk->next = NULL;

    return trunk;

}

inline node_branch*
push_node(mem_arena* arena, node_trunk* trunk, size_t branch_size)
{

    // Push the node, then the branch.
    node_branch* node = arena_push_struct(arena, node_branch);
    void* branch = arena_push(arena, branch_size);

    // Initialize the node.
    node->branch = branch;
    node->next = NULL;
    node->trunk = trunk;

    // Place the node at the front list.
    node_branch* head = trunk->next;
    trunk->next = node;
    node->next = head;

    // Update the count.
    trunk->count++;

    return node;

}
