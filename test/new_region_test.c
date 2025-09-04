#include "test.h"

#include "../src/heap_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    puts("Creating the allocator...");
    const size_t heap_init_size = 64;
    heap_allocator_t aloc = heap_allocator_create(heap_init_size);

    print_allocator_memory_layout(&aloc);

    // The size used will probably be 64 because of metadata and alignment
    puts("Allocating 16 bytes, there should be only 1 chunk.");
    const size_t first_block_size = 16;
    void *data1 = heap_alloc(&aloc, first_block_size);
    assert(data1 != nullptr);

    print_allocator_memory_layout(&aloc);

    puts("Allocating 128 bytes, should create new region.");
    const size_t second_block_size = 128;
    void *data2 = heap_alloc(&aloc, second_block_size);
    assert(data2 != nullptr);

    printf("data2: %p\n", data2);

    print_allocator_memory_layout(&aloc);

    puts("Freeing second, then first allocated block, chunks should not merge "
         "as they are not in the same region.");
    heap_free(&aloc, data2);
    heap_free(&aloc, data1);
    print_allocator_memory_layout(&aloc);
}
