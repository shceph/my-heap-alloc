#ifndef FALLBACK_REGION_H
#define FALLBACK_REGION_H

#include "fallback_chunk.h"

#include <stddef.h>

typedef struct {
    FallbackChunk *begin;
    size_t size;
} FallbackRegion;

#define FALLBACK_MAX_REGIONS 64UL

#endif // FBCK_REGION_H
