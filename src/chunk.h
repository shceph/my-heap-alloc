#include <stdalign.h>
#include <stddef.h>

#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_ALIGN    (alignof(max_align_t))
#define MIN_CHUNK_SIZE (sizeof(chunk_t) + CHUNK_ALIGN)

typedef struct chunk_t {
    // Last bit represents if the chunk is used, the rest is the size.
    // It's assumed that the size is 2 byte aligned.
    alignas(CHUNK_ALIGN) size_t attr;
    struct chunk_t *prev;
    struct chunk_t *next;
} chunk_t;

#define CHUNK_USED_BIT            (0x1UL)
#define CHUNK_START_OF_REGION_BIT (0x2UL)
#define CHUNK_FLAG_BITS           (CHUNK_ALIGN - 1)
#define CHUNK_SIZE_BITS           (~CHUNK_FLAG_BITS)

inline static size_t align_up(size_t size) {
    return (size + CHUNK_ALIGN - 1) & ~(CHUNK_ALIGN - 1);
}

inline static bool chunk_is_used(const chunk_t *chunk) {
    return (chunk->attr & CHUNK_USED_BIT) != 0;
}

inline static void chunk_set_used(chunk_t *chunk, bool val) {
    if (val) {
        chunk->attr |= CHUNK_USED_BIT;
    } else {
        chunk->attr &= ~CHUNK_USED_BIT;
    }
}

inline static size_t chunk_size(const chunk_t *chunk) {
    return chunk->attr & CHUNK_SIZE_BITS;
}

inline static bool chunk_get_bit(const chunk_t *chunk, size_t bit) {
    return (chunk->attr & bit) != 0;
}

inline static void chunk_set_bits_to_1(chunk_t *chunk, size_t bits) {
    chunk->attr |= bits;
}

inline static void chunk_set_bits_to_0(chunk_t *chunk, size_t bits) {
    chunk->attr &= (~bits);
}

// The size needs to be aligned by CHUNK_ALIGN.
inline static void chunk_set_size(chunk_t *chunk, size_t size) {
    chunk->attr = size + (chunk->attr & CHUNK_FLAG_BITS);
}

inline static void chunk_reset_flags(chunk_t *chunk) {
    chunk->attr &= ~CHUNK_FLAG_BITS;
}

#endif // CHUNK_H
