#include "rtree.h"

#include "fixed_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static inline uint8_t get_byte_from_ptr(void *ptr, int byte_index) {
    union {
        void *ptr;
        uint8_t bytes[sizeof(void *)];
    } converter = {.ptr = ptr};

    return converter.bytes[sizeof(void *) - 1 - byte_index];
}

static inline struct RtreeNode *node_init(struct Rtree *rtree) {
    assert(rtree->node_allocator.unit_size == sizeof(struct RtreeNode));

    struct RtreeNode *node = fixed_alloc(&rtree->node_allocator);

    for (int i = 0; i <= UINT8_MAX; ++i) {
        node->entries[i].next = nullptr;
    }

    node->node_count = 0;

    return node;
}

static inline void node_deinit(struct Rtree *rtree, struct RtreeNode *node) {
    fixed_free(&rtree->node_allocator, node);
}

static inline size_t *leaf_alloc(struct Rtree *rtree) {
    return fixed_alloc(&rtree->leaf_allocator);
}

static inline void leaf_free(struct Rtree *rtree, size_t *leaf) {
    return fixed_free(&rtree->leaf_allocator, leaf);
}

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

void rtree_push_ptr(struct Rtree *rtree, void *ptr, size_t allocated_size) {
    if (!rtree->head) {
        rtree->head = node_init(rtree);
    }

    struct RtreeNode *curr_entry = rtree->head;

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        uint8_t byte = get_byte_from_ptr(ptr, i);

        if (!curr_entry->entries[byte].next) {
            curr_entry->entries[byte].next = node_init(rtree);
            ++curr_entry->node_count;
        }

        curr_entry = curr_entry->entries[byte].next;
    }

    uint8_t byte = get_byte_from_ptr(ptr, RTREE_DEPTH - 1);

    assert(!curr_entry->entries[byte].leaf);

    if (!curr_entry->entries[byte].leaf) {
        curr_entry->entries[byte].leaf = fixed_alloc(&rtree->leaf_allocator);
        ++curr_entry->node_count;
    }

    *curr_entry->entries[byte].leaf = allocated_size;
}

void rtree_remove_ptr(struct Rtree *rtree, void *ptr, size_t *out_stored_leaf) {
    struct RtreeNode *nodes[RTREE_DEPTH];

    nodes[0] = rtree->head;

    for (int i = 1; i < RTREE_DEPTH; ++i) {
        assert(nodes[i - 1]);

        uint8_t byte = get_byte_from_ptr(ptr, i - 1);

        nodes[i] = nodes[i - 1]->entries[byte].next;
    }

    uint8_t byte = get_byte_from_ptr(ptr, RTREE_DEPTH - 1);

    assert(nodes[RTREE_DEPTH - 1]->entries[byte].leaf);

    if (out_stored_leaf) {
        *out_stored_leaf = *nodes[RTREE_DEPTH - 1]->entries[byte].leaf;
    }

    leaf_free(rtree, nodes[RTREE_DEPTH - 1]->entries[byte].leaf);

    nodes[RTREE_DEPTH - 1]->entries[byte].leaf = nullptr;
    --nodes[RTREE_DEPTH - 1]->node_count;

    for (int i = RTREE_DEPTH - 1; i > 0; --i) {
        if (nodes[i]->node_count != 0) {
            continue;
        }

        uint8_t byte = get_byte_from_ptr(ptr, i - 1);

        node_deinit(rtree, nodes[i]);
        nodes[i - 1]->entries[byte].next = nullptr;
        --nodes[i - 1]->node_count;
    }

    if (rtree->head->node_count == 0) {
        node_deinit(rtree, rtree->head);
    }
}

bool rtree_contains(struct Rtree *rtree, void *ptr) {
    struct RtreeNode *node = rtree->head;

    if (!node) {
        return false;
    }

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        union RtreeEntry entry = node->entries[get_byte_from_ptr(ptr, i)];

        if (!entry.next) {
            return false;
        }

        node = entry.next;
    }

    return node->entries[get_byte_from_ptr(ptr, RTREE_DEPTH - 1)].leaf !=
           nullptr;
}

bool rtree_retrieve_size_if_contains(struct Rtree *rtree, void *ptr,
                                     size_t *out) {
    struct RtreeNode *node = rtree->head;

    if (!node) {
        return false;
    }

    for (int i = 0; i < RTREE_DEPTH - 1; ++i) {
        union RtreeEntry entry = node->entries[get_byte_from_ptr(ptr, i)];

        if (!entry.next) {
            return false;
        }

        node = entry.next;
    }

    size_t *leaf = node->entries[get_byte_from_ptr(ptr, RTREE_DEPTH - 1)].leaf;

    if (!leaf) {
        return false;
    }

    *out = *leaf;
    return true;
}
