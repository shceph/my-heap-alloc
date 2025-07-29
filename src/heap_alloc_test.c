#include <stdio.h>

#include "heap_alloc.h"

#define ALLOCATOR_SIZE 4096
#define ARR0_SIZE 8
#define ARR1_SIZE 16

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
    fprintf(stderr, "Creating the allocator...\n");
    heap_allocator_t allocator = heap_allocator_create(ALLOCATOR_SIZE);
    fprintf(stderr, "[OK] Created the allocator\n");

    fprintf(stderr, "[TEST] Allocating for array 0...\n");

    int *arr0 = (int *)heap_alloc(&allocator, ARR0_SIZE * sizeof(int));

    if (arr0 == nullptr) {
        fprintf(stderr, "[FAIL] Failed to allocate memory for array 0\n");
        return 1;
    }

    fprintf(stderr, "[OK] arr0 != nullptr, should have allocated correctly\n");

    for (int i = 0; i < ARR0_SIZE; i++) {
        arr0[i] = i;
    }

    fprintf(stderr, "[TEST] Allocationg for array 1...\n");

    int *arr1 = (int *)heap_alloc(&allocator, ARR1_SIZE * sizeof(int));

    if (arr1 == nullptr) {
        fprintf(stderr, "[FAIL] Failed to allocate memory for array 1\n");
        return 1;
    }

    fprintf(stderr, "[OK] arr1 != nullptr, should have allocated correctly\n");

    for (int i = 0; i < ARR1_SIZE; i++) {
        arr1[i] = -i;
    }

    for (int i = 0; i < ARR0_SIZE; i++) {
        printf("%d\n", arr0[i]);
    }
    printf("Array 0 printed\n\n");

    for (int i = 0; i < ARR1_SIZE; i++) {
        printf("%d\n", arr1[i]);
    }
    printf("Array 1 printed\n");

    print_allocator_memory_layout(&allocator);

    return 0;
}
