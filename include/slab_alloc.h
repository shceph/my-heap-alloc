#ifndef FAST_ALLOC_H
#define FAST_ALLOC_H

#include "bitmap.h"
#include "fixed_alloc.h"
#include "stack_declaration.h"

#include <pthread.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t SlabSize;

enum SizeClass {
    SLAB_CLASS_8,
    SLAB_CLASS_16,
    SLAB_CLASS_24,
    SLAB_CLASS_32,
    SLAB_CLASS_48,
    SLAB_CLASS_64,
    SLAB_CLASS_80,
    SLAB_CLASS_96,
    SLAB_CLASS_112,
    SLAB_CLASS_128,
    SLAB_CLASS_160,
    SLAB_CLASS_192,
    SLAB_CLASS_224,
    SLAB_CLASS_256,
    SLAB_CLASS_288,
    SLAB_CLASS_320,
    SLAB_CLASS_352,
    SLAB_CLASS_384,
    SLAB_CLASS_416,
    SLAB_CLASS_448,
    SLAB_CLASS_480,
    SLAB_CLASS_512,
    SLAB_CLASS_544,
    SLAB_CLASS_576,
    SLAB_CLASS_608,
    SLAB_CLASS_640,
    SLAB_CLASS_672,
    SLAB_CLASS_704,
    SLAB_CLASS_736,
    SLAB_CLASS_768,
    SLAB_CLASS_800,
    SLAB_CLASS_832,
    SLAB_CLASS_864,
    SLAB_CLASS_896,
    SLAB_CLASS_928,
    SLAB_CLASS_960,
    SLAB_CLASS_992,
    SLAB_CLASS_1024,
    SLAB_NUM_CLASSES,
    SLAB_CLASS_INVALID,
};

#define SLAB_CLASS_MIN 8
#define SLAB_CLASS_MAX 1024

extern const SlabSize ELEMENT_SIZES[SLAB_NUM_CLASSES];

struct SlabAlloc;

typedef uint32_t CacheOffset;
typedef uint32_t CacheSizeType;

#define CACHE_OFFSET_MAX UINT32_MAX

STACK_DECLARE(CacheOffset, CacheSizeType, CacheStack)

struct Slab {
    uint8_t *data;
    uint32_t total_alloc_count;
    uint32_t max_alloc_count;
    enum SizeClass size_class;
    struct Bitmap bitmap;
    struct CacheStack cache;
    struct SlabAlloc *owner;
};

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

#endif // FAST_ALLOC_H
