#include "rtree.h"

#include "fixed_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// TODO: Cleanup, add a function that prints out rtree in the test.

struct Rtree rtree_init() {
    return (struct Rtree){
        .head = nullptr,
        .node_allocator = fixed_alloc_init(sizeof(struct RtreeNode)),
        .leaf_allocator = fixed_alloc_init(sizeof(size_t)),
    };
}

void rtree_deinit(struct Rtree *rtree) {
    fixed_alloc_deinit(&rtree->node_allocator);
}

struct RtreeNode *node_init(struct FixedAllocator *node_allocator) {
    assert(node_allocator->unit_size == sizeof(struct RtreeNode));

    struct RtreeNode *node = fixed_alloc(node_allocator);

    for (int i = 0; i <= UINT8_MAX; ++i) {
        node->entries[i].next = nullptr;
    }

    node->node_count = 0;

    return node;
}

void node_deinit(struct Rtree *rtree, struct RtreeNode *node) {
    fixed_free(&rtree->node_allocator, node);
}

void rtree_push_ptr(struct Rtree *rtree, void *ptr, size_t allocated_size) {
    union Converter converter = {
        .addr = ptr,
    };

    if (!rtree->head) {
        rtree->head = node_init(&rtree->node_allocator);
    }

    struct RtreeNode *curr_entry = rtree->head;

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        uint8_t byte = converter.bytes[i];

        if (!curr_entry->entries[byte].next) {
            curr_entry->entries[byte].next = node_init(&rtree->node_allocator);
            ++curr_entry->node_count;
        }

        curr_entry = curr_entry->entries[byte].next;
    }

    uint8_t byte = converter.bytes[RTREE_DEPTH - 1];

    assert(curr_entry->entries[byte].leaf == nullptr);

    if (!curr_entry->entries[byte].leaf) {
        curr_entry->entries[byte].leaf = fixed_alloc(&rtree->leaf_allocator);
        ++curr_entry->node_count;
    }

    *curr_entry->entries[byte].leaf = allocated_size;
}

void rtree_remove_ptr(struct Rtree *rtree, void *ptr) {
    union Converter converter = {
        .addr = ptr,
    };

    struct RtreeNode *nodes[RTREE_DEPTH];

    nodes[0] = rtree->head;

    for (int i = 1; i < RTREE_DEPTH; ++i) {
        assert(nodes[i - 1]);

        uint8_t byte = converter.bytes[i - 1];
        nodes[i] = nodes[i - 1]->entries[byte].next;
    }

    uint8_t byte = converter.bytes[RTREE_DEPTH - 1];

    assert(nodes[RTREE_DEPTH - 1]->entries[byte].leaf);

    fixed_free(&rtree->leaf_allocator,
               nodes[RTREE_DEPTH - 1]->entries[byte].leaf);

    nodes[RTREE_DEPTH - 1]->entries[byte].leaf = nullptr;
    --nodes[RTREE_DEPTH - 1]->node_count;

    for (int i = RTREE_DEPTH - 1; i > 0; --i) {
        if (nodes[i]->node_count != 0) {
            continue;
        }

        uint8_t byte = converter.bytes[i - 1];

        node_deinit(rtree, nodes[i]);
        nodes[i - 1]->entries[byte].next = nullptr;
        --nodes[i - 1]->node_count;
    }

    if (rtree->head->node_count == 0) {
        node_deinit(rtree, rtree->head);
    }
}

bool rtree_contains(struct Rtree *rtree, void *ptr) {
    union Converter converter = {
        .addr = ptr,
    };

    struct RtreeNode *node = rtree->head;

    if (!node) {
        return false;
    }

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        union RtreeEntry entry = node->entries[converter.bytes[i]];

        if (!entry.next) {
            return false;
        }

        node = entry.next;
    }

    return node->entries[converter.bytes[RTREE_DEPTH - 1]].leaf != nullptr;
}

bool rtree_retrieve_size_if_contains(struct Rtree *rtree, void *ptr,
                                     size_t *out) {
    union Converter converter = {
        .addr = ptr,
    };

    struct RtreeNode *node = rtree->head;

    if (!node) {
        return false;
    }

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        union RtreeEntry entry = node->entries[converter.bytes[i]];

        if (!entry.next) {
            return false;
        }

        node = entry.next;
    }

    size_t *leaf = node->entries[converter.bytes[RTREE_DEPTH - 1]].leaf;

    if (!leaf) {
        return false;
    }

    *out = *leaf;
    return true;
}
