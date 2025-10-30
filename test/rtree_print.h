#include "rtree.h"

#include <stdio.h>

static inline void print_node(struct RtreeNode *node, int depth) {
    if (depth >= (int)sizeof(void *)) {
        return;
    }

    for (int i = 0; i <= UINT8_MAX; ++i) {
        if (node->entries[i].next == NULL) {
            continue;
        }

        for (int j = 0; j < depth; ++j) {
            printf("  ");
        }

        printf("%02x\n", i);

        print_node(node->entries[i].next, depth + 1);
    }
}

static inline void print_tree(struct Rtree *rtree) {
    puts("\nPrinting the tree...\n");
    print_node(rtree->head, 0);
    puts("\nDone printing the tree.\n");
}
