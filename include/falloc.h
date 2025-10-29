#ifndef FAST_ALLOC_GLOBAL_WRAPPER_H
#define FAST_ALLOC_GLOBAL_WRAPPER_H

#include "rtree.h"
#include "slab_alloc.h"

#include "fallback_alloc/fallback_alloc.h"

#include <stddef.h>

#include <pthread.h>

struct Falloc {
    struct SlabAlloc slab_alloc;
    struct FallbackAlloc fallback_alloc;
    struct Rtree rtree;
    pthread_mutex_t lock;
    size_t size;
    void *stack[];
};

void finit();
void *falloc(size_t size);
void ffree(void *ptr);
void *frealloc(void *ptr, size_t size);
size_t fmemsize(void *ptr);
struct Falloc *falloc_get_instance();

#endif // FAST_ALLOC_GLOBAL_WRAPPER_H
