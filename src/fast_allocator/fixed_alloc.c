#include "fixed_alloc.h"

#include "stack_definition.h"

#include "../error.h"
#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

STACK_DEFINE(FixedAllocCacheElem, FixedAllocCacheSizeType, FixedAllocCache)

#define GET_NUM_OF_ELEMS(FIXED_ALLOC_VAR) ((FIXED_ALLOC_VAR).cache.capacity)

inline static bool is_full(struct FixedAllocator *fixed_alloc) {
    size_t fixed_alloc_buff_size =
        (GET_NUM_OF_ELEMS(*fixed_alloc)) * fixed_alloc->unit_size;

    return fixed_alloc->offset >=
           fixed_alloc_buff_size - fixed_alloc->unit_size;
}

inline static void *align_up_to_unit_size(const void *ptr, size_t unit_size) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t bias = ((intptr + (unit_size - 1)) & ~(unit_size - 1)) - intptr;

    return (char *)ptr + bias;
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

struct FixedAllocator fixed_alloc_init(size_t unit_size) {
    // num_of_elems * BLOCK_SIZE + num_of_elems * sizeof(CacheElem) = buff_size
    // num_of_elems * (BLOCK_SIZE + sizeof(CacheElem)) = buff_size
    // num_of_elems = buff_size / (BLOCK_SIZE + sizeof(CacheElem))

    void *mem = os_alloc(DEFAULT_ALLOC_SIZE);

    if (!mem) {
        fa_print_errno("os_alloc() failed in fixed_alloc_init()");
        assert(false);
    }

    void *aligned_up_mem = align_up_to_unit_size(mem, unit_size);

    size_t unused_mem_size = (char *)aligned_up_mem - (char *)mem;
    size_t buff_size = DEFAULT_ALLOC_SIZE - unused_mem_size;
    size_t num_of_elems = buff_size / (unit_size + sizeof(FixedAllocCacheElem));

    void *cache_mem = (char *)aligned_up_mem + (num_of_elems * unit_size);

    struct FixedAllocator fixed_alloc = {
        .os_allocated_mem = mem,
        .aligned_up_mem = aligned_up_mem,
        .os_allocated_size = DEFAULT_ALLOC_SIZE,
        .offset = 0,
        .unit_size = unit_size,
        .cache = FixedAllocCache_init(cache_mem, num_of_elems),
        .next = nullptr,
    };

    return fixed_alloc;
}

void fixed_alloc_deinit(struct FixedAllocator *fixed_alloc) {
    while (fixed_alloc != nullptr) {
        int ret = os_free(fixed_alloc->os_allocated_mem,
                          fixed_alloc->os_allocated_size);

        if (ret == OS_FREE_FAIL) {
            fa_print_errno("os_free() failed in fixed_alloc_deinit()");
            assert(false);
        }

        fixed_alloc = fixed_alloc->next;
    }
}

void *fixed_alloc(struct FixedAllocator *fixed_alloc) {
    assert(!is_full(fixed_alloc));

    void *ret;
    enum StackError err = FixedAllocCache_try_pop(&fixed_alloc->cache, &ret);

    if (err == STACK_OK) {
        return ret;
    }

    ret = fixed_alloc->aligned_up_mem + fixed_alloc->offset;
    fixed_alloc->offset += fixed_alloc->unit_size;
    return ret;
}

void fixed_free(struct FixedAllocator *fixed_alloc, void *block) {
    enum StackError err = FixedAllocCache_try_push(&fixed_alloc->cache, block);
    (void)err;
    assert(err != STACK_FULL);
}
