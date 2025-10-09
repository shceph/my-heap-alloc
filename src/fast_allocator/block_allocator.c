#include "block_allocator.h"

#include "../error.h"
#include "../os_allocator.h"

#include <assert.h>
#include <stdint.h>

inline static bool is_full(struct BlockAllocator *block_alloc) {
    return block_alloc->size - block_alloc->offset < FA_BLOCK_SIZE;
}

void *align_up_to_block_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t bias =
        ((intptr + (FA_BLOCK_SIZE - 1)) & ~(FA_BLOCK_SIZE - 1)) - intptr;

    return (char *)ptr + bias;
}

void *align_down_to_block_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t intptr_down_aligned = intptr & ~(FA_BLOCK_SIZE - 1);
    uintptr_t bias = intptr - intptr_down_aligned;
    return (char *)ptr - bias;
}

struct BlockAllocator block_alloc_init() {
    void *mem = os_alloc(DEFAULT_ALLOC_SIZE);

    if (mem == nullptr) {
        fa_print_errno("os_alloc() failed in block_alloc_init()");
        assert(false);
    }

    void *aligned_up_mem = align_up_to_block_size(mem);

    struct BlockAllocator block_alloc = {
        .mem = mem,
        .aligned_up_mem = aligned_up_mem,
        .size = DEFAULT_ALLOC_SIZE,
        .offset = 0,
        .next = nullptr,
    };

    return block_alloc;
}

void block_alloc_deinit(struct BlockAllocator *block_alloc) {
    while (block_alloc != nullptr) {
        int ret = os_free(block_alloc->mem, block_alloc->size);

        if (ret == OS_FREE_FAIL) {
            fa_print_errno("os_free() failed in block_alloc_deinit()");
            assert(false);
        }

        block_alloc = block_alloc->next;
    }
}

void *block_alloc_alloc(struct BlockAllocator *block_alloc) {
    if (is_full(block_alloc)) {
        (void)0;
    }

    assert(!is_full(block_alloc));

    void *ret = block_alloc->aligned_up_mem + block_alloc->offset;
    block_alloc->offset += FA_BLOCK_SIZE;
    return ret;
}
