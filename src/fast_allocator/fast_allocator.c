#include "fast_allocator.h"

#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static constexpr size_t SIZE_TO_CLASS_LOOKUP_SIZE = 2UL * FAST_ALLOC_PAGE_SIZE;
static FastAllocSize *size_to_class_lookup = nullptr;

inline static FastAllocSize block_size_per_class(FastAllocSizeClass class) {
    return FAST_ALLOC_PAGE_SIZE * FAST_ALLOC_PAGES_PER_CLASS[class];
}

inline static void *block_buff_end(const FastAllocBlock *block) {
    assert(block != nullptr);

    return block->data + block_size_per_class(block->size_class) -
           sizeof(FastAllocBlock);
}

inline static uint8_t *block_last_elem(const FastAllocBlock *block) {
    assert(block->data_size != 0);

    return block->data + ((size_t)((block->data_size - 1) *
                                   FAST_ALLOC_SIZES[block->size_class]));
}

inline static bool block_is_full(const FastAllocBlock *block) {
    return (bool)(

        block->data_size != 0 &&
        (block_last_elem(block) + FAST_ALLOC_SIZES[block->size_class] >=
         (uint8_t *)block->cache)

    );
}

inline static void cache_push(FastAllocBlock *block, FastAllocSize val) {
    block->cache[block->cache_size] = val;
    ++block->cache_size;
}

inline static FastAllocSize cache_pop(FastAllocBlock *block) {
    --block->cache_size;
    return block->cache[block->cache_size];
}

inline static void init_block(FastAllocBlock **block,
                              FastAllocSizeClass class) {
    assert(block != nullptr);
    assert(*block == nullptr && "Block already initialized.");

    FastAllocSize sz_to_alloc = block_size_per_class(class);
    uint8_t *mem = (uint8_t *)os_alloc(sz_to_alloc);

    assert(mem != nullptr);

    *block = (FastAllocBlock *)(mem + sz_to_alloc - sizeof(FastAllocBlock));

    // Splitting the buffer so it stores both the data and the cache. The cache
    // is just an array of indices. Each index is the offset from the pointer
    // to the memory used for allocation.
    // num_of_elems * elem_size + num_of_elems * cache_idx_size = buff_size
    // => num_of_elems = buff_size / (elem_size + cache_idx_size),
    // where cache_idx_size is sizeof(FastAllocSize).
    // So, the pointer to the start of the cache is data + num_of_elems *
    // elem_size.

    FastAllocSize buff_size = sz_to_alloc - sizeof(FastAllocBlock);
    FastAllocSize elem_size = FAST_ALLOC_SIZES[class];
    FastAllocSize cache_idx_size = sizeof(FastAllocSize);

    FastAllocSize num_of_elems = buff_size / (elem_size + cache_idx_size);

    FastAllocSize *cache =
        (FastAllocSize *)(mem + (size_t)(num_of_elems * elem_size));

    **block = (FastAllocBlock){
        .data = mem,
        .cache = cache,
        .data_size = 0,
        .cache_size = 0,
        .size_class = class,
        .next_block = nullptr,
    };
}

FastAllocator fast_alloc_init() {
    if (size_to_class_lookup == nullptr) {
        size_to_class_lookup = (uint32_t *)os_alloc(SIZE_TO_CLASS_LOOKUP_SIZE);
    }

    assert(size_to_class_lookup != nullptr);

    FastAllocSizeClass current_class_entry = 0;

    for (int i = 0; i <= FAST_ALLOC_CLASS_MAX; ++i) {
        if (i > FAST_ALLOC_SIZES[current_class_entry]) {
            ++current_class_entry;
        }

        size_to_class_lookup[i] = current_class_entry;
    }

    FastAllocator alloc;
    memset((void *)alloc.blocks, 0, sizeof(alloc.blocks));
    return alloc;
}

void fast_alloc_deinit(FastAllocator *alloc) {
    assert(alloc != nullptr);

    for (int i = 0; i < FAST_ALLOC_NUM_CLASSES; ++i) {
        FastAllocBlock *block = alloc->blocks[i];

        while (block != nullptr) {
            FastAllocBlock *next = block->next_block;

            FastAllocSize sz = block_size_per_class(block->size_class);

            os_free(block->data, sz);

            block = next;
        }
    }

    os_free(size_to_class_lookup, SIZE_TO_CLASS_LOOKUP_SIZE);
}

void *fast_alloc_alloc(FastAllocator *alloc, size_t size) {
    assert(alloc != nullptr);

    FastAllocSizeClass entry = size_to_class_lookup[size];

    if (entry == FAST_ALLOC_CLASS_INVALID) {
        // TODO: Use fallback allocator
        assert(false && "size too big, yet to implement");
    }

    if (alloc->blocks[entry] == nullptr) {
        init_block(&alloc->blocks[entry], entry);
    }

    FastAllocBlock *block = alloc->blocks[entry];

    while (true) {
        if (block->cache_size != 0) {
            return block->data + cache_pop(block);
        }

        if (!block_is_full(block)) {
            ++block->data_size;
            return block_last_elem(block);
        }

        if (block->next_block == nullptr) {
            init_block(&block->next_block, block->size_class);
        }

        block = block->next_block;
    }
}

void fast_alloc_free(FastAllocator *alloc, void *ptr, size_t size) {
    assert(alloc != nullptr);

    if (ptr == nullptr) {
        return;
    }

    FastAllocSizeClass class = size_to_class_lookup[size];
    FastAllocBlock *block = alloc->blocks[class];

    assert((uint8_t *)ptr >= block->data);

    FastAllocSize offset = (uint8_t *)ptr - block->data;
    cache_push(block, offset);
}
