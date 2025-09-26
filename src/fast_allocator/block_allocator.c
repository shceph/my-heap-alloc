#include "block_allocator.h"

#include "../os_allocator.h"

#include <assert.h>
#include <stdint.h>

inline static bool is_full(BlockAllocator *block_alloc) {
    return block_alloc->size - block_alloc->offset < FAST_ALLOC_BLOCK_SIZE;
}

void *align_up_to_block_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t bias = ((intptr + (FAST_ALLOC_BLOCK_SIZE - 1)) &
                      ~(FAST_ALLOC_BLOCK_SIZE - 1)) -
                     intptr;

    return (char *)ptr + bias;
}

void *align_down_to_block_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t intptr_down_aligned = intptr & ~(FAST_ALLOC_BLOCK_SIZE - 1);
    uintptr_t bias = intptr - intptr_down_aligned;
    return (char *)ptr - bias;
}

BlockAllocator block_alloc_init() {
    void *mem = os_alloc(DEFAULT_ALLOC_SIZE);

    assert(mem != nullptr);

    void *aligned_up_mem = align_up_to_block_size(mem);

    BlockAllocator block_alloc = {
        .mem = mem,
        .aligned_up_mem = aligned_up_mem,
        .size = DEFAULT_ALLOC_SIZE,
        .offset = 0,
        .next = nullptr,
    };

    return block_alloc;
}

void block_alloc_deinit(BlockAllocator *block_alloc) {
    while (block_alloc != nullptr) {
        int ret = os_free(block_alloc->mem, block_alloc->size);

        assert(ret == 0);

        block_alloc = block_alloc->next;
    }
}

void *block_alloc_alloc(BlockAllocator *block_alloc) {
    assert(!is_full(block_alloc));

    void *ret = block_alloc->aligned_up_mem + block_alloc->offset;
    block_alloc->offset += FAST_ALLOC_BLOCK_SIZE;
    return ret;
}
