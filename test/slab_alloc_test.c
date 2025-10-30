#include "slab_alloc_print_layout.h"

#include "slab_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define STR_SIZE ((size_t)(32))
static char str[STR_SIZE] = "The quick brown fox runs slowly";

#define STR_1_SIZE ((size_t)(32))
static char str_1[STR_1_SIZE] = "The quick green fox runs slowly";

int main(void) {
    const int allocs = 1000;
    const int ptr_to_free_index = 900;

    puts("Initializing fast allocator...");

    struct SlabAlloc alloc = slab_alloc_init(NULL);

    printf("Allocating %zu bytes %d times and copying string of len = %zu\n",
           STR_SIZE, allocs, STR_SIZE);

    void *ptr_to_free = NULL;

    for (int i = 0; i < allocs; ++i) {
        void *ptr = slab_alloc(&alloc, STR_SIZE);
        memcpy(ptr, str, STR_SIZE);

        if (i == ptr_to_free_index) {
            ptr_to_free = ptr;
        }

        printf("ptr: %p\n", ptr);
    }

    printf("\nFreeing %d. pointer...\n", ptr_to_free_index);

    slab_free(&alloc, ptr_to_free);
    slab_alloc_print_layout(&alloc);

    puts("Allocating again, should return pointer equal to the freed one...");

    void *ptr = slab_alloc(&alloc, STR_SIZE);

    assert(ptr == ptr_to_free);

    puts("Assertion passed, the allocator used the memory location of the "
         "previously freed pointer.");

    memcpy(ptr, str_1, STR_1_SIZE);

    printf("\nData at ptr: %s\n", (char *)ptr);

    slab_alloc_print_layout(&alloc);

    slab_alloc_deinit(&alloc);
}
