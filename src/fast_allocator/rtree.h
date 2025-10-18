#ifndef RTREE_H
#define RTREE_H

#include "fixed_alloc.h"

#include <stddef.h>
#include <stdint.h>

constexpr size_t RTREE_DEPTH = sizeof(void *);

union Converter {
    void *addr;
    uint8_t bytes[sizeof(void *)];
};

union RtreeEntry {
    struct RtreeNode *next;
    size_t *leaf;
};

struct RtreeNode {
    size_t node_count;
    union RtreeEntry entries[UINT8_MAX + 1];
};

struct Rtree {
    struct RtreeNode *head;
    struct FixedAllocator node_allocator;
    struct FixedAllocator leaf_allocator;
};

struct Rtree rtree_init();
void rtree_deinit(struct Rtree *rtree);
struct RtreeNode *node_init(struct FixedAllocator *node_allocator);
void rtree_push_ptr(struct Rtree *rtree, void *ptr, size_t allocated_size);
void rtree_remove_ptr(struct Rtree *rtree, void *ptr);
bool rtree_contains(struct Rtree *rtree, void *ptr);
bool rtree_retrieve_size_if_contains(struct Rtree *rtree, void *ptr,
                                     size_t *out);

#endif // RTREE_H
