#include <falloc.h>
#include <os_allocator.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ALLOC_SIZE 1024
#define NUM_OF_ALLOCS  (1 * 1024)
#define NUM_OF_RERUNS  1

typedef void *(*AllocFunc)(size_t);
typedef void (*FreeFunc)(void *);

typedef size_t AllocType;

double run_test(AllocFunc alloc, FreeFunc free) {
    (void)free;

    const size_t sizeof_arr = (size_t)NUM_OF_ALLOCS * sizeof(void *);
    void **ptrs = (void **)os_alloc(sizeof_arr);

    if (!ptrs) {
        perror("os_alloc failed.");
        return 0;
    }

    memset((void *)ptrs, 0, sizeof_arr);

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_OF_RERUNS; ++i) {
        for (int j = 0; j < NUM_OF_ALLOCS; ++j) {
            free(ptrs[j]);

            size_t size = rand() % MAX_ALLOC_SIZE;

            ptrs[j] = alloc(size);

            if (j > NUM_OF_ALLOCS / 2) {
                int index = (rand() % j) + 1;
                free(ptrs[index]);
                ptrs[index] = NULL;
            }
        }

        //         for (int j = 0; j < NUM_OF_ALLOCS; ++j) {
        //             free(ptrs[j]);
        //             ptrs[j] = NULL;
        //         }
    }

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = (double)(end.tv_sec - start.tv_sec) +
                  ((double)(end.tv_nsec - start.tv_nsec) / 1e9);

    os_free((void *)ptrs, sizeof_arr);

    return time;
}

void no_free(void *ptr) {
    (void)ptr;
}

int main(void) {
    time_t seed = time(NULL);

    finit();
    void *dummy = malloc(1024 * 1024);
    (void)dummy;

    puts("Testing falloc...");
    srand(seed);
    double falloc_time = run_test(&falloc, &ffree);
    puts("Testing malloc...");
    srand(seed);
    double malloc_time = run_test(&malloc, &free);

    printf("falloc time: %lfs\n", falloc_time);
    printf("malloc time: %lfs\n\n", malloc_time);

    printf("falloc is %lf%% as fast as malloc\n",
           malloc_time / falloc_time * 100.0);
    printf("malloc is %lf%% as fast as falloc\n",
           falloc_time / malloc_time * 100.0);
}
