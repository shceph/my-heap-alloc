#ifndef SLAB_ALLOC_H
#define SLAB_ALLOC_H

#include "slab_alloc/slab.h"

#include "fixed_alloc.h"

#include <pthread.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SlabPool {
    size_t len;
    size_t cap;
    enum SizeClass size_class;
    struct SlabPool *next;
    alignas(sizeof(struct Slab)) struct Slab slabs[];
};

struct Falloc;

struct SlabAlloc {
    struct SlabPool *pools[SLAB_NUM_CLASSES];
    struct FixedAllocator slab_store;
    struct FixedAllocator pool_store;
    struct Falloc *owner;
};

struct Slab *slab_from_ptr(void *ptr);
bool slab_alloc_is_ptr_in_instance(const struct SlabAlloc *alloc, void *ptr);

struct SlabAlloc slab_alloc_init(struct Falloc *owner);
void slab_alloc_deinit(struct SlabAlloc *alloc);
void *slab_alloc(struct SlabAlloc *alloc, size_t size);
void slab_free(struct SlabAlloc *alloc, void *ptr);
void *slab_realloc(struct SlabAlloc *alloc, void *ptr, size_t size);
size_t slab_memsize(void *ptr);

#endif // SLAB_ALLOC_H
