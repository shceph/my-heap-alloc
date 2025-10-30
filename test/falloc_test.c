#include "slab_alloc_print_layout.h"

#include <falloc.h>
#include <slab_alloc.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define STR_SIZE ((size_t)(288))
static const char STR[STR_SIZE] = "The quick brown fox runs slowly";

#define STR_1_SIZE 32
static const char STR_1[STR_1_SIZE] = "The quick green fox runs slowly";

int main(void) {
    const int allocs = 100;
    const int ptr_to_free_index = 22;
    const enum SlabSizeClass class = SLAB_CLASS_288;

    puts("Initializing fast allocator...");

    printf("Allocating %zu bytes %d times and copying string of len = %zu\n",
           STR_SIZE, allocs, STR_SIZE);

    void *ptrs[allocs];
    void *ptr_to_free = NULL;

    for (int i = 0; i < allocs; ++i) {
        void *ptr = falloc(STR_SIZE);
        memcpy(ptr, STR, STR_SIZE);

        if (i == ptr_to_free_index) {
            ptr_to_free = ptr;
        }

        printf("ptr: %p\n", ptr);
        ptrs[i] = ptr;
    }

    printf("\nFreeing %d. pointer...\n", ptr_to_free_index);

    ffree(ptr_to_free);
    slab_alloc_print_layout(&falloc_get_instance()->slab_alloc);

    puts("Allocating again, should return pointer equal to the freed one...");

    void *ptr = falloc(STR_SIZE);

    assert(ptr == ptr_to_free);

    puts("Assertion passed, the allocator used the memory location of the "
         "previously freed pointer.");

    memcpy(ptr, STR_1, STR_1_SIZE);

    printf("\nData at ptr: %s\n", (char *)ptr);

    print_bitmap(falloc_get_instance()->slab_alloc.slabs[class]);

    puts("Freeing all...");

    for (int i = 0; i < allocs; ++i) {
        ffree(ptrs[i]);
    }

    puts("Freed all, expecting the allocator to be empty.");

    slab_alloc_print_layout(&falloc_get_instance()->slab_alloc);

    print_bitmap(falloc_get_instance()->slab_alloc.slabs[class]);
}
