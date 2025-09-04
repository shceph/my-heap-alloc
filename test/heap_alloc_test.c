#include "test.h"

#include "../src/heap_alloc.h"

#include <stdio.h>
#include <string.h>

#define ALLOCATOR_SIZE 4096UL
#define ARR0_SIZE      8UL
#define ARR1_SIZE      16UL
#define TEST_STRING    "a really long string"
#define TEST_STR_SIZE  sizeof(TEST_STRING)

void realloc_test(heap_allocator_t *aloc) {
    puts("[TEST] Testing heap_realloc");

    int *arr = (int *)heap_alloc(aloc, ARR0_SIZE * sizeof(int));

    if (arr == nullptr) {
        puts("arr is null, aborting...");
        return;
    }

    puts("arr != null, continuing...");

    for (size_t i = 0; i < ARR0_SIZE; ++i) {
        arr[i] = (int)i;
        printf("%d\n", arr[i]);
    }

    puts("Reallocating the array...");

    arr = (int *)heap_realloc(aloc, arr, ARR0_SIZE * 2 * sizeof(int));

    for (size_t i = ARR0_SIZE; i < ARR0_SIZE * 2; ++i) {
        arr[i] = (int)i;
    }

    puts("The array now:");

    for (size_t i = 0; i < ARR0_SIZE * 2; ++i) {
        printf("%d\n", arr[i]);
    }
}

int main() {
    printf("[TEST] Creating the allocator...\n");
    heap_allocator_t aloc = heap_allocator_create(ALLOCATOR_SIZE);
    printf("[OK] Created the allocator\n");

    printf("[TEST] Allocating for array 0...\n");

    int *arr0 = (int *)heap_alloc(&aloc, ARR0_SIZE * sizeof(int));

    if (arr0 == nullptr) {
        printf("[FAIL] Failed to allocate memory for array 0\n");
        return 1;
    }

    printf("[OK] arr0 != nullptr, should have allocated correctly\n");

    for (size_t i = 0; i < ARR0_SIZE; i++) {
        arr0[i] = (int)i;
    }

    printf("[TEST] Allocationg for array 1...\n");

    int *arr1 = (int *)heap_alloc(&aloc, ARR1_SIZE * sizeof(int));

    if (arr1 == nullptr) {
        printf("[FAIL] Failed to allocate memory for array 1\n");
        return 1;
    }

    printf("[OK] arr1 != nullptr, should have allocated correctly\n");

    for (size_t i = 0; i < ARR1_SIZE; i++) {
        arr1[i] = -(int)i;
    }

    printf("[TEST] Allocationg for test string...\n");

    char *str = (char *)heap_alloc(&aloc, TEST_STR_SIZE);

    if (str == nullptr) {
        printf("[FAIL] Failed to allocate memory for str\n");
        return 1;
    }

    printf("[OK] str != nullptr, should have allocated correctly\n");

    strncpy(str, TEST_STRING, TEST_STR_SIZE);

    for (size_t i = 0; i < ARR0_SIZE; i++) {
        printf("%d\n", arr0[i]);
    }
    printf("Array 0 printed\n\n");

    for (size_t i = 0; i < ARR1_SIZE; i++) {
        printf("%d\n", arr1[i]);
    }
    printf("Array 1 printed\n\n");

    printf("Printing allocated string: %s\n", str);

    print_allocator_memory_layout(&aloc);

    printf("[TEST] Freeing arr0...\n");
    heap_free(&aloc, arr0);
    printf("[?] Freed, no status. There should be 4 chunks\n");

    print_allocator_memory_layout(&aloc);

    printf("[TEST] Freeing arr1...\n");
    heap_free(&aloc, arr1);
    printf("[?] Freed, no status. There should be 3 chunks\n");

    print_allocator_memory_layout(&aloc);

    printf("[TEST] Freeing str...\n");
    heap_free(&aloc, str);
    printf("[?] Freed, no status. There should be 1 chunk\n");

    print_allocator_memory_layout(&aloc);

    printf("[TEST] "
           "Verifying allocator uses entire chunk when too small to split\n");

    const size_t small_chunk_sz = 8;
    void *ptr0 = heap_alloc(&aloc, small_chunk_sz);
    void *ptr1 = heap_alloc(&aloc, small_chunk_sz);
    void *ptr2 = heap_alloc(&aloc, small_chunk_sz);
    heap_free(&aloc, ptr1);
    ptr1 = heap_alloc(&aloc, 4);

    print_allocator_memory_layout(&aloc);

    realloc_test(&aloc);
    print_allocator_memory_layout(&aloc);

    heap_free(&aloc, ptr0);
    heap_free(&aloc, ptr1);
    heap_free(&aloc, ptr2);

    return 0;
}
