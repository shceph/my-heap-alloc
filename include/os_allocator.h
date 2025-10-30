#ifndef OS_ALLOCATOR_H
#define OS_ALLOCATOR_H

#include <sys/mman.h>

#include <stddef.h>

#define OS_ALLOC_PAGE_SIZE 0x1000

// Sets errno and returns null on error.
static inline void *os_alloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    return ptr;
}

#define OS_FREE_OK   0
#define OS_FREE_FAIL 1

// Returns error code from munmap, sets errno on error.
static inline int os_free(void *ptr, size_t size) {
    int ret = munmap(ptr, size);

    if (ret == -1) {
        return OS_FREE_FAIL;
    }

    return OS_FREE_OK;
}

#endif // OS_ALLOCATOR_H
