#include "fixed_alloc.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define GET_NUM_OF_ELEMS(FIXED_ALLOC_VAR) ((FIXED_ALLOC_VAR).cache.capacity)

static inline bool is_full(struct FixedAllocBlock *block) {
    size_t fixed_alloc_buff_size =
        (GET_NUM_OF_ELEMS(*block)) * block->unit_size;

    return block->offset >= fixed_alloc_buff_size - block->unit_size &&
           block->cache.size == 0;
}

int main() {
    constexpr size_t unit_size = 0x1000;
    constexpr int allocations_count = 0x2000;
    void *ptrs[allocations_count];

    printf("Size to alloc: %zu KB\n", allocations_count * unit_size / 1024);

    struct FixedAllocator alloc = fixed_alloc_init(unit_size);

    puts("Doing a bunch of allocations and writing to the memory...");

    for (int i = 0; i < allocations_count; ++i) {
        ptrs[i] = fixed_alloc(&alloc);

        if (!ptrs[i]) {
            printf("Failed at i = %d, aborting...\n", i);
            return 1;
        }

        memset(ptrs[i], 0, unit_size);
    }

    puts("Passed.\n\nFreeing the pointers, expecting all the blocks not to be "
         "full...");

    for (int i = 0; i < allocations_count; ++i) {
        fixed_free(&alloc, ptrs[i]);
    }

    for (uint32_t i = 0; i < alloc.block_count; ++i) {
        if (is_full(&alloc.blocks[i])) {
            puts("A block is full. Test failed, aborting...");
            return 1;
        }
    }

    puts("Passed.");
}
