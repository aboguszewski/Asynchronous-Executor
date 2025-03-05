#ifndef DEBUG_H
#define DEBUG_H

// TODO: comment this to disable debug prints
#define DEBUG_PRINTS

#ifdef DEBUG_PRINTS

#include <stdarg.h>
#include <stdio.h>

static inline void debug(const char* fmt, ...)
{
    va_list fmt_args;
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
}

#else // DEBUG_PRINTS

#define perror(ARGS...)
#define debug(ARGS...)

#endif // DEBUG_PRINTS

#endif // DEBUG_H