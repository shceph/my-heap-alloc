#ifndef FALLBACK_ALLOCATOR_H
#define FALLBACK_ALLOCATOR_H

#include "fallback_chunk.h"
#include "fallback_region.h"

#include <stdalign.h>
#include <stddef.h>

struct FallbackAlloc {
    struct FallbackChunk *chunk_llist_head;
    struct FallbackRegion regions[FALLBACK_MAX_REGIONS];
    size_t total_size;
    size_t region_count;
};

struct FallbackAlloc fallback_allocator_create(size_t size);
void fallback_allocator_destroy(struct FallbackAlloc *aloc);

void *fallback_alloc(struct FallbackAlloc *aloc, size_t size);

enum FallbackSplitResult {
    FALLBACK_SPLIT_FAILURE = 0,
    FALLBACK_SPLIT_SUCCESS = 1
};
// split_size needs to be aligned by CHUNK_ALIGN.
enum FallbackSplitResult
fallback_chunk_split_unused(struct FallbackChunk *chunk, size_t split_size);

void *fallback_realloc(struct FallbackAlloc *aloc, void *ptr, size_t size);
void fallback_free(struct FallbackAlloc *aloc, void *ptr);

#endif // FALLBACK_ALLOCATOR_H
