#include "fast_alloc_global_wrapper.h"

#include "fast_alloc.h"

#include "../error.h"
#include "../os_allocator.h"

#include <assert.h>
#include <stddef.h>

thread_local struct FaAllocator *allocator = nullptr;

void *falloc(size_t size) {
    if (!allocator) {
        allocator = (struct FaAllocator *)os_alloc(FA_PAGE_SIZE);

        if (!allocator) {
            fa_print_error("os_alloc() faield in falloc()");
            assert(false);
        }

        *allocator = fa_init();
    }

    return fa_alloc(allocator, size);
}

void ffree(void *ptr) {
    fa_free(allocator, ptr);
}

void *frealloc(void *ptr, size_t size) {
    return fa_realloc(allocator, ptr, size);
}

size_t fmemsize(void *ptr) {
    return fa_memsize(ptr);
}

struct FaAllocator *falloc_get_instance() {
    return allocator;
}
