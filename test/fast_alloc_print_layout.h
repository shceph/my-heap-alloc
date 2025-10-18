#ifndef FAST_ALLOC_PRINT_LAYOUT_H
#define FAST_ALLOC_PRINT_LAYOUT_H

#include "../src/fast_allocator/bitmap.h"
#include "../src/fast_allocator/fast_alloc.h"

#include <stdio.h>

inline static BitmapSize ceil_int_div_by_64(BitmapSize num) {
    return (num + BITMAP_SIZE_BIT_COUNT - 1) >> LOG2_NUM_BITS_IN_BITMAP_SIZE;
}

inline static void print_block_data(const struct FaBlock *block) {
    printf("data: %p\n", block->data);
    // printf("cache: %p\n", (void *)block->cache);
    printf("bitmap: %p\n", (void *)&block->bmap);
    printf("next block: %p\n", (void *)block->next_block);
    // printf("data size: %d\n", block->data_size);
    // printf("cache size: %d\n", block->cache_size);
    printf("size class: %d\n\n", FA_SIZES[block->size_class]);
}

inline static void fast_alloc_print_layout(const struct FaAllocator *alloc) {
    puts("");
    bool block_in_use_found = false;

    for (int i = 0; i < FA_NUM_CLASSES; ++i) {
        struct FaBlock *block = alloc->blocks[i];

        while (block != nullptr) {
            block_in_use_found = true;
            print_block_data(block);
            block = block->next_block;
        }
    }

    if (!block_in_use_found) {
        puts("The allocator is empty.");
    }
}

inline static void print_size_t_binary(size_t value) {
    // Number of bits in size_t
    int bits = sizeof(size_t) * CHAR_BIT;

    for (int i = bits - 1; i >= 0; i--) {
        (void)putchar((value & ((size_t)1 << i)) ? '1' : '0');
    }

    (void)putchar('\n');
}

inline static void print_bitmap(const struct FaBlock *block) {
    (void)putchar('\n');

    if (block == nullptr) {
        puts("The block is null, so no bitmap printed.");
        return;
    }

    for (size_t i = 0; i < ceil_int_div_by_64(block->bmap.num_elems); ++i) {
        print_size_t_binary(block->bmap.map[i]);
    }

    (void)putchar('\n');
}

#endif // FAST_ALLOC_PRINT_LAYOUT_H
