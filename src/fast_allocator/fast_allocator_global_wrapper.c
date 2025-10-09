#include "fast_allocator_global_wrapper.h"

#include "fast_allocator.h"

#include "../error.h"
#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>

thread_local struct FaAllocator *allocator = nullptr;

void *falloc(size_t size) {
    if (allocator == nullptr) {
        allocator = (struct FaAllocator *)os_alloc(FA_PAGE_SIZE);

        if (allocator == nullptr) {
            fa_print_error("os_alloc() faield in falloc()");
            assert(false);
        }

        *allocator = fa_init();
    }

    return fa_alloc(allocator, size);
}

void ffree(void *ptr) {
    fast_alloc_free(allocator, ptr);
}

struct FaAllocator *falloc_get_instance() {
    return allocator;
}
