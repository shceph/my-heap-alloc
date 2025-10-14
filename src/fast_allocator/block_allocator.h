#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "stack_declaration.h"

#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef void *BaCacheElem;
typedef size_t BaCacheSizeType;

STACK_DECLARE(BaCacheElem, BaCacheSizeType, BaCache)

constexpr size_t DEFAULT_ALLOC_SIZE = 128 * OS_ALLOC_PAGE_SIZE;
constexpr size_t FA_BLOCK_SIZE = 4 * OS_ALLOC_PAGE_SIZE;

struct BlockAllocator {
    void *os_allocated_mem;
    void *aligned_up_mem;
    size_t os_allocated_size;
    size_t offset;
    struct BaCache cache;
    struct BlockAllocator *next;
};

void *align_up_to_block_size(const void *ptr);
void *align_down_to_block_size(const void *ptr);
struct BlockAllocator balloc_init();
void balloc_deinit(struct BlockAllocator *block_alloc);
void *balloc_alloc(struct BlockAllocator *block_alloc);
void balloc_free(struct BlockAllocator *block_alloc, void *block);

#endif // BLOCK_ALLOCATOR_H
