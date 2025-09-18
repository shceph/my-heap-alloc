#ifndef FALLBACK_CHUNK_H
#define FALLBACK_CHUNK_H

#include <stdalign.h>
#include <stddef.h>

#define FALLBACK_CHUNK_ALIGN (alignof(max_align_t))

typedef struct FallbackChunk {
    // Last bit represents if the chunk is used
    alignas(FALLBACK_CHUNK_ALIGN) size_t attr;
    struct FallbackChunk *prev;
    struct FallbackChunk *next;
} FallbackChunk;

#define FALLBACK_MIN_CHUNK_SIZE (sizeof(FallbackChunk) + FALLBACK_CHUNK_ALIGN)

#define FALLBACK_CHUNK_USED_BIT  (0x1UL)
#define FALLBACK_CHUNK_FLAG_BITS (FALLBACK_CHUNK_ALIGN - 1)
#define FALLBACK_CHUNK_SIZE_BITS (~FALLBACK_CHUNK_FLAG_BITS)

inline static size_t fallback_align_up(size_t size) {
    return (size + FALLBACK_CHUNK_ALIGN - 1) & ~(FALLBACK_CHUNK_ALIGN - 1);
}

inline static bool fallback_chunk_is_used(const FallbackChunk *chunk) {
    return (chunk->attr & FALLBACK_CHUNK_USED_BIT) != 0;
}

inline static void fallback_chunk_set_used(FallbackChunk *chunk, bool val) {
    if (val) {
        chunk->attr |= FALLBACK_CHUNK_USED_BIT;
    } else {
        chunk->attr &= ~FALLBACK_CHUNK_USED_BIT;
    }
}

inline static size_t fallback_chunk_size(const FallbackChunk *chunk) {
    return chunk->attr & FALLBACK_CHUNK_SIZE_BITS;
}

inline static bool fallback_chunk_get_bit(const FallbackChunk *chunk,
                                          size_t bit) {
    return (chunk->attr & bit) != 0;
}

inline static void fallback_chunk_set_bits_to_1(FallbackChunk *chunk,
                                                size_t bits) {
    chunk->attr |= bits;
}

inline static void fallback_chunk_set_bits_to_0(FallbackChunk *chunk,
                                                size_t bits) {
    chunk->attr &= (~bits);
}

// The size needs to be aligned by FALLBACK_CHUNK_ALIGN.
inline static void fallback_chunk_set_size(FallbackChunk *chunk, size_t size) {
    chunk->attr = size + (chunk->attr & FALLBACK_CHUNK_FLAG_BITS);
}

inline static void fallback_chunk_reset_flags(FallbackChunk *chunk) {
    chunk->attr &= ~FALLBACK_CHUNK_FLAG_BITS;
}

#endif // FALLBACK_CHUNK_H
