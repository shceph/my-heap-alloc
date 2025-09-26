#ifndef OS_ALLOCATOR_H
#define OS_ALLOCATOR_H

#include <sys/mman.h>

#include <stddef.h>

static constexpr size_t OS_ALLOC_PAGE_SIZE = 0x1000;

// Sets errno on error.
inline static void *os_alloc(size_t size) {
    void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        return nullptr;
    }

    return ptr;
}

// Returns error code from munmap, sets errno on error.
inline static int os_free(void *ptr, size_t size) {
    return munmap(ptr, size);
}

#endif // OS_ALLOCATOR_H
