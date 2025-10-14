#include "block_allocator.h"

#include "stack_definition.h"

#include "../error.h"
#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

STACK_DEFINE(BaCacheElem, BaCacheSizeType, BaCache)

#define GET_NUM_OF_ELEMS(BLOCK_ALLOC_VAR) ((BLOCK_ALLOC_VAR).cache.capacity)

inline static bool is_full(struct BlockAllocator *block_alloc) {
    size_t block_alloc_buff_size =
        (GET_NUM_OF_ELEMS(*block_alloc)) * FA_BLOCK_SIZE;
    return block_alloc->offset >= block_alloc_buff_size - FA_BLOCK_SIZE;
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

struct BlockAllocator balloc_init() {
    // num_of_elems * BLOCK_SIZE + num_of_elems * sizeof(CacheElem) = buff_size
    // num_of_elems * (BLOCK_SIZE + sizeof(CacheElem)) = buff_size
    // num_of_elems = buff_size / (BLOCK_SIZE + sizeof(CacheElem))

    void *mem = os_alloc(DEFAULT_ALLOC_SIZE);

    if (!mem) {
        fa_print_errno("os_alloc() failed in block_alloc_init()");
        assert(false);
    }

    void *aligned_up_mem = align_up_to_block_size(mem);

    size_t unused_mem_size = (char *)aligned_up_mem - (char *)mem;
    size_t buff_size = DEFAULT_ALLOC_SIZE - unused_mem_size;
    size_t num_of_elems = buff_size / (FA_BLOCK_SIZE + sizeof(BaCacheElem));

    void *cache_mem = (char *)aligned_up_mem + (num_of_elems * FA_BLOCK_SIZE);

    struct BlockAllocator block_alloc = {
        .os_allocated_mem = mem,
        .aligned_up_mem = aligned_up_mem,
        .os_allocated_size = DEFAULT_ALLOC_SIZE,
        .offset = 0,
        .cache = BaCache_init(cache_mem, num_of_elems),
        .next = nullptr,
    };

    return block_alloc;
}

void balloc_deinit(struct BlockAllocator *block_alloc) {
    while (block_alloc != nullptr) {
        int ret = os_free(block_alloc->os_allocated_mem,
                          block_alloc->os_allocated_size);

        if (ret == OS_FREE_FAIL) {
            fa_print_errno("os_free() failed in block_alloc_deinit()");
            assert(false);
        }

        block_alloc = block_alloc->next;
    }
}

void *balloc_alloc(struct BlockAllocator *block_alloc) {
    assert(!is_full(block_alloc));

    void *ret;
    enum StackError err = BaCache_try_pop(&block_alloc->cache, &ret);

    if (err == STACK_OK) {
        return ret;
    }

    ret = block_alloc->aligned_up_mem + block_alloc->offset;
    block_alloc->offset += FA_BLOCK_SIZE;
    return ret;
}

void balloc_free(struct BlockAllocator *block_alloc, void *block) {
    enum StackError err = BaCache_try_push(&block_alloc->cache, block);
    (void)err;
    assert(err != STACK_FULL);
}
