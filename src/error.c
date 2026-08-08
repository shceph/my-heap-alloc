#include "error.h"

#include <stdarg.h>
#include <stdio.h>

void fa_print_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    (void)vfprintf(stderr, fmt, args);

    va_end(args);
}

void fa_print_errno(const char *msg) {
    perror(msg);
}
