#ifndef CACHE_H
#define CACHE_H

#include <assert.h>
#include <stdint.h>

typedef uint16_t CacheOffset;
typedef uint16_t CacheSize;

constexpr CacheOffset CACHE_OFFSET_MAX = UINT16_MAX;

struct Cache {
    CacheOffset *begin;
    CacheSize size;
    CacheSize capacity;
};

inline static struct Cache cache_init(void *mem, CacheSize capacity) {
    return (struct Cache){
        .begin = mem,
        .size = 0,
        .capacity = capacity,
    };
}

enum CachePushRet {
    CACHE_PUSH_OK,
    CACHE_PUSH_IS_FULL,
};

inline static enum CachePushRet cache_push(struct Cache *cache,
                                           CacheOffset val) {
    if (cache->size == cache->capacity) {
        return CACHE_PUSH_IS_FULL;
    }

    cache->begin[cache->size] = val;
    ++cache->size;

    return CACHE_PUSH_OK;
}

inline static CacheOffset cache_pop(struct Cache *cache) {
    assert(cache->size != 0);

    --cache->size;
    return cache->begin[cache->size];
}

#endif // CACHE_H
