#include <stdio.h>
#include <string.h>

#include "heap_alloc.h"

#define ALLOCATOR_SIZE 4096
#define ARR0_SIZE      8
#define ARR1_SIZE      16
#define TEST_STRING    "a really long string"
#define TEST_STR_SIZE  sizeof(TEST_STRING)

void print_allocator_memory_layout(heap_allocator_t *allocator) {
    printf("\nallocator data addr: %p\n\n", allocator->begin);
    chunk_llist_t *chunk = allocator->chunk_llist_head;

    while (chunk != nullptr) {
        printf("addr: %p\n", chunk);
        printf("used: %s\n", chunk->used ? "true" : "false");
        printf("size: %zu\n\n", chunk->size);
        chunk = chunk->next;
    }
}

int main() {
    printf("[TEST] Creating the allocator...\n");
    heap_allocator_t allocator = heap_allocator_create(ALLOCATOR_SIZE);
    printf("[OK] Created the allocator\n");

    printf("[TEST] Allocating for array 0...\n");

    int *arr0 = (int *)heap_alloc(&allocator, ARR0_SIZE * sizeof(int));

    if (arr0 == nullptr) {
        printf("[FAIL] Failed to allocate memory for array 0\n");
        return 1;
    }

    printf("[OK] arr0 != nullptr, should have allocated correctly\n");

    for (int i = 0; i < ARR0_SIZE; i++) {
        arr0[i] = i;
    }

    printf("[TEST] Allocationg for array 1...\n");

    int *arr1 = (int *)heap_alloc(&allocator, ARR1_SIZE * sizeof(int));

    if (arr1 == nullptr) {
        printf("[FAIL] Failed to allocate memory for array 1\n");
        return 1;
    }

    for (int i = 0; i < ARR1_SIZE; i++) {
        arr1[i] = -i;
    }

    printf("[TEST] Allocationg for test string...\n");

    char *str = (char *)heap_alloc(&allocator, TEST_STR_SIZE);

    if (str == nullptr) {
        printf("[FAIL] Failed to allocate memory for str\n");
        return 1;
    }

    printf("[OK] str != nullptr, should have allocated correctly\n");

    strncpy(str, TEST_STRING, TEST_STR_SIZE);

    for (int i = 0; i < ARR0_SIZE; i++) {
        printf("%d\n", arr0[i]);
    }
    printf("Array 0 printed\n\n");

    for (int i = 0; i < ARR1_SIZE; i++) {
        printf("%d\n", arr1[i]);
    }
    printf("Array 1 printed\n\n");

    printf("Printing allocated string: %s\n", str);

    print_allocator_memory_layout(&allocator);

    printf("[TEST] Freeing arr0...\n");
    heap_free(&allocator, arr0);
    printf("[?] Freed, no status. There should be 4 chunks\n");

    print_allocator_memory_layout(&allocator);

    printf("[TEST] Freeing arr1...\n");
    heap_free(&allocator, arr1);
    printf("[?] Freed, no status. There should be 3 chunks\n");

    print_allocator_memory_layout(&allocator);

    printf("[TEST] Freeing str...\n");
    heap_free(&allocator, str);
    printf("[?] Freed, no status. There should be 1 chunk\n");

    print_allocator_memory_layout(&allocator);

    printf("[TEST] "
           "Verifying allocator uses entire chunk when too small to split\n");

    void *ptr0 = heap_alloc(&allocator, 8);
    void *ptr1 = heap_alloc(&allocator, 8);
    void *ptr2 = heap_alloc(&allocator, 8);
    heap_free(&allocator, ptr1);
    ptr1 = heap_alloc(&allocator, 4);

    print_allocator_memory_layout(&allocator);

    return 0;
}
