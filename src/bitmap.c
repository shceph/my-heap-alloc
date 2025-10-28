#include "bitmap.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline BitmapSize ceil_int_div_by_64(BitmapSize num) {
    return (num + BITMAP_SIZE_BIT_COUNT - 1) >> LOG2_NUM_BITS_IN_BITMAP_SIZE;
}

static inline BitmapSize ceil_int_div_by_8(BitmapSize num) {
    // NOLINTNEXTLINE(readability-magic-numbers)
    return (num + 7) >> 3;
}

struct Bitmap bitmap_init(void *mem_size_t_aligned, BitmapSize num_elements) {
    memset(mem_size_t_aligned, 0, ceil_int_div_by_8(num_elements));

    BitmapSize tail = num_elements & (BITMAP_SIZE_BIT_COUNT - 1);

    if (tail != 0) {
        BitmapSize *map = mem_size_t_aligned;
        BitmapSize unused_bits = SIZE_MAX << tail;
        map[ceil_int_div_by_64(num_elements) - 1] = unused_bits;
    }

    return (struct Bitmap){
        .map = mem_size_t_aligned,
        .num_elems = num_elements,
    };
}

BitmapSize bitmap_find_free_and_swap(struct Bitmap *bmap) {
    assert(bmap != nullptr);

    for (BitmapSize i = 0; i < ceil_int_div_by_64(bmap->num_elems); ++i) {
        if (bmap->map[i] == ALL_BITS_ARE_1) {
            continue;
        }

        BitmapSize inverted = ~bmap->map[i];
        int ctz = __builtin_ctzll(inverted);
        bmap->map[i] |= (BitmapSize)1 << ctz;
        return (i * BITMAP_SIZE_BIT_COUNT) + ctz;
    }

    return BITMAP_NOT_FOUND;
}

void bitmap_set_to_0(struct Bitmap *bmap, BitmapSize bit_index) {
    bmap->map[bit_index >> LOG2_NUM_BITS_IN_BITMAP_SIZE] &=
        ~((BitmapSize)1 << (bit_index & (BITMAP_SIZE_BIT_COUNT - 1)));
}

void bitmap_set_to_1(struct Bitmap *bmap, BitmapSize bit_index) {
    bmap->map[bit_index >> LOG2_NUM_BITS_IN_BITMAP_SIZE] |=
        (BitmapSize)1 << (bit_index & (BITMAP_SIZE_BIT_COUNT - 1));
}
