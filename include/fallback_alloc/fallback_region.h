#ifndef FALLBACK_REGION_H
#define FALLBACK_REGION_H

#include "fallback_chunk.h"

#include <stddef.h>

struct FallbackRegion {
    struct FallbackChunk *begin;
    size_t size;
};

#define FALLBACK_MAX_REGIONS 64UL

#endif // FBCK_REGION_H
