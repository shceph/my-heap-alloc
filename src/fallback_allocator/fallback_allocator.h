#ifndef FALLBACK_ALLOCATOR_H
#define FALLBACK_ALLOCATOR_H

#include "fallback_chunk.h"
#include "fallback_region.h"

#include <stdalign.h>
#include <stddef.h>

typedef struct {
    FallbackChunk *chunk_llist_head;
    FallbackRegion regions[FALLBACK_MAX_REGIONS];
    size_t total_size;
    size_t region_count;
} FallbackHeapAllocator;

FallbackHeapAllocator fallback_allocator_create(size_t size);
void fallback_allocator_destroy(FallbackHeapAllocator *aloc);

void *fallback_alloc(FallbackHeapAllocator *aloc, size_t size);

typedef enum {
    FALLBACK_SPLIT_FAILURE = 0,
    FALLBACK_SPLIT_SUCCESS = 1
} FallbackSplitResult;
// split_size needs to be aligned by CHUNK_ALIGN.
FallbackSplitResult fallback_chunk_split_unused(FallbackChunk *chunk,
                                                size_t split_size);

void *fallback_realloc(FallbackHeapAllocator *aloc, void *ptr, size_t size);
void fallback_free(FallbackHeapAllocator *aloc, void *ptr);

#endif // FALLBACK_ALLOCATOR_H
