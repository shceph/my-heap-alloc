#ifndef STACK_DECLARATION_H
#define STACK_DECLARATION_H

#define STACK_DECLARE(DATA_TYPE, SIZE_TYPE, NAME)                              \
    struct NAME {                                                              \
        DATA_TYPE *data;                                                       \
        SIZE_TYPE size;                                                        \
        SIZE_TYPE capacity;                                                    \
    };

#endif // STACK_DECLARATION_H
