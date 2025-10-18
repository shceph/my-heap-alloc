#include "../src/fast_allocator/rtree.h"

#include <stdio.h>

int main() {
    constexpr int ptr_count = 10;
    int arr_to_push[ptr_count];
    int arr_to_check[ptr_count];

    struct Rtree rtree = rtree_init();

    puts("Pushing pointers in the tree...");

    for (int i = 0; i < ptr_count; ++i) {
        rtree_push_ptr(&rtree, &arr_to_push[i], sizeof(int));
    }

    puts("Passed.\n");
    puts("Expecting rtree_contains to return true with pushed pointers...");

    for (int i = 0; i < ptr_count; ++i) {
        size_t size = 0;

        bool contains =
            rtree_retrieve_size_if_contains(&rtree, &arr_to_push[i], &size);
        assert(contains);

        printf("retrieved size: %zu\n", size);
    }

    puts("Passed.\n");
    puts("Expecting rtree_contains to return false with pointers that are not "
         "pushed...");

    for (int i = 0; i < ptr_count; ++i) {
        bool contains = rtree_contains(&rtree, &arr_to_check[i]);
        assert(!contains);
    }

    puts("Passed.\n");
    puts("Removing all pushed pointers from the tree, expecting rtree_contains "
         "to return false for those pointers...");

    for (int i = 0; i < ptr_count; ++i) {
        rtree_remove_ptr(&rtree, &arr_to_push[i]);
        bool contains = rtree_contains(&rtree, &arr_to_push[i]);
        assert(!contains);
    }

    puts("Passed.");
}
