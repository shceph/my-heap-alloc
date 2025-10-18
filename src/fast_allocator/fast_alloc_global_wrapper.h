#ifndef FAST_ALLOC_GLOBAL_WRAPPER_H
#define FAST_ALLOC_GLOBAL_WRAPPER_H

#include "fast_alloc.h"

#include <stddef.h>

void *falloc(size_t size);
void ffree(void *ptr);
void *frealloc(void *ptr, size_t size);
size_t fmemsize(void *ptr);
struct FaAllocator *falloc_get_instance();

#endif // FAST_ALLOC_GLOBAL_WRAPPER_H
