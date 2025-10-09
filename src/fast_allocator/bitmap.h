#ifndef BITMAP_H
#define BITMAP_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef size_t BitmapSize;

constexpr BitmapSize BITMAP_SIZE_BIT_COUNT = sizeof(BitmapSize) * 8;
constexpr BitmapSize LOG2_NUM_BITS_IN_BITMAP_SIZE =
    BITMAP_SIZE_BIT_COUNT == 64 ? 6 : 5;

constexpr BitmapSize BITMAP_NOT_FOUND = SIZE_MAX;
constexpr BitmapSize ALL_BITS_ARE_1 = SIZE_MAX;

struct Bitmap {
    BitmapSize *map;
    BitmapSize num_elems;
};

struct Bitmap bitmap_init(void *mem_size_t_aligned, BitmapSize num_elements);
BitmapSize bitmap_find_free_and_swap(struct Bitmap *bmap);
void bitmap_set_to_0(struct Bitmap *bmap, BitmapSize bit_index);
void bitmap_set_to_1(struct Bitmap *bmap, BitmapSize bit_index);

#endif // BITMAP_H
