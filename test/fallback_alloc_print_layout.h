#ifndef TEST_H
#define TEST_H

#include "fallback_alloc/fallback_alloc.h"

#include <stdio.h>

static inline void print_allocator_memory_layout(FallbackHeapAllocator *aloc) {
    printf("\nallocator data addr: %p\n\n", (void *)aloc->chunk_llist_head);

    for (size_t i = 0; i < aloc->region_count; ++i) {
        printf("\n======Region Number %02zu======\n\n", i);

        FallbackChunk *chunk = aloc->regions[i].begin;

        while (chunk != nullptr) {
            printf("addr: %p\n", (void *)chunk);
            printf("used: %s\n",
                   fallback_chunk_is_used(chunk) ? "true" : "false");
            printf("size: %zu (0x%lx)\n", fallback_chunk_size(chunk),
                   fallback_chunk_size(chunk));
            printf("prev: %p\n", (void *)chunk->prev);
            printf("next: %p\n\n", (void *)chunk->next);
            chunk = chunk->next;
        }
    }
}

#endif // TEST_H
