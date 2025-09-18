#ifndef FAST_ALLOC_PRINT_LAYOUT_H
#define FAST_ALLOC_PRINT_LAYOUT_H

#include "../src/fast_allocator/fast_allocator.h"

#include <stdio.h>

inline static void print_block_data(FastAllocBlock *block) {
    printf("data: %p\n", block->data);
    printf("cache: %p\n", (void *)block->cache);
    printf("next block: %p\n", (void *)block->next_block);
    printf("data size: %d\n", block->data_size);
    printf("cache size: %d\n", block->cache_size);
    printf("size class: %d\n\n", FAST_ALLOC_SIZES[block->size_class]);
}

inline static void fast_alloc_print_layout(FastAllocator *alloc) {
    puts("");

    for (int i = 0; i < FAST_ALLOC_NUM_CLASSES; ++i) {
        FastAllocBlock *block = alloc->blocks[i];

        while (block != nullptr) {
            print_block_data(block);
            block = block->next_block;
        }
    }
}

#endif // FAST_ALLOC_PRINT_LAYOUT_H
