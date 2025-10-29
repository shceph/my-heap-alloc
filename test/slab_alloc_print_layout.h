#ifndef FAST_ALLOC_PRINT_LAYOUT_H
#define FAST_ALLOC_PRINT_LAYOUT_H

#include "bitmap.h"
#include "slab_alloc.h"

#include <stdio.h>

static inline BitmapSize ceil_int_div_by_64(BitmapSize num) {
    return (num + BITMAP_SIZE_BIT_COUNT - 1) >> LOG2_NUM_BITS_IN_BITMAP_SIZE;
}

static inline void print_slab_data(const struct Slab *slab) {
    printf("data: %p\n", slab->data);
    // printf("cache: %p\n", (void *)slab->cache);
    printf("bitmap: %p\n", (void *)&slab->bmap);
    printf("next slab: %p\n", (void *)slab->next_slab);
    // printf("data size: %d\n", slab->data_size);
    // printf("cache size: %d\n", slab->cache_size);
    printf("size class: %d\n\n", SLAB_SIZES[slab->size_class]);
}

static inline void slab_alloc_print_layout(const struct SlabAlloc *alloc) {
    puts("");
    bool slab_in_use_found = false;

    for (int i = 0; i < SLAB_NUM_CLASSES; ++i) {
        struct Slab *slab = alloc->slabs[i];

        while (slab != nullptr) {
            slab_in_use_found = true;
            print_slab_data(slab);
            slab = slab->next_slab;
        }
    }

    if (!slab_in_use_found) {
        puts("The allocator is empty.");
    }
}

static inline void print_size_t_binary(size_t value) {
    // Number of bits in size_t
    int bits = sizeof(size_t) * CHAR_BIT;

    for (int i = bits - 1; i >= 0; i--) {
        (void)putchar((value & ((size_t)1 << i)) ? '1' : '0');
    }

    (void)putchar('\n');
}

static inline void print_bitmap(const struct Slab *slab) {
    (void)putchar('\n');

    if (slab == nullptr) {
        puts("The slab is null, so no bitmap printed.");
        return;
    }

    for (size_t i = 0; i < ceil_int_div_by_64(slab->bmap.num_elems); ++i) {
        print_size_t_binary(slab->bmap.map[i]);
    }

    (void)putchar('\n');
}

#endif // FAST_ALLOC_PRINT_LAYOUT_H
