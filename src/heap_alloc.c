#include <sys/mman.h>

#include "heap_alloc.h"

heap_allocator_t heap_allocator_create(size_t size) {
    void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    heap_allocator_t allocator = {
        .size             = size,
        .begin            = ptr,
        .chunk_llist_head = (chunk_llist_t *)ptr,
    };

    allocator.chunk_llist_head->used = false;
    allocator.chunk_llist_head->size = size;
    allocator.chunk_llist_head->next = nullptr;

    return allocator;
}

void *heap_alloc(heap_allocator_t *allocator, size_t size) {
    if (allocator == nullptr || size == 0) {
        return nullptr;
    }

    chunk_llist_t *chunk = allocator->chunk_llist_head;

    while (chunk != nullptr) {
        if (chunk->used) {
            chunk = chunk->next;
            continue;
        }

        size_t new_chunk_size = size + sizeof(chunk_llist_t);

        if (new_chunk_size > chunk->size) {
            chunk = chunk->next;
            continue;
        }

        if (chunk->size - new_chunk_size < MIN_CHUNK_SIZE) {
            chunk->used = true;
            return (void *)(chunk + 1);
        }

        chunk_llist_t *curr_chunk_new_location =
            (chunk_llist_t *)((char *)chunk + new_chunk_size);
        curr_chunk_new_location->used = chunk->used;
        curr_chunk_new_location->size = chunk->size - new_chunk_size;
        curr_chunk_new_location->next = chunk->next;

        chunk->used = true;
        chunk->size = new_chunk_size;
        chunk->next = curr_chunk_new_location;

        return (void *)(chunk + 1);
    }

    return nullptr;
}

void heap_free(heap_allocator_t *allocator, void *ptr) {
    if (allocator == nullptr) {
        return;
    }

    chunk_llist_t *parent = nullptr;
    chunk_llist_t *chunk  = allocator->chunk_llist_head;

    while (chunk != nullptr) {
        if (chunk + 1 != ptr) {
            parent = chunk;
            chunk  = chunk->next;
            continue;
        }

        chunk->used          = false;
        chunk_llist_t *child = chunk->next;

        if (child != nullptr && !child->used) {
            chunk->next = child->next;
            chunk->size += child->size;
        }

        if (parent != nullptr && !parent->used) {
            parent->next = chunk->next;
            parent->size += chunk->size;
        }

        return;
    }
}
