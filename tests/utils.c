// Required for `unistd.h` include to contain `pipe2`.
#define _GNU_SOURCE

#include "utils.h"

#include <fcntl.h> // For O_NONBLOCK
#include <stdio.h> // For printf
#include <stdlib.h> // For exit
#include <string.h> // For strlen
#include <unistd.h> // For pipe, read, write

#include "debug.h"
#include "err.h"

int create_example_read_pipe_end(
    const char* message, size_t write_length, unsigned write_delay, unsigned close_delay)
{
    // Create a pipe
    int pipe_fds[2];
    ASSERT_SYS_OK(pipe2(pipe_fds, O_NONBLOCK));
    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    debug("Will write message='%s'\n", message);

    // In a separate process, write MESSAGE into the pipe.
    if (fork() == 0) {
        ASSERT_SYS_OK(close(read_fd));

        // We write len + 1 bytes in total to include '\0'.
        size_t const n_bytes = strlen(message) + 1;
        for (size_t start = 0; start < n_bytes; start += write_length) {
            ASSERT_SYS_OK(sleep(write_delay));
            size_t const n = write_length > n_bytes - start ? n_bytes - start : write_length;
            ASSERT_SYS_OK(write(write_fd, message + start, n));
        }
        ASSERT_SYS_OK(sleep(close_delay));
        ASSERT_SYS_OK(close(write_fd));
        exit(0);
    }

    ASSERT_SYS_OK(close(write_fd));
    return read_fd;
}