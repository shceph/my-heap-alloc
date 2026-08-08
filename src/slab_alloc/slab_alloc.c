#include "slab_alloc/slab_alloc.h"
#include "slab_alloc/slab.h"

#include "fixed_alloc.h"
#include "os_alloc.h"

#include <pthread.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SLAB_POOL_SIZE            OS_ALLOC_PAGE_SIZE
#define SIZE_TO_CLASS_LOOKUP_SIZE (2UL * OS_ALLOC_PAGE_SIZE)

static SlabSize *size_to_class_lookup = NULL;
static SlabSize *num_of_elems_per_class_lookup = NULL;

static inline void setup_size_to_class_lookup() {
    assert(!size_to_class_lookup);

    size_to_class_lookup = (SlabSize *)os_alloc(SIZE_TO_CLASS_LOOKUP_SIZE);

    if (!size_to_class_lookup) {
        assert(false && "os_alloc() failed in setup_size_to_class_lookup()");
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
        assert(false &&
               "os_alloc() failed in setup_num_of_elems_per_class_lookup()");
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
    slab_init(alloc, &alloc->slab_store, slab, size_class,
              num_of_elems_per_class_lookup[size_class]);

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
    assert(slab->owner == alloc);
    free_from_slab(slab, ptr);
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
