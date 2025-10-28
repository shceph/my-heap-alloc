#include "falloc.h"

#include "rtree.h"
#include "slab_alloc.h"

#include "fallback_allocator/fallback_allocator.h"

#include "error.h"
#include "os_allocator.h"

#include <assert.h>
#include <stddef.h>

#define FALLBACK_ALLOC_DEFAULT_SIZE ((size_t)(10 * 1024 * 1024))

thread_local struct Falloc *allocator = NULL;

static inline void cache_init(struct Falloc *alloc) {
    pthread_mutex_init(&alloc->lock, NULL);
    alloc->size = 0;
}

static inline void *alloc_big(struct Falloc *alloc, size_t size) {
    // void *ptr = os_alloc(size);
    void *ptr = fallback_alloc(&alloc->fallback_alloc, size);

    if (!ptr) {
        return nullptr;
    }

    rtree_push_ptr(&alloc->rtree, ptr, size);

    return ptr;
}

static inline void free_big(struct Falloc *alloc, void *ptr) {
    size_t allocated_size = 0;
    rtree_remove_ptr(&alloc->rtree, ptr, &allocated_size);
    // os_free(ptr, allocated_size);
    fallback_free(&alloc->fallback_alloc, ptr);
}

static inline void cross_thread_cache_push(struct Falloc *diff_thread_alloc,
                                           void *ptr) {
    int err_code = pthread_mutex_lock(&diff_thread_alloc->lock);
    assert(err_code == 0);

    diff_thread_alloc->stack[diff_thread_alloc->size] = ptr;
    ++diff_thread_alloc->size;

    err_code = pthread_mutex_unlock(&diff_thread_alloc->lock);
    assert(err_code == 0);
}

static inline void *cross_thread_cache_pop(struct Falloc *alloc) {
    void *ret;

    int err_code = pthread_mutex_lock(&alloc->lock);
    assert(err_code == 0);

    --alloc->size;
    assert(alloc->size >= 0);

    ret = alloc->stack[alloc->size];

    err_code = pthread_mutex_unlock(&alloc->lock);
    assert(err_code == 0);

    return ret;
}

static inline size_t get_cross_thread_cache_size(struct Falloc *alloc) {
    int err_code = pthread_mutex_lock(&alloc->lock);
    assert(err_code == 0);

    size_t size = alloc->size;

    err_code = pthread_mutex_unlock(&alloc->lock);
    assert(err_code == 0);

    return size;
}

// TODO: Check if the cross thread cache is full.
static inline void cross_thread_free(void *ptr) {
    struct Slab *metadata = slab_from_ptr(ptr);
    cross_thread_cache_push(metadata->owner->owner, ptr);
}

static inline void clear_cross_thread_cache(struct Falloc *alloc) {
    while (get_cross_thread_cache_size(alloc) != 0) {
        enum FaFreeRet ret =
            slab_free(&alloc->slab_alloc, cross_thread_cache_pop(alloc));
        assert(ret != PTR_NOT_OWNED_BY_PASSED_ALLOCATOR_INSTANCE);
    }
}

void finit() {
    assert(!allocator);

    allocator = (struct Falloc *)os_alloc((size_t)2 * FA_PAGE_SIZE);

    if (!allocator) {
        fa_print_error("os_alloc() faield in falloc()");
        assert(false);
    }

    *allocator = (struct Falloc){
        .slab_alloc = slab_alloc_init(allocator),
        .fallback_alloc =
            fallback_allocator_create(FALLBACK_ALLOC_DEFAULT_SIZE),
        .rtree = rtree_init(),
    };

    cache_init(allocator);
}

void *falloc(size_t size) {
    if (!allocator) {
        finit();
    }

    clear_cross_thread_cache(allocator);

    if (size > SLAB_CLASS_MAX) {
        return alloc_big(allocator, size);
    }

    return slab_alloc(&allocator->slab_alloc, size);
}

void ffree(void *ptr) {
    if (allocator && rtree_contains(&allocator->rtree, ptr)) {
        free_big(allocator, ptr);
        return;
    }

    if (!allocator ||
        !slab_alloc_is_ptr_in_this_instance(&allocator->slab_alloc, ptr)) {
        cross_thread_free(ptr);
        return;
    }

    clear_cross_thread_cache(allocator);

    slab_free(&allocator->slab_alloc, ptr);
}

void *frealloc(void *ptr, size_t size) {
    return slab_realloc(&allocator->slab_alloc, ptr, size);
}

size_t fmemsize(void *ptr) {
    size_t size = 0;

    if (rtree_retrieve_size_if_contains(&allocator->rtree, ptr, &size)) {
        return size;
    }

    return slab_memsize(ptr);
}

struct Falloc *falloc_get_instance() {
    return allocator;
}
