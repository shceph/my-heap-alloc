#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

constexpr size_t DEFAULT_ALLOC_SIZE = 128 * OS_ALLOC_PAGE_SIZE;
constexpr size_t FA_BLOCK_SIZE = 4 * OS_ALLOC_PAGE_SIZE;

struct BlockAllocator {
    void *mem;
    void *aligned_up_mem;
    size_t size;
    size_t offset;
    struct BlockAllocator *next;
};

void *align_up_to_block_size(const void *ptr);
void *align_down_to_block_size(const void *ptr);
struct BlockAllocator block_alloc_init();
void block_alloc_deinit(struct BlockAllocator *block_alloc);
void *block_alloc_alloc(struct BlockAllocator *block_alloc);

#endif // BLOCK_ALLOCATOR_H
