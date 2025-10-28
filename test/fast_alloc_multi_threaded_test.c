#include "../src/falloc.h"
#include "../src/slab_alloc.h"

#include <pthread.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct SlabAlloc alloc;

void *alloc_then_free_thread_unsafe(void * /*arg*/) {
    constexpr int alloc_count = 800;
    constexpr enum SlabSizeClass class = SLAB_CLASS_48;
    SlabSize size = SLAB_SIZES[class];

    void *ptrs[alloc_count];

    for (int i = 0; i < alloc_count; ++i) {
        ptrs[i] = slab_alloc(&alloc, size);
        memset(ptrs[i], 0, size);
    }

    for (int i = 0; i < alloc_count; ++i) {
        slab_free(&alloc, ptrs[i]);
    }

    return nullptr;
}

void *alloc_then_free_thread_safe(void * /*arg*/) {
    constexpr int alloc_count = 800;
    constexpr enum SlabSizeClass class = SLAB_CLASS_48;
    SlabSize size = SLAB_SIZES[class];

    void *ptrs[alloc_count];

    for (int i = 0; i < alloc_count; ++i) {
        ptrs[i] = falloc(size);
        memset(ptrs[i], 0, size);
    }

    for (int i = 0; i < alloc_count; ++i) {
        ffree(ptrs[i]);
    }

    return ptrs[0];
}

void *free_ptr_from_main_thread(void *ptr) {
    ffree(ptr);

    return nullptr;
}

int main() {
    void *(*func_ptr)(void *) = &alloc_then_free_thread_safe;

    puts("Doing a bunch of allocations and frees in multiple threads to check "
         "the behaviour...");

    pthread_t thr1;
    pthread_t thr2;
    pthread_create(&thr1, nullptr, func_ptr, nullptr);
    pthread_create(&thr2, nullptr, func_ptr, nullptr);

    void *allocated_ptr = nullptr;

    pthread_join(thr1, &allocated_ptr);
    pthread_join(thr2, nullptr);

    puts("No segfault happened, which should mean that everything is working "
         "fine.\n");

    puts("Allocating memory in the main thread, and freeing it in another. "
         "After allocating it again, it is expected that the newly allocated "
         "memory should point to the memory that was freed in another thread, "
         "as the freed pointer should be the latest in the cache stack...");

    constexpr size_t sz_to_alloc = 8;
    void *ptr = falloc(sz_to_alloc);
    pthread_create(&thr1, nullptr, &free_ptr_from_main_thread, ptr);
    pthread_join(thr1, &allocated_ptr);
    void *ptr2 = falloc(sz_to_alloc);
    assert(ptr == ptr2);

    puts("Newly allocated pointer is the same as the one freed from another "
         "thread, which should mean that cross thread freeing is working "
         "correctly.");

    ffree(ptr2);
}
