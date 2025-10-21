#ifndef STACK_DEFINITION_H
#define STACK_DEFINITION_H

#include <assert.h>

enum StackError {
    STACK_OK,
    STACK_FULL,
    STACK_EMPTY,
};

#define STACK_DEFINE(DATA_TYPE, SIZE_TYPE, NAME)                               \
    static inline struct NAME NAME##_init(void *mem, SIZE_TYPE capacity) {     \
        return (struct NAME){                                                  \
            .data = (DATA_TYPE *)mem,                                          \
            .size = 0,                                                         \
            .capacity = capacity,                                              \
        };                                                                     \
    }                                                                          \
                                                                               \
    static inline enum StackError NAME##_try_push(struct NAME *self,           \
                                                  DATA_TYPE data) {            \
        if (self->size >= self->capacity) {                                    \
            return STACK_FULL;                                                 \
        }                                                                      \
                                                                               \
        self->data[self->size++] = data;                                       \
        return STACK_OK;                                                       \
    }                                                                          \
                                                                               \
    static inline void NAME##_push(struct NAME *self, DATA_TYPE data) {        \
        assert(self->size < self->capacity);                                   \
        self->data[self->size++] = data;                                       \
    }                                                                          \
                                                                               \
    static inline enum StackError NAME##_try_pop(                              \
        struct NAME *self, /* NOLINTNEXTLINE(bugprone-macro-parentheses)*/     \
        DATA_TYPE *out) {                                                      \
        if (self->size == 0) {                                                 \
            return STACK_EMPTY;                                                \
        }                                                                      \
                                                                               \
        *out = self->data[--self->size];                                       \
        return STACK_OK;                                                       \
    }                                                                          \
                                                                               \
    static inline DATA_TYPE NAME##_pop(struct NAME *self) {                    \
        assert(self->size > 0);                                                \
        return self->data[--self->size];                                       \
    }

#endif // STACK_DEFINITION_H
