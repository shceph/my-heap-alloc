#include "fast_allocator_global_wrapper.h"

#include "fast_allocator.h"

#include <assert.h>
#include <stddef.h>

thread_local FastAllocator allocator;
thread_local bool is_allocator_inited = false;

void *falloc(size_t size) {
    if (!is_allocator_inited) {
        allocator = fast_alloc_init();
        is_allocator_inited = true;
    }

    return fast_alloc_alloc(&allocator, size);
}

void ffree(void *ptr) {
    assert(is_allocator_inited);

    fast_alloc_free(&allocator, ptr);
}

FastAllocator *falloc_get_instance() {
    return &allocator;
}
