#include "fallback_allocator.h"

#include "fallback_chunk.h"
#include "fallback_region.h"

#include <sys/mman.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

inline static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

inline static size_t max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

FallbackHeapAllocator fallback_allocator_create(size_t size) {
    size = fallback_align_up(size);

    FallbackChunk *ptr =
        (FallbackChunk *)mmap(nullptr, size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        (void)fprintf(stderr, "mmap call failed, error message: %s\n",
                      strerror(errno));
    }

    FallbackHeapAllocator aloc = {
        .chunk_llist_head = ptr,
        .regions[0] = {.begin = ptr, .size = size},
        .total_size = size,
        .region_count = 1,
    };

    for (size_t i = 1; i < FALLBACK_MAX_REGIONS; ++i) {
        aloc.regions[i].begin = nullptr;
        aloc.regions[i].size = 0;
    }

    aloc.chunk_llist_head->attr = 0;
    fallback_chunk_set_used(aloc.chunk_llist_head, false);
    fallback_chunk_set_size(aloc.chunk_llist_head, size);
    aloc.chunk_llist_head->next = nullptr;
    aloc.chunk_llist_head->prev = nullptr;

    return aloc;
}

void fallback_allocator_destroy(FallbackHeapAllocator *aloc) {
    for (size_t i = 0; i < FALLBACK_MAX_REGIONS; ++i) {
        int ret = munmap(aloc->regions[i].begin, aloc->regions[i].size);

        if (ret != 0) {
            (void)fprintf(
                stderr,
                "fbck_allocator_destroy: Error unmaping region with begin "
                "= %p\n",
                (void *)aloc->regions[i].begin);
            (void)fprintf(stderr, "Error message: %s\n", strerror(errno));
        }
    }
}

static bool fbck_allocator_add_region(FallbackHeapAllocator *aloc,
                                      size_t needed_size) {
    if (aloc->region_count >= FALLBACK_MAX_REGIONS) {
        (void)fprintf(
            stderr, "fbck_allocator_add_region: No more regions available.\n");
        return false;
    }

    size_t new_reg_size =
        max_size(aloc->total_size, needed_size + sizeof(FallbackChunk));
    FallbackChunk *ptr =
        (FallbackChunk *)mmap(nullptr, new_reg_size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == nullptr) {
        return false;
    }

    aloc->regions[aloc->region_count].begin = ptr;
    aloc->regions[aloc->region_count].size = new_reg_size;

    fallback_chunk_set_size(ptr, new_reg_size);
    fallback_chunk_set_used(ptr, false);
    ptr->next = nullptr;
    ptr->prev = nullptr;

    aloc->total_size += new_reg_size;
    ++aloc->region_count;

    return true;
}

void *fallback_alloc(FallbackHeapAllocator *aloc, size_t size) {
    if (aloc == nullptr || size == 0) {
        return nullptr;
    }

    size = fallback_align_up(size);

    FallbackChunk *chunk = nullptr;

    for (size_t i = 0; i < aloc->region_count; ++i) {
        chunk = aloc->regions[i].begin;

        while (chunk != nullptr) {
            if (fallback_chunk_split_unused(chunk, size) ==
                FALLBACK_SPLIT_FAILURE) {
                chunk = chunk->next;
                continue;
            }

            return (void *)(chunk + 1);
        }
    }

    if (!fbck_allocator_add_region(aloc, size)) {
        return nullptr;
    }

    chunk = aloc->regions[aloc->region_count - 1].begin;

    if (fallback_chunk_split_unused(chunk, size) == FALLBACK_SPLIT_SUCCESS) {
        return (void *)(chunk + 1);
    }

    return nullptr;
}

FallbackSplitResult fallback_chunk_split_unused(FallbackChunk *chunk,
                                                size_t split_size) {
    if (fallback_chunk_is_used(chunk)) {
        return FALLBACK_SPLIT_FAILURE;
    }

    size_t new_chunk_size = split_size + sizeof(FallbackChunk);

    if (new_chunk_size > fallback_chunk_size(chunk)) {
        return FALLBACK_SPLIT_FAILURE;
    }

    if (fallback_chunk_size(chunk) - new_chunk_size < FALLBACK_MIN_CHUNK_SIZE) {
        fallback_chunk_set_used(chunk, true);
        return FALLBACK_SPLIT_SUCCESS;
    }

    FallbackChunk *curr_chunk_new_location =
        (FallbackChunk *)((char *)chunk + new_chunk_size);
    fallback_chunk_reset_flags(curr_chunk_new_location);
    fallback_chunk_set_size(curr_chunk_new_location,
                            fallback_chunk_size(chunk) - new_chunk_size);
    curr_chunk_new_location->next = chunk->next;
    curr_chunk_new_location->prev = chunk;

    fallback_chunk_set_used(chunk, true);
    fallback_chunk_set_size(chunk, new_chunk_size);
    chunk->next = curr_chunk_new_location;

    return FALLBACK_SPLIT_SUCCESS;
}

void *fallback_realloc(FallbackHeapAllocator *aloc, void *ptr, size_t size) {
    if (aloc == nullptr) {
        return nullptr;
    }

    FallbackChunk *chunk_to_realloc = (FallbackChunk *)ptr - 1;
    size_t chunk_sz =
        fallback_chunk_size(chunk_to_realloc) - sizeof(FallbackChunk);

    if (chunk_sz == size) {
        return ptr;
    }

    size_t data_to_cpy_sz = min_size(chunk_sz, size);

    fallback_free(aloc, ptr);
    void *new_mem = fallback_alloc(aloc, size);

    if (new_mem == ptr) {
        return new_mem;
    }

    memmove(new_mem, ptr, data_to_cpy_sz);

    return new_mem;
}

void fallback_free(FallbackHeapAllocator *aloc, void *ptr) {
    if (aloc == nullptr || ptr == nullptr) {
        return;
    }

    FallbackChunk *chunk = (FallbackChunk *)ptr - 1;
    FallbackChunk *child = chunk->next;
    FallbackChunk *parent = chunk->prev;

    fallback_chunk_set_used(chunk, false);

    if (child != nullptr && !fallback_chunk_is_used(child)) {
        chunk->next = child->next;

        if (chunk->next != nullptr) {
            chunk->next->prev = chunk;
        }

        fallback_chunk_set_size(chunk, fallback_chunk_size(chunk) +
                                           fallback_chunk_size(child));
    }

    if (parent != nullptr && !fallback_chunk_is_used(parent)) {
        parent->next = chunk->next;

        if (parent->next != nullptr) {
            parent->next->prev = parent;
        }

        fallback_chunk_set_size(parent, fallback_chunk_size(parent) +
                                            fallback_chunk_size(chunk));
    }
}
