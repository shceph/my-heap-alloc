#ifndef HEAP_ALLOC_H
#define HEAP_ALLOC_H

#include <stddef.h>

typedef struct chunk_llist {
    bool used;
    size_t size;
    struct chunk_llist *next;
} chunk_llist_t;

typedef struct heap_allocator {
    size_t size;
    void *begin;
    chunk_llist_t *chunk_llist_head;
} heap_allocator_t;

heap_allocator_t heap_allocator_create(size_t size);
void *heap_alloc(heap_allocator_t *allocator, size_t size);
void heap_free(heap_allocator_t *allocator, void *ptr);

#endif
