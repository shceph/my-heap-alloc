#ifndef FAST_ALLOC_H
#define FAST_ALLOC_H

#include "bitmap.h"
#include "fixed_alloc.h"
#include "stack_declaration.h"

#include <pthread.h>

#include <stddef.h>
#include <stdint.h>

typedef uint32_t SlabSize;

constexpr SlabSize FA_PAGE_SIZE = 0x1000;

enum SlabSizeClass {
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

constexpr SlabSize SLAB_CLASS_MIN = 8;
constexpr SlabSize SLAB_CLASS_MAX = 1024;

constexpr SlabSize SLAB_SIZES[SLAB_NUM_CLASSES] = {
    [SLAB_CLASS_8] = 8,     [SLAB_CLASS_16] = 16,     [SLAB_CLASS_24] = 24,
    [SLAB_CLASS_32] = 32,   [SLAB_CLASS_48] = 48,     [SLAB_CLASS_64] = 64,
    [SLAB_CLASS_80] = 80,   [SLAB_CLASS_96] = 96,     [SLAB_CLASS_112] = 112,
    [SLAB_CLASS_128] = 128, [SLAB_CLASS_160] = 160,   [SLAB_CLASS_192] = 192,
    [SLAB_CLASS_224] = 224, [SLAB_CLASS_256] = 256,   [SLAB_CLASS_288] = 288,
    [SLAB_CLASS_320] = 320, [SLAB_CLASS_352] = 352,   [SLAB_CLASS_384] = 384,
    [SLAB_CLASS_416] = 416, [SLAB_CLASS_448] = 448,   [SLAB_CLASS_480] = 480,
    [SLAB_CLASS_512] = 512, [SLAB_CLASS_544] = 544,   [SLAB_CLASS_576] = 576,
    [SLAB_CLASS_608] = 608, [SLAB_CLASS_640] = 640,   [SLAB_CLASS_672] = 672,
    [SLAB_CLASS_704] = 704, [SLAB_CLASS_736] = 736,   [SLAB_CLASS_768] = 768,
    [SLAB_CLASS_800] = 800, [SLAB_CLASS_832] = 832,   [SLAB_CLASS_864] = 864,
    [SLAB_CLASS_896] = 896, [SLAB_CLASS_928] = 928,   [SLAB_CLASS_960] = 960,
    [SLAB_CLASS_992] = 992, [SLAB_CLASS_1024] = 1024,
};

// Using this to multiply with reciprocals instead of dividing, which is faster.
constexpr float SLAB_SIZE_CLASS_RECIPROCALS[SLAB_NUM_CLASSES] = {
    [SLAB_CLASS_8] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_8],
    [SLAB_CLASS_16] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_16],
    [SLAB_CLASS_24] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_24],
    [SLAB_CLASS_32] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_32],
    [SLAB_CLASS_48] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_48],
    [SLAB_CLASS_64] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_64],
    [SLAB_CLASS_80] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_80],
    [SLAB_CLASS_96] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_96],
    [SLAB_CLASS_112] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_112],
    [SLAB_CLASS_128] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_128],
    [SLAB_CLASS_160] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_160],
    [SLAB_CLASS_192] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_192],
    [SLAB_CLASS_224] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_224],
    [SLAB_CLASS_256] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_256],
    [SLAB_CLASS_288] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_288],
    [SLAB_CLASS_320] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_320],
    [SLAB_CLASS_352] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_352],
    [SLAB_CLASS_384] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_384],
    [SLAB_CLASS_416] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_416],
    [SLAB_CLASS_448] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_448],
    [SLAB_CLASS_480] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_480],
    [SLAB_CLASS_512] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_512],
    [SLAB_CLASS_544] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_544],
    [SLAB_CLASS_576] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_576],
    [SLAB_CLASS_608] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_608],
    [SLAB_CLASS_640] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_640],
    [SLAB_CLASS_672] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_672],
    [SLAB_CLASS_704] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_704],
    [SLAB_CLASS_736] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_736],
    [SLAB_CLASS_768] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_768],
    [SLAB_CLASS_800] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_800],
    [SLAB_CLASS_832] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_832],
    [SLAB_CLASS_864] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_864],
    [SLAB_CLASS_896] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_896],
    [SLAB_CLASS_928] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_928],
    [SLAB_CLASS_960] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_960],
    [SLAB_CLASS_992] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_992],
    [SLAB_CLASS_1024] = 1.0F / (float)SLAB_SIZES[SLAB_CLASS_1024],
};

struct SlabAlloc;

typedef uint16_t CacheOffset;
typedef uint16_t CacheSizeType;

constexpr CacheOffset CACHE_OFFSET_MAX = UINT16_MAX;

STACK_DECLARE(CacheOffset, CacheSizeType, CacheStack)

struct Slab {
    uint8_t *data;
    uint32_t total_alloc_count;
    uint32_t max_alloc_count;
    enum SlabSizeClass size_class;
    struct Bitmap bmap;
    struct CacheStack cache;
    struct Slab *next_slab;
    struct Slab *prev_slab;
    struct SlabAlloc *owner;
};

struct Falloc;

struct SlabAlloc {
    struct Slab *slabs[SLAB_NUM_CLASSES];
    struct FixedAllocator fixed_alloc;
    struct Falloc *owner;
};

struct Slab *slab_from_ptr(void *ptr);
bool slab_alloc_is_ptr_in_this_instance(const struct SlabAlloc *alloc,
                                        void *ptr);

struct SlabAlloc slab_alloc_init(struct Falloc *owner);
void slab_alloc_deinit(struct SlabAlloc *alloc);
void *slab_alloc(struct SlabAlloc *alloc, size_t size);

enum FaFreeRet {
    OK,
    PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE,
};

enum FaFreeRet slab_free(struct SlabAlloc *alloc, void *ptr);
void *slab_realloc(struct SlabAlloc *alloc, void *ptr, size_t size);
size_t slab_memsize(void *ptr);

#endif // FAST_ALLOC_H
