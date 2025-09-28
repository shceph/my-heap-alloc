#ifndef FAST_ALLOCATOR_H
#define FAST_ALLOCATOR_H

#include "block_allocator.h"

#include <pthread.h>

#include <stddef.h>
#include <stdint.h>

typedef uint32_t FastAllocSize;

constexpr FastAllocSize FAST_ALLOC_PAGE_SIZE = 0x1000;

typedef enum {
    FAST_ALLOC_CLASS_8,
    FAST_ALLOC_CLASS_16,
    FAST_ALLOC_CLASS_24,
    FAST_ALLOC_CLASS_32,
    FAST_ALLOC_CLASS_48,
    FAST_ALLOC_CLASS_64,
    FAST_ALLOC_CLASS_80,
    FAST_ALLOC_CLASS_96,
    FAST_ALLOC_CLASS_112,
    FAST_ALLOC_CLASS_128,
    FAST_ALLOC_CLASS_160,
    FAST_ALLOC_CLASS_192,
    FAST_ALLOC_CLASS_224,
    FAST_ALLOC_CLASS_256,
    FAST_ALLOC_CLASS_288,
    FAST_ALLOC_CLASS_320,
    FAST_ALLOC_CLASS_352,
    FAST_ALLOC_CLASS_384,
    FAST_ALLOC_CLASS_416,
    FAST_ALLOC_CLASS_448,
    FAST_ALLOC_CLASS_480,
    FAST_ALLOC_CLASS_512,
    FAST_ALLOC_CLASS_544,
    FAST_ALLOC_CLASS_576,
    FAST_ALLOC_CLASS_608,
    FAST_ALLOC_CLASS_640,
    FAST_ALLOC_CLASS_672,
    FAST_ALLOC_CLASS_704,
    FAST_ALLOC_CLASS_736,
    FAST_ALLOC_CLASS_768,
    FAST_ALLOC_CLASS_800,
    FAST_ALLOC_CLASS_832,
    FAST_ALLOC_CLASS_864,
    FAST_ALLOC_CLASS_896,
    FAST_ALLOC_CLASS_928,
    FAST_ALLOC_CLASS_960,
    FAST_ALLOC_CLASS_992,
    FAST_ALLOC_CLASS_1024,
    FAST_ALLOC_NUM_CLASSES,
    FAST_ALLOC_CLASS_INVALID,
} FastAllocSizeClass;

constexpr FastAllocSize FAST_ALLOC_CLASS_MIN = 8;
constexpr FastAllocSize FAST_ALLOC_CLASS_MAX = 1024;

constexpr FastAllocSizeClass FAST_ALLOC_SIZES[FAST_ALLOC_NUM_CLASSES] = {
    [FAST_ALLOC_CLASS_8] = 8,     [FAST_ALLOC_CLASS_16] = 16,
    [FAST_ALLOC_CLASS_24] = 24,   [FAST_ALLOC_CLASS_32] = 32,
    [FAST_ALLOC_CLASS_48] = 48,   [FAST_ALLOC_CLASS_64] = 64,
    [FAST_ALLOC_CLASS_80] = 80,   [FAST_ALLOC_CLASS_96] = 96,
    [FAST_ALLOC_CLASS_112] = 112, [FAST_ALLOC_CLASS_128] = 128,
    [FAST_ALLOC_CLASS_160] = 160, [FAST_ALLOC_CLASS_192] = 192,
    [FAST_ALLOC_CLASS_224] = 224, [FAST_ALLOC_CLASS_256] = 256,
    [FAST_ALLOC_CLASS_288] = 288, [FAST_ALLOC_CLASS_320] = 320,
    [FAST_ALLOC_CLASS_352] = 352, [FAST_ALLOC_CLASS_384] = 384,
    [FAST_ALLOC_CLASS_416] = 416, [FAST_ALLOC_CLASS_448] = 448,
    [FAST_ALLOC_CLASS_480] = 480, [FAST_ALLOC_CLASS_512] = 512,
    [FAST_ALLOC_CLASS_544] = 544, [FAST_ALLOC_CLASS_576] = 576,
    [FAST_ALLOC_CLASS_608] = 608, [FAST_ALLOC_CLASS_640] = 640,
    [FAST_ALLOC_CLASS_672] = 672, [FAST_ALLOC_CLASS_704] = 704,
    [FAST_ALLOC_CLASS_736] = 736, [FAST_ALLOC_CLASS_768] = 768,
    [FAST_ALLOC_CLASS_800] = 800, [FAST_ALLOC_CLASS_832] = 832,
    [FAST_ALLOC_CLASS_864] = 864, [FAST_ALLOC_CLASS_896] = 896,
    [FAST_ALLOC_CLASS_928] = 928, [FAST_ALLOC_CLASS_960] = 960,
    [FAST_ALLOC_CLASS_992] = 992, [FAST_ALLOC_CLASS_1024] = 1024,
};

struct FastAllocator;

typedef struct FastAllocBlock {
    uint8_t *data;
    FastAllocSize *cache;
    FastAllocSize data_size;
    FastAllocSize cache_size;
    FastAllocSizeClass size_class;
    struct FastAllocBlock *next_block;
    struct FastAllocator *owner;
} FastAllocBlock;

typedef struct FastAllocator {
    FastAllocBlock *blocks[FAST_ALLOC_NUM_CLASSES];
    BlockAllocator block_alloc;
    pthread_mutex_t cross_thread_cache_lock;
    size_t cross_thread_cache_size;
    void *cross_thread_cache[];
} FastAllocator;

FastAllocator fast_alloc_init();
void fast_alloc_deinit(FastAllocator *alloc);
void *fast_alloc_alloc(FastAllocator *alloc, size_t size);

typedef enum {
    OK,
    PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE,
} FastAllocFreeRet;

FastAllocFreeRet fast_alloc_free(FastAllocator *alloc, void *ptr);

#endif // FAST_ALLOCATOR_H
