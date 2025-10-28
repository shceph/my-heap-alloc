#include "rtree_print.h"

#include "../src/falloc.h"

static constexpr size_t STRING_SIZE = 2048;
static constexpr size_t BIG_STRING_SIZE = 99999999;

struct String {
    char buff[STRING_SIZE];
};

struct BigString {
    char buff[BIG_STRING_SIZE];
};

int main() {
    constexpr size_t big_string_count = 6;
    constexpr size_t small_string_count = 10;

    struct BigString *big_strings[big_string_count];
    struct BigString *small_strings[small_string_count];

    puts("Allocating a couple of really big strings...");

    for (size_t i = 0; i < big_string_count; ++i) {
        big_strings[i] = falloc(sizeof(struct BigString));
    }

    print_tree(&falloc_get_instance()->rtree);

    puts("Passed.\n\nAllocating a couple of smaller strings...");

    for (size_t i = 0; i < small_string_count; ++i) {
        small_strings[i] = falloc(sizeof(struct BigString));
    }

    print_tree(&falloc_get_instance()->rtree);

    puts("Passed.\n\nFreeing the big strings...");

    for (size_t i = 0; i < big_string_count; ++i) {
        ffree(big_strings[i]);
    }

    print_tree(&falloc_get_instance()->rtree);

    puts("Passed.\n\nFreeing the small strings...");

    for (size_t i = 0; i < small_string_count; ++i) {
        ffree(small_strings[i]);
    }

    print_tree(&falloc_get_instance()->rtree);

    puts("Passed.");
}
