#ifndef FALLBACK_CHUNK_H
#define FALLBACK_CHUNK_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#define FALLBACK_CHUNK_ALIGN (alignof(max_align_t))

struct FallbackChunk {
    // Last bit represents if the chunk is used
    alignas(FALLBACK_CHUNK_ALIGN) size_t attr;
    struct FallbackChunk *prev;
    struct FallbackChunk *next;
};

#define FALLBACK_MIN_CHUNK_SIZE                                                \
    (sizeof(struct FallbackChunk) + FALLBACK_CHUNK_ALIGN)

#define FALLBACK_CHUNK_USED_BIT  (0x1UL)
#define FALLBACK_CHUNK_FLAG_BITS (FALLBACK_CHUNK_ALIGN - 1)
#define FALLBACK_CHUNK_SIZE_BITS (~FALLBACK_CHUNK_FLAG_BITS)

static inline size_t fallback_align_up(size_t size) {
    return (size + FALLBACK_CHUNK_ALIGN - 1) & ~(FALLBACK_CHUNK_ALIGN - 1);
}

static inline bool fallback_chunk_is_used(const struct FallbackChunk *chunk) {
    return (chunk->attr & FALLBACK_CHUNK_USED_BIT) != 0;
}

static inline void fallback_chunk_set_used(struct FallbackChunk *chunk,
                                           bool val) {
    if (val) {
        chunk->attr |= FALLBACK_CHUNK_USED_BIT;
    } else {
        chunk->attr &= ~FALLBACK_CHUNK_USED_BIT;
    }
}

static inline size_t fallback_chunk_size(const struct FallbackChunk *chunk) {
    return chunk->attr & FALLBACK_CHUNK_SIZE_BITS;
}

static inline bool fallback_chunk_get_bit(const struct FallbackChunk *chunk,
                                          size_t bit) {
    return (chunk->attr & bit) != 0;
}

static inline void fallback_chunk_set_bits_to_1(struct FallbackChunk *chunk,
                                                size_t bits) {
    chunk->attr |= bits;
}

static inline void fallback_chunk_set_bits_to_0(struct FallbackChunk *chunk,
                                                size_t bits) {
    chunk->attr &= (~bits);
}

// The size needs to be aligned by FALLBACK_CHUNK_ALIGN.
static inline void fallback_chunk_set_size(struct FallbackChunk *chunk,
                                           size_t size) {
    chunk->attr = size + (chunk->attr & FALLBACK_CHUNK_FLAG_BITS);
}

static inline void fallback_chunk_reset_flags(struct FallbackChunk *chunk) {
    chunk->attr &= ~FALLBACK_CHUNK_FLAG_BITS;
}

#endif // FALLBACK_CHUNK_H
