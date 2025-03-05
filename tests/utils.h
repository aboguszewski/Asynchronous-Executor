#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stddef.h>

/**
 * Return the read end of a new pipe that's slowly filled (by a subprocess).
 *
 * A subprocess is created to write `message` into the pipe, `write_length` bytes at a time,
 * with a before each write, and another delay before closing.
 */
int create_example_read_pipe_end(
    const char* message, size_t write_length, unsigned write_delay, unsigned close_delay);

#endif // TEST_UTILS_H
