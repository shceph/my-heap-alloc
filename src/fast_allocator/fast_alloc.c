#include "fast_alloc.h"

#include "bitmap.h"
#include "fixed_alloc.h"
#include "stack_definition.h"

#include "../error.h"
#include "../os_allocator.h"

#include <pthread.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

STACK_DEFINE(CacheOffset, CacheSizeType, CacheStack)

static constexpr CacheSizeType DEFAULT_CACHE_CAPACITY = 10;

static constexpr size_t SIZE_TO_CLASS_LOOKUP_SIZE = 2UL * FA_PAGE_SIZE;
static FaSize *size_to_class_lookup = nullptr;
static FaSize *num_of_elems_per_class_lookup = nullptr;

inline static void setup_size_to_class_lookup() {
    assert(!size_to_class_lookup);

    size_to_class_lookup = (FaSize *)os_alloc(SIZE_TO_CLASS_LOOKUP_SIZE);

    if (!size_to_class_lookup) {
        fa_print_errno("os_alloc() failed in setup_size_to_class_lookup()");
        assert(false);
    }

    enum FaSizeClass current_class_entry = 0;

    for (int i = 0; i <= FA_CLASS_MAX; ++i) {
        if (i > FA_SIZES[current_class_entry]) {
            ++current_class_entry;
        }

        size_to_class_lookup[i] = current_class_entry;
    }
}

inline static void setup_num_of_elems_per_class_lookup() {
    assert(num_of_elems_per_class_lookup == nullptr);

    num_of_elems_per_class_lookup = (FaSize *)os_alloc(FA_PAGE_SIZE);

    if (!num_of_elems_per_class_lookup) {
        fa_print_errno(
            "os_alloc() failed in setup_num_of_elems_per_class_lookup()");
        assert(false);
    }

    for (enum FaSizeClass class = FA_CLASS_8; class <= FA_CLASS_1024; ++class) {
        // Splitting the buffer so it stores both the data and the bitmap.

        // num_of_elems * elem_size + ceil(num_of_elems / 8) = buff_size
        // num_of_elems * elem_size + (num_of_elems + 7) / 8 = buff_size
        // num_of_elems * elem_size + num_of_elems / 8 + 7/8 = buff_size
        // num_of_elems * (elem_size + 1/8) = buff_size - 7/8
        // num_of_elems = (buff_size - 7/8) / (elem_size + 1/8)

        static constexpr float seven_eighths = 7.0F / 8.0F;
        static constexpr float one_eighth = 1.0F / 8.0F;

        constexpr FaSize buff_size =
            FA_BLOCK_SIZE - (DEFAULT_CACHE_CAPACITY * sizeof(CacheOffset)) -
            sizeof(struct FaBlock);
        constexpr float bits_per_byte = 8.0F;
        constexpr float bitmap_elem_size = 1 / bits_per_byte;

        FaSize elem_size = FA_SIZES[class];
        FaSize num_of_elems = (FaSize)((buff_size - seven_eighths) /
                                       ((float)elem_size + one_eighth));

        num_of_elems_per_class_lookup[class] = num_of_elems;
    }
}

inline static bool is_aligned(size_t val, size_t align) {
    return (val & (align - 1)) == 0;
}

inline static void handle_incrementing_alloc_counter(struct FaBlock *block) {
    ++block->total_alloc_count;

    if (block->total_alloc_count > block->max_alloc_count) {
        // TODO: Probably can just increment max
        block->max_alloc_count = block->total_alloc_count;
    }
}

static constexpr bool SHOULD_DESTROY_BLOCK = true;

// If the ret vaule is SHOULD_DESTROY_BLOCK (aka true), the block shall be
// destroyed.
inline static bool handle_decrementing_alloc_counter(struct FaBlock *block) {
    --block->total_alloc_count;

    constexpr int block_destroy_max_allocs_threshold = 10;

    return (bool)(block->total_alloc_count == 0 &&
                  block->max_alloc_count >= block_destroy_max_allocs_threshold);
}

inline static void cross_thread_free(void *ptr);
inline static void clear_cross_thread_cache(struct FaAllocator *alloc);

inline static struct FaBlock *metadata_from_ptr(void *ptr) {
    char *ptr_down_aligned = (char *)align_down_to_block_size(ptr);

    struct FaBlock *block_metadata =
        (struct FaBlock *)(ptr_down_aligned + FA_BLOCK_SIZE -
                           sizeof(struct FaBlock));

    return block_metadata;
}

inline static bool is_ptr_in_allocator(const struct FaAllocator *alloc,
                                       void *ptr) {
    assert(alloc != nullptr);
    assert(ptr != nullptr);

    struct FaBlock *block = metadata_from_ptr(ptr);
    return block->owner == alloc;
}

inline static void *block_buff_end(const struct FaBlock *block) {
    assert(block != nullptr);

    return block->data + FA_BLOCK_SIZE - sizeof(struct FaBlock);
}

inline static bool is_ptr_in_block(const struct FaBlock *block, void *ptr) {
    return (bool)(

        (uint8_t *)ptr >= block->data && (size_t *)ptr < block->bmap.map

    );
}

inline static void block_init(struct FaAllocator *alloc, struct FaBlock *parent,
                              struct FaBlock **block, enum FaSizeClass class) {
    assert(block != nullptr);
    assert(*block == nullptr && "Block already initialized.");

    uint8_t *mem = (uint8_t *)fixed_alloc(&alloc->fixed_alloc);
    assert(mem != nullptr);

    *block = (struct FaBlock *)(mem + FA_BLOCK_SIZE) - 1;

    FaSize num_of_elems = num_of_elems_per_class_lookup[class];

    FaSize *bmap_data =
        (FaSize *)(mem + (size_t)(num_of_elems * FA_SIZES[class]));

    CacheOffset *cache_data = (CacheOffset *)(*block) - DEFAULT_CACHE_CAPACITY;

    assert(is_aligned((uintptr_t)bmap_data, sizeof(BitmapSize)));

    **block = (struct FaBlock){
        .data = mem,
        .total_alloc_count = 0,
        .max_alloc_count = 0,
        .bmap = bitmap_init(bmap_data, num_of_elems),
        .cache = CacheStack_init(cache_data, DEFAULT_CACHE_CAPACITY),
        .size_class = class,
        .next_block = nullptr,
        .prev_block = parent,
        .owner = alloc,
    };
}

inline static void block_deinit(struct FaAllocator *alloc,
                                struct FaBlock *block) {
    if (block->prev_block == nullptr) {
        alloc->blocks[block->size_class] = block->next_block;
    } else {
        block->prev_block->next_block = block->next_block;
    }

    if (block->next_block != nullptr) {
        block->next_block->prev_block = block->prev_block;
    }

    fixed_free(&alloc->fixed_alloc, block->data);
}

struct FaAllocator fa_init() {
    if (!size_to_class_lookup) {
        setup_size_to_class_lookup();
    }

    if (!num_of_elems_per_class_lookup) {
        setup_num_of_elems_per_class_lookup();
    }

    struct FixedAllocator fixed_alloc = fixed_alloc_init(FA_BLOCK_SIZE);

    struct FaAllocator alloc;
    memset((void *)alloc.blocks, 0, sizeof(alloc.blocks));
    alloc.fixed_alloc = fixed_alloc;
    pthread_mutex_init(&alloc.cross_thread_cache_lock, nullptr);
    alloc.cross_thread_cache_size = 0;

    return alloc;
}

void fa_deinit(struct FaAllocator *alloc) {
    assert(alloc != nullptr);

    fixed_alloc_deinit(&alloc->fixed_alloc);
}

void *fa_alloc(struct FaAllocator *alloc, size_t size) {
    assert(alloc != nullptr);
    assert(size <= FA_CLASS_MAX && "size too big, yet to implement");

    clear_cross_thread_cache(alloc);

    enum FaSizeClass class = size_to_class_lookup[size];

    if (!alloc->blocks[class]) {
        block_init(alloc, nullptr, &alloc->blocks[class], class);
    }

    struct FaBlock *block = alloc->blocks[class];
    assert(block != nullptr);

    while (true) {
        if (block->cache.size != 0) {
            CacheOffset offset = CacheStack_pop(&block->cache);

            size_t bitmap_index =
                (size_t)((float)offset *
                         FA_SIZE_CLASS_RECIPROCALS[block->size_class]);

            bitmap_set_to_1(&block->bmap, bitmap_index);

            handle_incrementing_alloc_counter(block);
            return block->data + offset;
        }

        size_t free_slot = bitmap_find_free_and_swap(&block->bmap);

        if (free_slot != BITMAP_NOT_FOUND) {
            handle_incrementing_alloc_counter(block);
            return (char *)block->data + (size_t)(free_slot * FA_SIZES[class]);
        }

        if (!block->next_block) {
            block_init(alloc, block, &block->next_block, class);
        }

        block = block->next_block;
    }
}

enum FaFreeRet fa_free(struct FaAllocator *alloc, void *ptr) {
    if (!alloc || !is_ptr_in_allocator(alloc, ptr)) {
        cross_thread_free(ptr);
        return PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE;
    }

    clear_cross_thread_cache(alloc);

    struct FaBlock *block = metadata_from_ptr(ptr);
    assert((uint8_t *)ptr >= block->data);

    FaSize offset = (uint8_t *)ptr - block->data;
    assert(offset <= CACHE_OFFSET_MAX);

    size_t bitmap_index =
        (size_t)((float)offset * FA_SIZE_CLASS_RECIPROCALS[block->size_class]);
    bitmap_set_to_0(&block->bmap, bitmap_index);

    CacheStack_try_push(&block->cache, (CacheOffset)offset);

    if (handle_decrementing_alloc_counter(block) == SHOULD_DESTROY_BLOCK) {
        block_deinit(alloc, block);
    }

    return OK;
}

void *fa_realloc(struct FaAllocator *alloc, void *ptr, size_t size) {
    if (!ptr) {
        return fa_alloc(alloc, size);
    }

    if (size == 0) {
        fa_free(alloc, ptr);
        return nullptr;
    }

    struct FaBlock *block = metadata_from_ptr(ptr);
    FaSize old_size = FA_SIZES[block->size_class];

    if (size <= old_size) {
        return ptr;
    }

    void *new_mem = fa_alloc(alloc, size);

    if (!new_mem) {
        return nullptr;
    }

    memcpy(new_mem, ptr, old_size);

    fa_free(alloc, ptr);

    return new_mem;
}

size_t fa_memsize(void *ptr) {
    struct FaBlock *block = metadata_from_ptr(ptr);
    return FA_SIZES[block->size_class];
}

inline static void
cross_thread_cache_push(struct FaAllocator *diff_thread_alloc, void *ptr) {
    int err_code =
        pthread_mutex_lock(&diff_thread_alloc->cross_thread_cache_lock);
    assert(err_code == 0);

    diff_thread_alloc
        ->cross_thread_cache[diff_thread_alloc->cross_thread_cache_size] = ptr;
    ++diff_thread_alloc->cross_thread_cache_size;

    err_code =
        pthread_mutex_unlock(&diff_thread_alloc->cross_thread_cache_lock);
    assert(err_code == 0);
}

inline static void *cross_thread_cache_pop(struct FaAllocator *alloc) {
    void *ret;

    int err_code = pthread_mutex_lock(&alloc->cross_thread_cache_lock);
    assert(err_code == 0);

    --alloc->cross_thread_cache_size;
    assert(alloc->cross_thread_cache_size >= 0);

    ret = alloc->cross_thread_cache[alloc->cross_thread_cache_size];

    err_code = pthread_mutex_unlock(&alloc->cross_thread_cache_lock);
    assert(err_code == 0);

    return ret;
}

inline static size_t get_cross_thread_cache_size(struct FaAllocator *alloc) {
    int err_code = pthread_mutex_lock(&alloc->cross_thread_cache_lock);
    assert(err_code == 0);

    size_t size = alloc->cross_thread_cache_size;

    err_code = pthread_mutex_unlock(&alloc->cross_thread_cache_lock);
    assert(err_code == 0);

    return size;
}

// TODO: Check if the cross thread cache is full.
inline static void cross_thread_free(void *ptr) {
    struct FaBlock *metadata = metadata_from_ptr(ptr);
    cross_thread_cache_push(metadata->owner, ptr);
}

inline static void clear_cross_thread_cache(struct FaAllocator *alloc) {
    while (get_cross_thread_cache_size(alloc) != 0) {
        enum FaFreeRet ret = fa_free(alloc, cross_thread_cache_pop(alloc));
        assert(ret != PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE);
    }
}
