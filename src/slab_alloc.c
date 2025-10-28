#include "slab_alloc.h"

#include "bitmap.h"
#include "fixed_alloc.h"
#include "stack_definition.h"

#include "error.h"
#include "os_allocator.h"

#include <pthread.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

STACK_DEFINE(CacheOffset, CacheSizeType, CacheStack)

static constexpr CacheSizeType DEFAULT_CACHE_CAPACITY = 10;

static constexpr size_t SIZE_TO_CLASS_LOOKUP_SIZE = 2UL * FA_PAGE_SIZE;
static SlabSize *size_to_class_lookup = nullptr;
static SlabSize *num_of_elems_per_class_lookup = nullptr;

static inline void setup_size_to_class_lookup() {
    assert(!size_to_class_lookup);

    size_to_class_lookup = (SlabSize *)os_alloc(SIZE_TO_CLASS_LOOKUP_SIZE);

    if (!size_to_class_lookup) {
        fa_print_errno("os_alloc() failed in setup_size_to_class_lookup()");
        assert(false);
    }

    enum SlabSizeClass current_class_entry = 0;

    for (int i = 0; i <= SLAB_CLASS_MAX; ++i) {
        if (i > SLAB_SIZES[current_class_entry]) {
            ++current_class_entry;
        }

        size_to_class_lookup[i] = current_class_entry;
    }
}

static inline void setup_num_of_elems_per_class_lookup() {
    assert(num_of_elems_per_class_lookup == nullptr);

    num_of_elems_per_class_lookup = (SlabSize *)os_alloc(FA_PAGE_SIZE);

    if (!num_of_elems_per_class_lookup) {
        fa_print_errno(
            "os_alloc() failed in setup_num_of_elems_per_class_lookup()");
        assert(false);
    }

    for (enum SlabSizeClass class = SLAB_CLASS_8; class <= SLAB_CLASS_1024;
         ++class) {
        // Splitting the buffer so it stores both the data and the bitmap.

        // num_of_elems * elem_size + ceil(num_of_elems / 8) = buff_size
        // num_of_elems * elem_size + (num_of_elems + 7) / 8 = buff_size
        // num_of_elems * elem_size + num_of_elems / 8 + 7/8 = buff_size
        // num_of_elems * (elem_size + 1/8) = buff_size - 7/8
        // num_of_elems = (buff_size - 7/8) / (elem_size + 1/8)

        static constexpr float seven_eighths = 7.0F / 8.0F;
        static constexpr float one_eighth = 1.0F / 8.0F;

        constexpr SlabSize buff_size =
            SLAB_SIZE - (DEFAULT_CACHE_CAPACITY * sizeof(CacheOffset)) -
            sizeof(struct Slab);
        constexpr float bits_per_byte = 8.0F;
        constexpr float bitmap_elem_size = 1 / bits_per_byte;

        SlabSize elem_size = SLAB_SIZES[class];
        SlabSize num_of_elems = (SlabSize)((buff_size - seven_eighths) /
                                           ((float)elem_size + one_eighth));

        num_of_elems_per_class_lookup[class] = num_of_elems;
    }
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

static constexpr bool SHOULD_DESTROY_SLAB = true;

// If the ret vaule is SHOULD_DESTROY_SLAB (aka true), the slab shall be
// destroyed.
static inline bool handle_decrementing_alloc_counter(struct Slab *slab) {
    --slab->total_alloc_count;

    constexpr int slab_destroy_max_allocs_threshold = 10;

    return (bool)(slab->total_alloc_count == 0 &&
                  slab->max_alloc_count >= slab_destroy_max_allocs_threshold);
}

struct Slab *slab_from_ptr(void *ptr) {
    char *ptr_down_aligned = (char *)align_down_to_slab_size(ptr);

    struct Slab *slab_metadata =
        (struct Slab *)(ptr_down_aligned + SLAB_SIZE - sizeof(struct Slab));

    return slab_metadata;
}

bool slab_alloc_is_ptr_in_this_instance(const struct SlabAlloc *alloc,
                                        void *ptr) {
    assert(alloc != nullptr);
    assert(ptr != nullptr);

    struct Slab *slab = slab_from_ptr(ptr);
    return slab->owner == alloc;
}

static inline void *slab_buff_end(const struct Slab *slab) {
    assert(slab != nullptr);

    return slab->data + SLAB_SIZE - sizeof(struct Slab);
}

static inline bool is_ptr_in_slab(const struct Slab *slab, void *ptr) {
    return (bool)(

        (uint8_t *)ptr >= slab->data && (size_t *)ptr < slab->bmap.map

    );
}

static inline void slab_init(struct SlabAlloc *alloc, struct Slab *parent,
                             struct Slab **slab, enum SlabSizeClass class) {
    assert(slab != nullptr);
    assert(*slab == nullptr && "Slab already initialized.");

    uint8_t *mem = (uint8_t *)fixed_alloc(&alloc->fixed_alloc);
    assert(mem != nullptr);

    *slab = (struct Slab *)(mem + SLAB_SIZE) - 1;

    SlabSize num_of_elems = num_of_elems_per_class_lookup[class];

    SlabSize *bmap_data =
        (SlabSize *)(mem + (size_t)(num_of_elems * SLAB_SIZES[class]));

    CacheOffset *cache_data = (CacheOffset *)(*slab) - DEFAULT_CACHE_CAPACITY;

    assert(is_aligned((uintptr_t)bmap_data, sizeof(BitmapSize)));

    **slab = (struct Slab){
        .data = mem,
        .total_alloc_count = 0,
        .max_alloc_count = 0,
        .bmap = bitmap_init(bmap_data, num_of_elems),
        .cache = CacheStack_init(cache_data, DEFAULT_CACHE_CAPACITY),
        .size_class = class,
        .next_slab = nullptr,
        .prev_slab = parent,
        .owner = alloc,
    };
}

static inline void slab_deinit(struct SlabAlloc *alloc, struct Slab *slab) {
    if (slab->prev_slab == nullptr) {
        alloc->slabs[slab->size_class] = slab->next_slab;
    } else {
        slab->prev_slab->next_slab = slab->next_slab;
    }

    if (slab->next_slab != nullptr) {
        slab->next_slab->prev_slab = slab->prev_slab;
    }

    fixed_free(&alloc->fixed_alloc, slab->data);
}

struct SlabAlloc slab_alloc_init(struct Falloc *owner) {
    if (!size_to_class_lookup) {
        setup_size_to_class_lookup();
    }

    if (!num_of_elems_per_class_lookup) {
        setup_num_of_elems_per_class_lookup();
    }

    struct FixedAllocator fixed_alloc = fixed_alloc_init(SLAB_SIZE);

    struct SlabAlloc alloc;
    memset((void *)alloc.slabs, 0, sizeof(alloc.slabs));
    alloc.fixed_alloc = fixed_alloc;
    alloc.owner = owner;

    return alloc;
}

void slab_alloc_deinit(struct SlabAlloc *alloc) {
    assert(alloc != nullptr);

    fixed_alloc_deinit(&alloc->fixed_alloc);
}

void *slab_alloc(struct SlabAlloc *alloc, size_t size) {
    assert(alloc != nullptr);

    enum SlabSizeClass class = size_to_class_lookup[size];

    if (!alloc->slabs[class]) {
        slab_init(alloc, nullptr, &alloc->slabs[class], class);
    }

    struct Slab *slab = alloc->slabs[class];
    assert(slab != nullptr);

    while (true) {
        if (slab->cache.size != 0) {
            CacheOffset offset = CacheStack_pop(&slab->cache);

            size_t bitmap_index =
                (size_t)((float)offset *
                         SLAB_SIZE_CLASS_RECIPROCALS[slab->size_class]);

            bitmap_set_to_1(&slab->bmap, bitmap_index);

            increment_alloc_counter(slab);
            return slab->data + offset;
        }

        size_t free_slot = bitmap_find_free_and_swap(&slab->bmap);

        if (free_slot != BITMAP_NOT_FOUND) {
            increment_alloc_counter(slab);
            return (char *)slab->data + (size_t)(free_slot * SLAB_SIZES[class]);
        }

        if (!slab->next_slab) {
            slab_init(alloc, slab, &slab->next_slab, class);
        }

        slab = slab->next_slab;
    }
}

enum FaFreeRet slab_free(struct SlabAlloc *alloc, void *ptr) {
    struct Slab *slab = slab_from_ptr(ptr);
    assert((uint8_t *)ptr >= slab->data);

    SlabSize offset = (uint8_t *)ptr - slab->data;
    assert(offset <= CACHE_OFFSET_MAX);

    size_t bitmap_index =
        (size_t)((float)offset * SLAB_SIZE_CLASS_RECIPROCALS[slab->size_class]);
    bitmap_set_to_0(&slab->bmap, bitmap_index);

    CacheStack_try_push(&slab->cache, (CacheOffset)offset);

    if (handle_decrementing_alloc_counter(slab) == SHOULD_DESTROY_SLAB) {
        slab_deinit(alloc, slab);
    }

    return OK;
}

void *slab_realloc(struct SlabAlloc *alloc, void *ptr, size_t size) {
    if (!ptr) {
        return slab_alloc(alloc, size);
    }

    if (size == 0) {
        slab_free(alloc, ptr);
        return nullptr;
    }

    struct Slab *slab = slab_from_ptr(ptr);
    SlabSize old_size = SLAB_SIZES[slab->size_class];

    if (size <= old_size) {
        return ptr;
    }

    void *new_mem = slab_alloc(alloc, size);

    if (!new_mem) {
        return nullptr;
    }

    memcpy(new_mem, ptr, old_size);

    slab_free(alloc, ptr);

    return new_mem;
}

size_t slab_memsize(void *ptr) {
    struct Slab *slab = slab_from_ptr(ptr);
    return SLAB_SIZES[slab->size_class];
}
