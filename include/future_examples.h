#ifndef FUTURE_EXAMPLES_H
#define FUTURE_EXAMPLES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "future.h"
#include "future_combinators.h"
#include "waker.h"

// ========================= ApplyFuture =========================
typedef struct ApplyFuture {
    Future base;
    void* (*func)(void*);
} ApplyFuture;

/**
 * Creates a future that calls a function and returns its result.
 *
 * The function will be given `future.arg` as its argument (NULL by default).
 */
ApplyFuture apply_future_create(void* (*func)(void*));

// ========================= PipeReadFuture =========================
typedef struct PipeReadFuture {
    Future base; // Base future structure
    int fd; // File descriptor to read from
    uint8_t* buffer; // Buffer to store the result
    size_t n; // Size of the buffer = number of bytes to be read
    size_t read_so_far; // Number of bytes read so far
} PipeReadFuture;

#define PIPE_FUTURE_ERR_EOF 1

/**
 * Creates a future that reads a fixed number of bytes from a pipe.
 *
 * The future will call read() from the specified file descriptor
 * until exactly n bytes are read or EOF is reached (write-end of pipe is closed).
 * In the latter case, resolves to FUTURE_FAILURE with errcode set to PIPE_FUTURE_ERR_EOF.
 */
PipeReadFuture pipe_read_future_create(int fd, uint8_t* buffer, size_t n);

// ========================= PipeWriteFuture =========================
typedef struct PipeWriteFuture {
    Future base; // Base future structure.
    int fd; // File descriptor to write to.
    size_t n; // Number of bytes to be write (input must be at least that size).
    bool stop_on_zero_byte; // Whether to stop writing after a zero byte is written.
    size_t written_so_far; // Number of bytes written so far.
} PipeWriteFuture;

/**
 * Creates a future that writes at most a fixed number of bytes to a pipe.
 *
 * The future will call write() to the specified file descriptor
 * until exactly n bytes are written or a zero byte is written (if stop_on_zero_byte is true).
 * Bytes to be written are taken from the argument of the future `(const char*)future->base.arg`.
 */
PipeWriteFuture pipe_write_future_create(int fd, size_t n, bool stop_on_zero_byte);

#endif // FUTURE_EXAMPLES_H
