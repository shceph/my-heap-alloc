#include "../src/fast_allocator/fast_allocator.h"
#include "../src/fast_allocator/fast_allocator_global_wrapper.h"

#include <bits/pthreadtypes.h>
#include <pthread.h>

#include <stdio.h>
#include <string.h>

FastAllocator alloc;

void *alloc_then_free_thread_unsafe(void * /*arg*/) {
    constexpr int alloc_count = 800;
    constexpr FastAllocSizeClass class = FAST_ALLOC_CLASS_48;
    constexpr FastAllocSize size = FAST_ALLOC_SIZES[class];

    void *ptrs[alloc_count];

    for (int i = 0; i < alloc_count; ++i) {
        ptrs[i] = fast_alloc_alloc(&alloc, size);
        memset(ptrs[i], 0, size);
    }

    for (int i = 0; i < alloc_count; ++i) {
        fast_alloc_free(&alloc, ptrs[i]);
    }

    return nullptr;
}

void *alloc_then_free_thread_safe(void * /*arg*/) {
    constexpr int alloc_count = 800;
    constexpr FastAllocSizeClass class = FAST_ALLOC_CLASS_48;
    constexpr FastAllocSize size = FAST_ALLOC_SIZES[class];

    void *ptrs[alloc_count];

    for (int i = 0; i < alloc_count; ++i) {
        ptrs[i] = falloc(size);
        memset(ptrs[i], 0, size);
    }

    for (int i = 0; i < alloc_count; ++i) {
        ffree(ptrs[i]);
    }

    return nullptr;
}

int main(int argc, const char *argv[]) {
    bool run_unsafe_variant = false;
    const char *thread_unsafe_flag = "--thread-unsafe";

    if (argc > 1 &&
        strncmp(thread_unsafe_flag, argv[1], strlen(thread_unsafe_flag)) == 0) {
        run_unsafe_variant = true;
    }

    void *(*func_ptr)(void *) = &alloc_then_free_thread_safe;

    if (run_unsafe_variant) {
        puts("Runs unsafe variant.");
        func_ptr = alloc_then_free_thread_unsafe;
        alloc = fast_alloc_init();
    }

    pthread_t thr1;
    pthread_t thr2;
    pthread_create(&thr1, nullptr, func_ptr, nullptr);
    pthread_create(&thr2, nullptr, func_ptr, nullptr);

    pthread_join(thr1, nullptr);
    pthread_join(thr2, nullptr);
}
