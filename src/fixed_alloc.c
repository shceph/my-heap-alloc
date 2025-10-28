#include "fixed_alloc.h"

#include "stack_definition.h"

#include "error.h"
#include "os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

STACK_DEFINE(FixedAllocCacheElem, FixedAllocCacheSizeType, FixedAllocCache)

#define GET_NUM_OF_ELEMS(FIXED_ALLOC_VAR) ((FIXED_ALLOC_VAR).cache.capacity)

static constexpr size_t DEFAULT_ALLOC_SIZE = 0x1000 * OS_ALLOC_PAGE_SIZE;

static inline bool is_full(struct FixedAllocBlock *block) {
    size_t fixed_alloc_buff_size =
        (GET_NUM_OF_ELEMS(*block)) * block->unit_size;

    return block->offset >= fixed_alloc_buff_size - block->unit_size &&
           block->cache.size == 0;
}

static inline bool is_ptr_in_block(struct FixedAllocBlock *block, void *ptr) {
    uint8_t *block_begin = (uint8_t *)block->aligned_up_mem;
    uint8_t *block_end =
        block_begin + (GET_NUM_OF_ELEMS(*block) * block->unit_size);
    uint8_t *byte_ptr = (uint8_t *)ptr;

    return byte_ptr >= block_begin && byte_ptr < block_end;
}

static inline void *align_up_to_unit_size(const void *ptr, size_t unit_size) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t bias = ((intptr + (unit_size - 1)) & ~(unit_size - 1)) - intptr;

    return (char *)ptr + bias;
}

void *align_up_to_block_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t bias = ((intptr + (SLAB_SIZE - 1)) & ~(SLAB_SIZE - 1)) - intptr;

    return (char *)ptr + bias;
}

void *align_down_to_slab_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t intptr_down_aligned = intptr & ~(SLAB_SIZE - 1);
    uintptr_t bias = intptr - intptr_down_aligned;
    return (char *)ptr - bias;
}

static inline struct FixedAllocBlock block_init(size_t unit_size,
                                                size_t block_size) {
    // num_of_elems * unit_size + num_of_elems * sizeof(CacheElem) = buff_size
    // num_of_elems * (unit_size + sizeof(CacheElem)) = buff_size
    // num_of_elems = buff_size / (unit_size + sizeof(CacheElem))

    void *mem = os_alloc(block_size);

    if (!mem) {
        fa_print_errno("os_alloc() failed in fixed_alloc_init()");
        assert(false);
    }

    void *aligned_up_mem = align_up_to_unit_size(mem, unit_size);

    size_t unused_mem_size = (char *)aligned_up_mem - (char *)mem;
    size_t buff_size = block_size - unused_mem_size;
    size_t num_of_elems = buff_size / (unit_size + sizeof(FixedAllocCacheElem));

    void *cache_mem = (char *)aligned_up_mem + (num_of_elems * unit_size);

    struct FixedAllocBlock block = {
        .os_allocated_mem = mem,
        .aligned_up_mem = aligned_up_mem,
        .os_allocated_size = block_size,
        .offset = 0,
        .unit_size = unit_size,
        .cache = FixedAllocCache_init(cache_mem, num_of_elems),
    };

    return block;
}

static inline void block_deinit(struct FixedAllocBlock *block) {
    int ret = os_free(block->os_allocated_mem, block->os_allocated_size);

    if (ret == OS_FREE_FAIL) {
        fa_print_errno("os_free() failed in fixed_alloc_deinit()");
        assert(false);
    }
}

static inline void add_block(struct FixedAllocator *alloc) {
    assert(alloc->block_count > 0 &&
           alloc->block_count < FIXED_ALLOC_BLOCK_CAPACITY);

    alloc->blocks[alloc->block_count] =
        block_init(alloc->unit_size,
                   alloc->blocks[alloc->block_count - 1].os_allocated_size * 2);

    ++alloc->block_count;
}

static inline void *allocate_from_block(struct FixedAllocBlock *block) {
    if (is_full(block)) {
        return nullptr;
    }

    void *ret;
    enum StackError err = FixedAllocCache_try_pop(&block->cache, &ret);

    if (err == STACK_OK) {
        return ret;
    }

    ret = block->aligned_up_mem + block->offset;
    block->offset += block->unit_size;
    return ret;
}

static inline void free_from_block(struct FixedAllocBlock *block, void *ptr) {
    enum StackError err = FixedAllocCache_try_push(&block->cache, ptr);
    (void)err;
    assert(err != STACK_FULL);
}

struct FixedAllocator fixed_alloc_init(size_t unit_size) {
    struct FixedAllocBlock *blocks =
        os_alloc(FIXED_ALLOC_BLOCK_CAPACITY * sizeof(struct FixedAllocBlock));

    blocks[0] = block_init(unit_size, DEFAULT_ALLOC_SIZE);

    return (struct FixedAllocator){
        .block_count = 1,
        .unit_size = unit_size,
        .blocks = blocks,
    };
}

void fixed_alloc_deinit(struct FixedAllocator *alloc) {
    for (uint32_t i = 0; i < alloc->block_count; ++i) {
        block_deinit(&alloc->blocks[i]);
    }

    os_free(alloc->blocks,
            FIXED_ALLOC_BLOCK_CAPACITY * sizeof(struct FixedAllocBlock));
}

void *fixed_alloc(struct FixedAllocator *alloc) {
    for (uint32_t i = 0; i < alloc->block_count; ++i) {
        void *ret = allocate_from_block(&alloc->blocks[i]);

        if (ret) {
            return ret;
        }
    }

    add_block(alloc);

    return allocate_from_block(&alloc->blocks[alloc->block_count - 1]);
}

void fixed_free(struct FixedAllocator *alloc, void *ptr) {
    for (uint32_t i = 0; i < alloc->block_count; ++i) {
        struct FixedAllocBlock *block = &alloc->blocks[i];

        if (is_ptr_in_block(block, ptr)) {
            free_from_block(block, ptr);
            return;
        }
    }

    assert(false && "ptr not allocated with this FixedAllocator instance");
}
