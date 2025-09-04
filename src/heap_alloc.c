#include "heap_alloc.h"

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

heap_allocator_t heap_allocator_create(size_t size) {
    size = align_up(size);

    chunk_t *ptr = (chunk_t *)mmap(nullptr, size, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    heap_allocator_t aloc = {
        .chunk_llist_head = ptr,
        .regions[0] = {.begin = ptr, .size = size},
        .total_size = size,
        .region_count = 1,
    };

    for (size_t i = 1; i < MAX_REGIONS; ++i) {
        aloc.regions[i].begin = nullptr;
        aloc.regions[i].size = 0;
    }

    chunk_set_used(aloc.chunk_llist_head, false);
    chunk_set_size(aloc.chunk_llist_head, size);
    chunk_set_bits_to_1(aloc.chunk_llist_head, CHUNK_START_OF_REGION_BIT);
    aloc.chunk_llist_head->next = nullptr;
    aloc.chunk_llist_head->prev = nullptr;

    return aloc;
}

void heap_allocator_destroy(heap_allocator_t *aloc) {
    for (size_t i = 0; i < MAX_REGIONS; ++i) {
        int ret = munmap(aloc->regions[i].begin, aloc->regions[i].size);

        if (ret != 0) {
            fprintf(stderr,
                    "heap_allocator_destroy: Error unmaping region with begin "
                    "= %p\n",
                    (void *)aloc->regions[i].begin);
            fprintf(stderr, "Error message: %s\n", strerror(errno));
        }
    }
}

static bool heap_allocator_add_region(heap_allocator_t *aloc,
                                      size_t needed_size) {
    if (aloc->region_count >= MAX_REGIONS) {
        fprintf(stderr,
                "heap_allocator_add_region: No more regions available.\n");
        return false;
    }

    size_t new_reg_size =
        max_size(aloc->total_size, needed_size + sizeof(chunk_t));
    chunk_t *ptr =
        (chunk_t *)mmap(nullptr, new_reg_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == nullptr) {
        return false;
    }

    aloc->regions[aloc->region_count].begin = ptr;
    aloc->regions[aloc->region_count].size = new_reg_size;

    chunk_t *prev_reg_begin = aloc->regions[aloc->region_count - 1].begin;

    // TODO: This sucks, must be fixed
    while (prev_reg_begin->next != nullptr) {
        prev_reg_begin = prev_reg_begin->next;
    }

    prev_reg_begin->next = ptr;

    chunk_set_size(ptr, new_reg_size);
    chunk_reset_flags(ptr);
    chunk_set_bits_to_1(ptr, CHUNK_START_OF_REGION_BIT);
    chunk_set_used(ptr, false);
    ptr->next = nullptr;
    ptr->prev = nullptr;

    aloc->total_size += new_reg_size;
    ++aloc->region_count;

    return true;
}

void *heap_alloc(heap_allocator_t *aloc, size_t size) {
    if (aloc == nullptr || size == 0) {
        return nullptr;
    }

    size = align_up(size);

    chunk_t *chunk = aloc->chunk_llist_head;

    while (chunk != nullptr) {
        if (chunk_split_unused(chunk, size) == SPLIT_FAILURE) {
            chunk = chunk->next;
            continue;
        }

        return (void *)(chunk + 1);
    }

    if (!heap_allocator_add_region(aloc, size)) {
        return nullptr;
    }

    chunk = aloc->regions[aloc->region_count - 1].begin;

    if (chunk_split_unused(chunk, size) == SPLIT_SUCCESS) {
        return (void *)(chunk + 1);
    }

    return nullptr;
}

split_result_t chunk_split_unused(chunk_t *chunk, size_t split_size) {
    if (chunk_is_used(chunk)) {
        return SPLIT_FAILURE;
    }

    size_t new_chunk_size = split_size + sizeof(chunk_t);

    if (new_chunk_size > chunk_size(chunk)) {
        return SPLIT_FAILURE;
    }

    if (chunk_size(chunk) - new_chunk_size < MIN_CHUNK_SIZE) {
        chunk_set_used(chunk, true);
        return SPLIT_SUCCESS;
    }

    chunk_t *curr_chunk_new_location =
        (chunk_t *)((char *)chunk + new_chunk_size);
    chunk_reset_flags(curr_chunk_new_location);
    chunk_set_size(curr_chunk_new_location, chunk_size(chunk) - new_chunk_size);
    curr_chunk_new_location->next = chunk->next;
    curr_chunk_new_location->prev = chunk;

    chunk_set_used(chunk, true);
    chunk_set_size(chunk, new_chunk_size);
    chunk->next = curr_chunk_new_location;

    return SPLIT_SUCCESS;
}

void *heap_realloc(heap_allocator_t *aloc, void *ptr, size_t size) {
    if (aloc == nullptr) {
        return nullptr;
    }

    chunk_t *chunk_to_realloc = (chunk_t *)ptr - 1;
    size_t chunk_sz = chunk_size(chunk_to_realloc) - sizeof(chunk_t);

    if (chunk_sz == size) {
        return ptr;
    }

    size_t data_to_cpy_sz = min_size(chunk_sz, size);

    heap_free(aloc, ptr);
    void *new_mem = heap_alloc(aloc, size);

    if (new_mem == ptr) {
        return new_mem;
    }

    memmove(new_mem, ptr, data_to_cpy_sz);

    return new_mem;
}

void heap_free(heap_allocator_t *aloc, void *ptr) {
    if (aloc == nullptr || ptr == nullptr) {
        return;
    }

    chunk_t *chunk = (chunk_t *)ptr - 1;
    chunk_t *child = chunk->next;
    chunk_t *parent = chunk->prev;

    chunk_set_used(chunk, false);

    if (child != nullptr && !chunk_is_used(child) &&
        !chunk_get_bit(child, CHUNK_START_OF_REGION_BIT)) {
        chunk->next = child->next;

        if (chunk->next != nullptr) {
            chunk->next->prev = chunk;
        }

        chunk_set_size(chunk, chunk_size(chunk) + chunk_size(child));
    }

    if (parent != nullptr && !chunk_is_used(parent)) {
        parent->next = chunk->next;

        if (parent->next != nullptr) {
            parent->next->prev = parent;
        }

        chunk_set_size(parent, chunk_size(parent) + chunk_size(chunk));
    }
}
