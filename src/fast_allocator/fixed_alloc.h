#ifndef FIXED_ALLOC_H
#define FIXED_ALLOC_H

#include "stack_declaration.h"

#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef void *FixedAllocCacheElem;
typedef size_t FixedAllocCacheSizeType;

STACK_DECLARE(FixedAllocCacheElem, FixedAllocCacheSizeType, FixedAllocCache)

constexpr size_t DEFAULT_ALLOC_SIZE = 128 * OS_ALLOC_PAGE_SIZE;
constexpr size_t FA_BLOCK_SIZE = 4 * OS_ALLOC_PAGE_SIZE;

struct FixedAllocator {
    void *os_allocated_mem;
    void *aligned_up_mem;
    size_t os_allocated_size;
    size_t offset;
    size_t unit_size;
    struct FixedAllocCache cache;
    struct FixedAllocator *next;
};

void *align_up_to_block_size(const void *ptr);
void *align_down_to_block_size(const void *ptr);
// unit_size must be a power of 2.
struct FixedAllocator fixed_alloc_init(size_t unit_size);
void fixed_alloc_deinit(struct FixedAllocator *fixed_alloc);
void *fixed_alloc(struct FixedAllocator *fixed_alloc);
void fixed_free(struct FixedAllocator *fixed_alloc, void *block);

#endif // FIXED_ALLOC_H
