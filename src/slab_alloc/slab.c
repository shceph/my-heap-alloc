#include "slab_alloc/slab.h"

#include "bitmap.h"
#include "fixed_alloc.h"
#include "stack_definition.h"

#include <libdivide.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

STACK_DEFINE(CacheOffset, CacheSizeType, CacheStack)

const SlabSize ELEMENT_SIZES[SLAB_NUM_CLASSES] = {
    [SLAB_CLASS_8] = 8,     [SLAB_CLASS_16] = 16,     [SLAB_CLASS_24] = 24,
    [SLAB_CLASS_32] = 32,   [SLAB_CLASS_48] = 48,     [SLAB_CLASS_64] = 64,
    [SLAB_CLASS_80] = 80,   [SLAB_CLASS_96] = 96,     [SLAB_CLASS_112] = 112,
    [SLAB_CLASS_128] = 128, [SLAB_CLASS_160] = 160,   [SLAB_CLASS_192] = 192,
    [SLAB_CLASS_224] = 224, [SLAB_CLASS_256] = 256,   [SLAB_CLASS_288] = 288,
    [SLAB_CLASS_320] = 320, [SLAB_CLASS_352] = 352,   [SLAB_CLASS_384] = 384,
    [SLAB_CLASS_416] = 416, [SLAB_CLASS_448] = 448,   [SLAB_CLASS_480] = 480,
    [SLAB_CLASS_512] = 512, [SLAB_CLASS_544] = 544,   [SLAB_CLASS_576] = 576,
    [SLAB_CLASS_608] = 608, [SLAB_CLASS_640] = 640,   [SLAB_CLASS_672] = 672,
    [SLAB_CLASS_704] = 704, [SLAB_CLASS_736] = 736,   [SLAB_CLASS_768] = 768,
    [SLAB_CLASS_800] = 800, [SLAB_CLASS_832] = 832,   [SLAB_CLASS_864] = 864,
    [SLAB_CLASS_896] = 896, [SLAB_CLASS_928] = 928,   [SLAB_CLASS_960] = 960,
    [SLAB_CLASS_992] = 992, [SLAB_CLASS_1024] = 1024,
};

static inline bool is_aligned(size_t val, size_t align) {
    return (val & (align - 1)) == 0;
}

static inline void increment_alloc_counter(struct Slab *slab) {
    ++slab->total_alloc_count;

    if (slab->total_alloc_count > slab->max_alloc_count) {
        // TODO: Probably can just increment max
        slab->max_alloc_count = slab->total_alloc_count;
    }
}

#define SHOULD_DESTROY_SLAB true

// If the return vaule is SHOULD_DESTROY_SLAB (aka true), the slab should be
// destroyed.
static inline bool decrement_alloc_counter(struct Slab *slab) {
    const int slab_destroy_max_allocs_threshold = 10;

    --slab->total_alloc_count;

    return (bool)(slab->total_alloc_count == 0 &&
                  slab->max_alloc_count >= slab_destroy_max_allocs_threshold);
}

static inline void *slab_buff_end(const struct Slab *slab) {
    assert(slab != NULL);

    return slab->data + SLAB_SIZE - sizeof(struct Slab);
}

static inline bool is_ptr_in_slab(const struct Slab *slab, void *ptr) {
    return (uint8_t *)ptr >= slab->data && (size_t *)ptr < slab->bitmap.map;
}

void slab_init(struct SlabAlloc *owner, struct FixedAllocator *slab_store,
               struct Slab *slab, enum SizeClass size_class,
               SlabSize num_of_elems) {
    uint8_t *mem = (uint8_t *)fixed_alloc(slab_store);
    assert(mem);

    struct Slab **ptr_to_metadata =
        (struct Slab **)(mem + SLAB_SIZE - sizeof(struct Slab *));

    *ptr_to_metadata = slab;

    // SlabSize num_of_elems = num_of_elems_per_class_lookup[size_class];

    SlabSize *bitmap_data =
        (SlabSize *)(mem + (size_t)(num_of_elems * ELEMENT_SIZES[size_class]));

    CacheOffset *cache_data =
        (CacheOffset *)ptr_to_metadata - DEFAULT_CACHE_CAPACITY;

    assert(is_aligned((uintptr_t)bitmap_data, sizeof(BitmapSize)));

    *slab = (struct Slab){
        .data = mem,
        .total_alloc_count = 0,
        .max_alloc_count = 0,
        .bitmap = bitmap_init(bitmap_data, num_of_elems),
        .cache = CacheStack_init(cache_data, DEFAULT_CACHE_CAPACITY),
        .size_class = size_class,
        .owner = owner,
    };
}

void slab_deinit(struct FixedAllocator *slab_store, struct Slab *slab) {
    fixed_free(slab_store, slab->data);
}

void *find_in_slab(struct Slab *slab) {
    if (slab->cache.size != 0) {
        CacheOffset offset = CacheStack_pop(&slab->cache);

        struct libdivide_u64_t fast_d =
            libdivide_u64_gen(ELEMENT_SIZES[slab->size_class]);
        size_t bitmap_index = libdivide_u64_do(offset, &fast_d);

        bitmap_set_to_1(&slab->bitmap, bitmap_index);

        increment_alloc_counter(slab);
        return slab->data + offset;
    }

    size_t free_slot = bitmap_find_free_and_swap(&slab->bitmap);

    if (free_slot != BITMAP_NOT_FOUND) {
        increment_alloc_counter(slab);
        return (char *)slab->data +
               (size_t)(free_slot * ELEMENT_SIZES[slab->size_class]);
    }

    return NULL;
}

void free_from_slab(struct Slab *slab, void *ptr) {
    assert((uint8_t *)ptr >= slab->data);

    SlabSize offset = (uint8_t *)ptr - slab->data;
    assert(offset <= CACHE_OFFSET_MAX);

    struct libdivide_u64_t fast_d =
        libdivide_u64_gen(ELEMENT_SIZES[slab->size_class]);
    size_t bitmap_index = libdivide_u64_do(offset, &fast_d);

    bitmap_set_to_0(&slab->bitmap, bitmap_index);

    CacheStack_try_push(&slab->cache, (CacheOffset)offset);
}
