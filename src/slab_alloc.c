#include "slab_alloc.h"

#include "bitmap.h"
#include "error.h"
#include "fixed_alloc.h"
#include "os_alloc.h"
#include "stack_definition.h"

#include <libdivide.h>

#include <pthread.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

STACK_DEFINE(CacheOffset, CacheSizeType, CacheStack)

#define SLAB_SIZE ((size_t)(1024 * OS_ALLOC_PAGE_SIZE))

#define SLAB_POOL_SIZE OS_ALLOC_PAGE_SIZE

#define DEFAULT_CACHE_CAPACITY    100
#define SIZE_TO_CLASS_LOOKUP_SIZE (2UL * OS_ALLOC_PAGE_SIZE)

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

static SlabSize *size_to_class_lookup = NULL;
static SlabSize *num_of_elems_per_class_lookup = NULL;

static inline void setup_size_to_class_lookup() {
    assert(!size_to_class_lookup);

    size_to_class_lookup = (SlabSize *)os_alloc(SIZE_TO_CLASS_LOOKUP_SIZE);

    if (!size_to_class_lookup) {
        fa_print_errno("os_alloc() failed in setup_size_to_class_lookup()");
        assert(false);
    }

    enum SizeClass current_class_entry = 0;

    for (int i = 0; i <= SLAB_CLASS_MAX; ++i) {
        if (i > ELEMENT_SIZES[current_class_entry]) {
            ++current_class_entry;
        }

        size_to_class_lookup[i] = current_class_entry;
    }
}

static inline void setup_num_of_elems_per_class_lookup() {
    assert(num_of_elems_per_class_lookup == NULL);

    num_of_elems_per_class_lookup = (SlabSize *)os_alloc(OS_ALLOC_PAGE_SIZE);

    if (!num_of_elems_per_class_lookup) {
        fa_print_errno(
            "os_alloc() failed in setup_num_of_elems_per_class_lookup()");
        assert(false);
    }

    enum {
        BITS_PER_BYTE = 8,
        BITMAP_BITS_PER_ELEM = 1,
        BITMAP_ROUNDING_ADJUSTMENT = BITS_PER_BYTE - 1,
    };

    for (enum SizeClass size_class = SLAB_CLASS_8;
         size_class <= SLAB_CLASS_1024; ++size_class) {
        // Splitting the buffer so it stores both the data and the bitmap.

        // num_of_elems * elem_size + ceil(num_of_elems / 8) = buff_size
        // num_of_elems * elem_size + (num_of_elems + 7) / 8 = buff_size
        // num_of_elems * elem_size + num_of_elems / 8 + 7/8 = buff_size
        // num_of_elems * (elem_size + 1/8) = buff_size - 7/8
        // num_of_elems = (buff_size - 7/8) / (elem_size + 1/8)
        // num_of_elems = (8 * (buff_size - 7/8)) / (8 * (elem_size + 1/8))
        // num_of_elems = (8 * buff_size - 7) / (8 * elem_size + 1)

        const SlabSize buff_size =
            SLAB_SIZE - (DEFAULT_CACHE_CAPACITY * sizeof(CacheOffset)) -
            sizeof(struct Slab *);

        SlabSize elem_size = ELEMENT_SIZES[size_class];
        SlabSize num_of_elems =
            ((BITS_PER_BYTE * buff_size) - BITMAP_ROUNDING_ADJUSTMENT) /
            ((BITS_PER_BYTE * elem_size) + BITMAP_BITS_PER_ELEM);

        num_of_elems_per_class_lookup[size_class] = num_of_elems;
    }
}

static inline void *align_down_to_slab_size(const void *ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t intptr_down_aligned = intptr & ~(SLAB_SIZE - 1);
    uintptr_t bias = intptr - intptr_down_aligned;

    return (char *)ptr - bias;
}

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

static inline void *find_in_slab(struct Slab *slab) {
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

static inline void *slab_buff_end(const struct Slab *slab) {
    assert(slab != NULL);

    return slab->data + SLAB_SIZE - sizeof(struct Slab);
}

static inline bool is_ptr_in_slab(const struct Slab *slab, void *ptr) {
    return (uint8_t *)ptr >= slab->data && (size_t *)ptr < slab->bitmap.map;
}

static inline struct SlabPool *slab_pool_init(struct SlabAlloc *alloc,
                                              enum SizeClass size_class) {
    struct SlabPool *pool = (struct SlabPool *)fixed_alloc(&alloc->pool_store);

    const size_t flex_arr_size = SLAB_POOL_SIZE - sizeof(struct SlabPool);

    memset(pool->slabs, 0, flex_arr_size);

    pool->len = 0;
    pool->cap = flex_arr_size / sizeof(struct Slab);
    pool->size_class = size_class;
    pool->next = NULL;

    return pool;
}

static inline void slab_init(struct SlabAlloc *alloc, struct Slab *slab,
                             enum SizeClass size_class) {
    uint8_t *mem = (uint8_t *)fixed_alloc(&alloc->slab_store);
    assert(mem);

    struct Slab **ptr_to_metadata =
        (struct Slab **)(mem + SLAB_SIZE - sizeof(struct Slab *));

    *ptr_to_metadata = slab;

    SlabSize num_of_elems = num_of_elems_per_class_lookup[size_class];

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
        .owner = alloc,
    };
}

static inline void slab_deinit(struct SlabAlloc *alloc, struct Slab *slab) {
    fixed_free(&alloc->slab_store, slab->data);
}

static inline struct Slab *add_slab(struct SlabAlloc *alloc,
                                    enum SizeClass size_class) {
    if (!alloc->pools[size_class]) {
        alloc->pools[size_class] = slab_pool_init(alloc, size_class);

        if (!alloc->pools[size_class]) {
            return NULL;
        }
    }

    struct SlabPool *pool = alloc->pools[size_class];

    while (pool->len == pool->cap) {
        if (!pool->next) {
            pool->next = slab_pool_init(alloc, size_class);

            if (!pool->next) {
                return NULL;
            }
        }

        pool = pool->next;
    }

    assert(pool->len < pool->cap);

    struct Slab *slab = &pool->slabs[pool->len];
    slab_init(alloc, slab, size_class);

    ++pool->len;

    return slab;
}

struct Slab *slab_from_ptr(void *ptr) {
    char *ptr_down_aligned = (char *)align_down_to_slab_size(ptr);

    struct Slab **slab_metadata =
        (struct Slab **)(ptr_down_aligned + SLAB_SIZE - sizeof(struct Slab *));

    return *slab_metadata;
}

bool slab_alloc_is_ptr_in_instance(const struct SlabAlloc *instance,
                                   void *ptr) {
    assert(instance);
    assert(ptr);

    struct Slab *slab = slab_from_ptr(ptr);
    return slab->owner == instance;
}

struct SlabAlloc slab_alloc_init(struct Falloc *owner) {
    if (!size_to_class_lookup) {
        setup_size_to_class_lookup();
    }

    if (!num_of_elems_per_class_lookup) {
        setup_num_of_elems_per_class_lookup();
    }

    struct SlabAlloc alloc;
    memset((void *)alloc.pools, 0, sizeof(alloc.pools));
    alloc.slab_store = fixed_alloc_init(SLAB_SIZE);
    alloc.pool_store = fixed_alloc_init(SLAB_POOL_SIZE);
    alloc.owner = owner;

    return alloc;
}

void slab_alloc_deinit(struct SlabAlloc *alloc) {
    assert(alloc);

    fixed_alloc_deinit(&alloc->slab_store);
}

void *slab_alloc(struct SlabAlloc *alloc, size_t size) {
    assert(alloc);

    enum SizeClass size_class = size_to_class_lookup[size];

    if (!alloc->pools[size_class] && !add_slab(alloc, size_class)) {
        return NULL;
    }

    struct SlabPool *pool = alloc->pools[size_class];

    while (pool) {
        for (size_t i = 0; i < pool->len; ++i) {
            struct Slab *slab = &pool->slabs[i];

            void *ptr = find_in_slab(slab);

            if (ptr) {
                return ptr;
            }
        }

        pool = pool->next;
    }

    struct Slab *slab = add_slab(alloc, size_class);

    if (!slab) {
        return NULL;
    }

    return find_in_slab(slab);
}

void slab_free(struct SlabAlloc *alloc, void *ptr) {
    (void)alloc;

    struct Slab *slab = slab_from_ptr(ptr);
    assert((uint8_t *)ptr >= slab->data);
    assert(slab->owner == alloc);

    SlabSize offset = (uint8_t *)ptr - slab->data;
    assert(offset <= CACHE_OFFSET_MAX);

    struct libdivide_u64_t fast_d =
        libdivide_u64_gen(ELEMENT_SIZES[slab->size_class]);
    size_t bitmap_index = libdivide_u64_do(offset, &fast_d);

    bitmap_set_to_0(&slab->bitmap, bitmap_index);

    CacheStack_try_push(&slab->cache, (CacheOffset)offset);
}

void *slab_realloc(struct SlabAlloc *alloc, void *ptr, size_t size) {
    if (!ptr) {
        return slab_alloc(alloc, size);
    }

    if (size == 0) {
        slab_free(alloc, ptr);
        return NULL;
    }

    struct Slab *slab = slab_from_ptr(ptr);
    SlabSize old_size = ELEMENT_SIZES[slab->size_class];

    if (size <= old_size) {
        return ptr;
    }

    void *new_mem = slab_alloc(alloc, size);

    if (!new_mem) {
        return NULL;
    }

    memcpy(new_mem, ptr, old_size);

    slab_free(alloc, ptr);

    return new_mem;
}

size_t slab_memsize(void *ptr) {
    struct Slab *slab = slab_from_ptr(ptr);
    return ELEMENT_SIZES[slab->size_class];
}
