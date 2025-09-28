#include "fast_allocator_global_wrapper.h"

#include "fast_allocator.h"

#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>

thread_local FastAllocator *allocator = nullptr;

void *falloc(size_t size) {
    if (allocator == nullptr) {
        allocator = (FastAllocator *)os_alloc(FAST_ALLOC_PAGE_SIZE);
        *allocator = fast_alloc_init();
    }

    return fast_alloc_alloc(allocator, size);
}

void ffree(void *ptr) {
    fast_alloc_free(allocator, ptr);
}

FastAllocator *falloc_get_instance() {
    return allocator;
}
