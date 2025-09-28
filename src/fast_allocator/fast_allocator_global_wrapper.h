#ifndef FAST_ALLOCATOR_GLOBAL_WRAPPER
#define FAST_ALLOCATOR_GLOBAL_WRAPPER

#include "fast_allocator.h"

#include <stddef.h>

void *falloc(size_t size);
void ffree(void *ptr);
FastAllocator *falloc_get_instance();

#endif // FAST_ALLOCATOR_GLOBAL_WRAPPER
