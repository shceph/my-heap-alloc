#ifndef FIXED_ALLOC_H
#define FIXED_ALLOC_H

#include "stack_declaration.h"

#include "os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef void *FixedAllocCacheElem;
typedef size_t FixedAllocCacheSizeType;

STACK_DECLARE(FixedAllocCacheElem, FixedAllocCacheSizeType, FixedAllocCache)

constexpr size_t SLAB_SIZE = 8 * OS_ALLOC_PAGE_SIZE;
constexpr size_t FIXED_ALLOC_BLOCK_CAPACITY = 64;

struct FixedAllocBlock {
    void *os_allocated_mem;
    void *aligned_up_mem;
    size_t os_allocated_size;
    size_t offset;
    size_t unit_size;
    struct FixedAllocCache cache;
};

struct FixedAllocator {
    uint32_t block_count;
    uint32_t unit_size;
    struct FixedAllocBlock *blocks;
};

void *align_up_to_block_size(const void *ptr);
void *align_down_to_slab_size(const void *ptr);
// unit_size must be a power of 2.
struct FixedAllocator fixed_alloc_init(size_t unit_size);
void fixed_alloc_deinit(struct FixedAllocator *fixed_alloc);
void *fixed_alloc(struct FixedAllocator *fixed_alloc);
void fixed_free(struct FixedAllocator *fixed_alloc, void *ptr);

#endif // FIXED_ALLOC_H
