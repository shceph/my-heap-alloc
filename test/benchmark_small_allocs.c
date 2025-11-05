#include <falloc.h>
#include <os_alloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ALLOC_SIZE 1024
#define NUM_OF_ALLOCS  (128 * 1024)
#define NUM_OF_RERUNS  100

#define NANOSECONDS_PER_SECOND 1e9

typedef void *(*AllocFunc)(size_t);
typedef void (*FreeFunc)(void *);

typedef size_t AllocType;

double run_test(AllocFunc alloc_func, FreeFunc free_func) {
    (void)free_func;

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
            free_func(ptrs[j]);

            size_t size = rand() % MAX_ALLOC_SIZE;

            ptrs[j] = alloc_func(size);

            if (j > NUM_OF_ALLOCS / 2) {
                int index = rand() % j;
                free_func(ptrs[index]);
                ptrs[index] = NULL;
            }
        }
    }

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time =
        (double)(end.tv_sec - start.tv_sec) +
        ((double)(end.tv_nsec - start.tv_nsec) / NANOSECONDS_PER_SECOND);

    os_free((void *)ptrs, sizeof_arr);

    return time;
}

#define WARMUP_NUM_OF_ALLOCS 100

static void *warmup_ptrs_1[WARMUP_NUM_OF_ALLOCS];
static void *warmup_ptrs_2[WARMUP_NUM_OF_ALLOCS];

void warmup(AllocFunc alloc_func, void *ptrs[]) {
    srand(time(NULL));

    const size_t max_size = 1024;

    for (int i = 0; i < WARMUP_NUM_OF_ALLOCS; ++i) {
        size_t size = (rand() % max_size) + 1;
        ptrs[i] = alloc_func(size);
    }
}

int main(void) {
    time_t seed = time(NULL);

    warmup(&malloc, warmup_ptrs_1);
    warmup(&falloc, warmup_ptrs_2);

    puts("Testing falloc...");
    srand(seed);
    double falloc_time = run_test(&falloc, &ffree);
    puts("Testing malloc...");
    srand(seed);
    double malloc_time = run_test(&malloc, &free);

    printf("falloc time: %lfs\n", falloc_time);
    printf("malloc time: %lfs\n\n", malloc_time);

    printf("falloc is %lf%% faster than malloc\n",
           malloc_time / falloc_time * 100.0);
    printf("malloc is %lf%% faster than falloc\n",
           falloc_time / malloc_time * 100.0);
}
