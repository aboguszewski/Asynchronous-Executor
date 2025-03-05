#ifndef MIM_ERR_H
#define MIM_ERR_H

#include <stdnoreturn.h>

/* Assert that expr evaluates to zero (otherwise use result as error number, as in pthreads). */
#define ASSERT_ZERO(expr)                                                                          \
    do {                                                                                           \
        int errno = (expr);                                                                        \
        if (errno != 0)                                                                            \
            syserr("Failed: %s\n\tIn function %s() in %s line %d.\n\tErrno: ", #expr, __func__,    \
                __FILE__, __LINE__);                                                               \
    } while (0)

/* Assert that expression doesn't evaluate to -1 (as almost every system function does on error).

    Use as a function, with a semicolon, like: ASSERT_SYS_OK(close(fd));
    (This is implemented with a 'do-while(0)' block to make it work between if () and else.)
*/
#define ASSERT_SYS_OK(expr)                                                                        \
    do {                                                                                           \
        if ((expr) == -1)                                                                          \
            syserr("System command failed: %s\n\tIn function %s() in %s line %d.\n\tErrno: ",      \
                #expr, __func__, __FILE__, __LINE__);                                              \
    } while (0)

/* Prints with information about system error (errno) and quits. */
_Noreturn extern void syserr(const char* fmt, ...);

/* Prints (like printf) and quits. */
_Noreturn extern void fatal(const char* fmt, ...);

#endif
