#ifndef FAST_ALLOCATOR_H
#define FAST_ALLOCATOR_H

#include "bitmap.h"
#include "block_allocator.h"
#include "stack_declaration.h"

#include <pthread.h>

#include <stddef.h>
#include <stdint.h>

typedef uint32_t FaSize;

constexpr FaSize FA_PAGE_SIZE = 0x1000;

enum FaSizeClass {
    FA_CLASS_8,
    FA_CLASS_16,
    FA_CLASS_24,
    FA_CLASS_32,
    FA_CLASS_48,
    FA_CLASS_64,
    FA_CLASS_80,
    FA_CLASS_96,
    FA_CLASS_112,
    FA_CLASS_128,
    FA_CLASS_160,
    FA_CLASS_192,
    FA_CLASS_224,
    FA_CLASS_256,
    FA_CLASS_288,
    FA_CLASS_320,
    FA_CLASS_352,
    FA_CLASS_384,
    FA_CLASS_416,
    FA_CLASS_448,
    FA_CLASS_480,
    FA_CLASS_512,
    FA_CLASS_544,
    FA_CLASS_576,
    FA_CLASS_608,
    FA_CLASS_640,
    FA_CLASS_672,
    FA_CLASS_704,
    FA_CLASS_736,
    FA_CLASS_768,
    FA_CLASS_800,
    FA_CLASS_832,
    FA_CLASS_864,
    FA_CLASS_896,
    FA_CLASS_928,
    FA_CLASS_960,
    FA_CLASS_992,
    FA_CLASS_1024,
    FA_NUM_CLASSES,
    FA_CLASS_INVALID,
};

constexpr FaSize FA_CLASS_MIN = 8;
constexpr FaSize FA_CLASS_MAX = 1024;

constexpr FaSize FA_SIZES[FA_NUM_CLASSES] = {
    [FA_CLASS_8] = 8,     [FA_CLASS_16] = 16,     [FA_CLASS_24] = 24,
    [FA_CLASS_32] = 32,   [FA_CLASS_48] = 48,     [FA_CLASS_64] = 64,
    [FA_CLASS_80] = 80,   [FA_CLASS_96] = 96,     [FA_CLASS_112] = 112,
    [FA_CLASS_128] = 128, [FA_CLASS_160] = 160,   [FA_CLASS_192] = 192,
    [FA_CLASS_224] = 224, [FA_CLASS_256] = 256,   [FA_CLASS_288] = 288,
    [FA_CLASS_320] = 320, [FA_CLASS_352] = 352,   [FA_CLASS_384] = 384,
    [FA_CLASS_416] = 416, [FA_CLASS_448] = 448,   [FA_CLASS_480] = 480,
    [FA_CLASS_512] = 512, [FA_CLASS_544] = 544,   [FA_CLASS_576] = 576,
    [FA_CLASS_608] = 608, [FA_CLASS_640] = 640,   [FA_CLASS_672] = 672,
    [FA_CLASS_704] = 704, [FA_CLASS_736] = 736,   [FA_CLASS_768] = 768,
    [FA_CLASS_800] = 800, [FA_CLASS_832] = 832,   [FA_CLASS_864] = 864,
    [FA_CLASS_896] = 896, [FA_CLASS_928] = 928,   [FA_CLASS_960] = 960,
    [FA_CLASS_992] = 992, [FA_CLASS_1024] = 1024,
};

// Using this to multiply with reciprocals instead of dividing, which is faster.
constexpr float FA_SIZE_CLASS_RECIPROCALS[FA_NUM_CLASSES] = {
    [FA_CLASS_8] = 1.0F / (float)FA_SIZES[FA_CLASS_8],
    [FA_CLASS_16] = 1.0F / (float)FA_SIZES[FA_CLASS_16],
    [FA_CLASS_24] = 1.0F / (float)FA_SIZES[FA_CLASS_24],
    [FA_CLASS_32] = 1.0F / (float)FA_SIZES[FA_CLASS_32],
    [FA_CLASS_48] = 1.0F / (float)FA_SIZES[FA_CLASS_48],
    [FA_CLASS_64] = 1.0F / (float)FA_SIZES[FA_CLASS_64],
    [FA_CLASS_80] = 1.0F / (float)FA_SIZES[FA_CLASS_80],
    [FA_CLASS_96] = 1.0F / (float)FA_SIZES[FA_CLASS_96],
    [FA_CLASS_112] = 1.0F / (float)FA_SIZES[FA_CLASS_112],
    [FA_CLASS_128] = 1.0F / (float)FA_SIZES[FA_CLASS_128],
    [FA_CLASS_160] = 1.0F / (float)FA_SIZES[FA_CLASS_160],
    [FA_CLASS_192] = 1.0F / (float)FA_SIZES[FA_CLASS_192],
    [FA_CLASS_224] = 1.0F / (float)FA_SIZES[FA_CLASS_224],
    [FA_CLASS_256] = 1.0F / (float)FA_SIZES[FA_CLASS_256],
    [FA_CLASS_288] = 1.0F / (float)FA_SIZES[FA_CLASS_288],
    [FA_CLASS_320] = 1.0F / (float)FA_SIZES[FA_CLASS_320],
    [FA_CLASS_352] = 1.0F / (float)FA_SIZES[FA_CLASS_352],
    [FA_CLASS_384] = 1.0F / (float)FA_SIZES[FA_CLASS_384],
    [FA_CLASS_416] = 1.0F / (float)FA_SIZES[FA_CLASS_416],
    [FA_CLASS_448] = 1.0F / (float)FA_SIZES[FA_CLASS_448],
    [FA_CLASS_480] = 1.0F / (float)FA_SIZES[FA_CLASS_480],
    [FA_CLASS_512] = 1.0F / (float)FA_SIZES[FA_CLASS_512],
    [FA_CLASS_544] = 1.0F / (float)FA_SIZES[FA_CLASS_544],
    [FA_CLASS_576] = 1.0F / (float)FA_SIZES[FA_CLASS_576],
    [FA_CLASS_608] = 1.0F / (float)FA_SIZES[FA_CLASS_608],
    [FA_CLASS_640] = 1.0F / (float)FA_SIZES[FA_CLASS_640],
    [FA_CLASS_672] = 1.0F / (float)FA_SIZES[FA_CLASS_672],
    [FA_CLASS_704] = 1.0F / (float)FA_SIZES[FA_CLASS_704],
    [FA_CLASS_736] = 1.0F / (float)FA_SIZES[FA_CLASS_736],
    [FA_CLASS_768] = 1.0F / (float)FA_SIZES[FA_CLASS_768],
    [FA_CLASS_800] = 1.0F / (float)FA_SIZES[FA_CLASS_800],
    [FA_CLASS_832] = 1.0F / (float)FA_SIZES[FA_CLASS_832],
    [FA_CLASS_864] = 1.0F / (float)FA_SIZES[FA_CLASS_864],
    [FA_CLASS_896] = 1.0F / (float)FA_SIZES[FA_CLASS_896],
    [FA_CLASS_928] = 1.0F / (float)FA_SIZES[FA_CLASS_928],
    [FA_CLASS_960] = 1.0F / (float)FA_SIZES[FA_CLASS_960],
    [FA_CLASS_992] = 1.0F / (float)FA_SIZES[FA_CLASS_992],
    [FA_CLASS_1024] = 1.0F / (float)FA_SIZES[FA_CLASS_1024],
};

struct FaAllocator;

typedef uint16_t CacheOffset;
typedef uint16_t CacheSizeType;

constexpr CacheOffset CACHE_OFFSET_MAX = UINT16_MAX;

STACK_DECLARE(CacheOffset, CacheSizeType, CacheStack)

struct FaBlock {
    uint8_t *data;
    uint32_t total_alloc_count;
    uint32_t max_alloc_count;
    enum FaSizeClass size_class;
    struct Bitmap bmap;
    struct CacheStack cache;
    struct FaBlock *next_block;
    struct FaBlock *prev_block;
    struct FaAllocator *owner;
};

struct FaAllocator {
    struct FaBlock *blocks[FA_NUM_CLASSES];
    struct BlockAllocator block_alloc;
    pthread_mutex_t cross_thread_cache_lock;
    size_t cross_thread_cache_size;
    void *cross_thread_cache[];
};

struct FaAllocator fa_init();
void fa_deinit(struct FaAllocator *alloc);
void *fa_alloc(struct FaAllocator *alloc, size_t size);

enum FaFreeRet {
    OK,
    PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE,
};

enum FaFreeRet fa_free(struct FaAllocator *alloc, void *ptr);
void *fa_realloc(struct FaAllocator *alloc, void *ptr, size_t size);
size_t fa_memsize(void *ptr);

#endif // FAATOR_H
