#ifndef HEAP_ALLOC_H
#define HEAP_ALLOC_H

#include "chunk.h"

#include <stdalign.h>
#include <stddef.h>

typedef struct region_t {
    chunk_t *begin;
    size_t size;
} region_t;

#define MAX_REGIONS 64UL

typedef struct heap_allocator_t {
    chunk_t *chunk_llist_head;
    region_t regions[MAX_REGIONS];
    size_t total_size;
    size_t region_count;
} heap_allocator_t;

heap_allocator_t heap_allocator_create(size_t size);
void heap_allocator_destroy(heap_allocator_t *aloc);

void *heap_alloc(heap_allocator_t *aloc, size_t size);

typedef enum split_result_t {
    SPLIT_FAILURE = 0,
    SPLIT_SUCCESS = 1
} split_result_t;
// split_size needs to be aligned by CHUNK_ALIGN.
split_result_t chunk_split_unused(chunk_t *chunk, size_t split_size);

void *heap_realloc(heap_allocator_t *aloc, void *ptr, size_t size);
void heap_free(heap_allocator_t *aloc, void *ptr);

#endif // HEAP_ALLOC_H
