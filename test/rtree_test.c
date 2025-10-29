#include "rtree_print.h"

#include "rtree.h"

#include <inttypes.h>
#include <stdio.h>

typedef int ArrayType;

int main() {
    constexpr int ptr_count = 30;
    ArrayType arr_to_push[ptr_count];
    ArrayType arr_to_check[ptr_count];

    printf("Array to push addr:  %p\n", (void *)arr_to_push);
    printf("Array to check addr: %p\n\n", (void *)arr_to_check);

    struct Rtree rtree = rtree_init();

    puts("Pushing pointers in the tree...");

    for (int i = 0; i < ptr_count; ++i) {
        rtree_push_ptr(&rtree, &arr_to_push[i], sizeof(ArrayType));
    }

    print_tree(&rtree);

    puts("Passed.\n");
    puts("Expecting rtree_contains to return true with pushed pointers...\n");

    for (int i = 0; i < ptr_count; ++i) {
        size_t size = 0;

        bool contains =
            rtree_retrieve_size_if_contains(&rtree, &arr_to_push[i], &size);
        (void)contains;
        assert(contains);

        assert(size == sizeof(ArrayType));

        printf("Retrieved size: %zu\n", size);
    }

    puts("\nPassed.\n");
    puts("Expecting rtree_contains to return false with pointers that are not "
         "pushed...");

    for (int i = 0; i < ptr_count; ++i) {
        bool contains = rtree_contains(&rtree, &arr_to_check[i]);
        (void)contains;
        assert(!contains);
    }

    puts("Passed.\n");
    puts("Removing all pushed pointers from the tree, expecting rtree_contains "
         "to return false for those pointers...\n");

    for (int i = 0; i < ptr_count; ++i) {
        size_t size = 0;
        rtree_remove_ptr(&rtree, &arr_to_push[i], &size);

        printf("Retrieved size: %zu\n", size);

        bool contains = rtree_contains(&rtree, &arr_to_push[i]);
        (void)contains;
        assert(!contains);
    }

    puts("\nPassed.");

    print_tree(&rtree);
}
