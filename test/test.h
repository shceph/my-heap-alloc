#ifndef TEST_H
#define TEST_H

#include "../src/heap_alloc.h"

#include <stdio.h>

void print_allocator_memory_layout(heap_allocator_t *aloc) {
    printf("\nallocator data addr: %p\n\n", (void *)aloc->chunk_llist_head);
    chunk_t *chunk = aloc->chunk_llist_head;

    while (chunk != nullptr) {
        if (chunk_get_bit(chunk, CHUNK_START_OF_REGION_BIT)) {
            printf("\n======New Region======\n\n");
        }

        printf("addr: %p\n", (void *)chunk);
        printf("used: %s\n", (int)chunk_is_used(chunk) ? "true" : "false");
        printf("size: %zu (0x%lx)\n", chunk_size(chunk), chunk_size(chunk));
        printf("prev: %p\n", (void *)chunk->prev);
        printf("next: %p\n\n", (void *)chunk->next);
        chunk = chunk->next;
    }
}

#endif // TEST_H
