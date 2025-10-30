#include "fallback_alloc_print_layout.h"

#include "fallback_alloc/fallback_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    puts("Creating the allocator...");
    const size_t heap_init_size = 64;
    struct FallbackAlloc aloc = fallback_allocator_create(heap_init_size);

    print_allocator_memory_layout(&aloc);

    // The size used will probably be 64 because of metadata and alignment
    puts("Allocating 16 bytes, there should be only 1 chunk.");
    const size_t first_block_size = 16;
    void *data1 = fallback_alloc(&aloc, first_block_size);
    assert(data1 != NULL);

    print_allocator_memory_layout(&aloc);

    puts("Allocating 128 bytes, should create new region.");
    const size_t second_block_size = 128;
    void *data2 = fallback_alloc(&aloc, second_block_size);
    assert(data2 != NULL);

    printf("data2: %p\n", data2);

    print_allocator_memory_layout(&aloc);

    puts("Freeing second, then first allocated block, chunks should not merge "
         "as they are not in the same region.");
    fallback_free(&aloc, data2);
    fallback_free(&aloc, data1);
    print_allocator_memory_layout(&aloc);

    puts("Now finally, we will try to allocate more than the first region can "
         "hold, and see if the second region will be used, as it should.");
    const size_t large_data_size = 64;
    void *large_data = fallback_alloc(&aloc, large_data_size);
    assert(large_data != NULL);
    print_allocator_memory_layout(&aloc);

    puts("Freeing the data, expecting 2 empty regions, each having 1 unused "
         "chunk");
    fallback_free(&aloc, large_data);
    print_allocator_memory_layout(&aloc);
}
