#include "fast_alloc_print_layout.h"

#include "../src/fast_allocator/fast_allocator.h"
#include "../src/fast_allocator/fast_allocator_global_wrapper.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static constexpr size_t STR_SIZE = 288;
static char str[STR_SIZE] = "The quick brown fox runs slowly";

static constexpr size_t STR_1_SIZE = 32;
static char str_1[STR_1_SIZE] = "The quick green fox runs slowly";

int main() {
    constexpr int allocs = 100;
    constexpr int ptr_to_free_index = 22;
    constexpr enum FaSizeClass class = FA_CLASS_288;

    puts("Initializing fast allocator...");

    printf("Allocating %zu bytes %d times and copying string of len = %zu\n",
           STR_SIZE, allocs, STR_SIZE);

    void *ptrs[allocs];
    void *ptr_to_free = nullptr;

    for (int i = 0; i < allocs; ++i) {
        void *ptr = falloc(STR_SIZE);
        memcpy(ptr, str, STR_SIZE);

        if (i == ptr_to_free_index) {
            ptr_to_free = ptr;
        }

        printf("ptr: %p\n", ptr);
        ptrs[i] = ptr;
    }

    printf("\nFreeing %d. pointer...\n", ptr_to_free_index);

    ffree(ptr_to_free);
    fast_alloc_print_layout(falloc_get_instance());

    puts("Allocating again, should return pointer equal to the freed one...");

    void *ptr = falloc(STR_SIZE);

    assert(ptr == ptr_to_free);

    puts("Assertion passed, the allocator used the memory location of the "
         "previously freed pointer.");

    memcpy(ptr, str_1, STR_1_SIZE);

    printf("\nData at ptr: %s\n", (char *)ptr);

    print_bitmap(&falloc_get_instance()->blocks[class]->bmap);

    puts("Freeing all...");

    for (int i = 0; i < allocs; ++i) {
        ffree(ptrs[i]);
    }

    fast_alloc_print_layout(falloc_get_instance());

    print_bitmap(&falloc_get_instance()->blocks[class]->bmap);
}
