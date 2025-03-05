// Required for `unistd.h` include to contain `pipe2`.
#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <stdint.h> // For uint64_t
#include <stdio.h> // For printf
#include <stdlib.h> // For exit
#include <string.h> // For memcmp
#include <sys/timerfd.h> // For timerfd
#include <unistd.h> // For pipe, read, write

#include "err.h"
#include "executor.h"
#include "future.h"
#include "future_combinators.h"
#include "future_examples.h"
#include "mio.h"
#include "utils.h"


/** A function that capitalizes a c-string in place and returns it. */
void* capitalize(void* arg)
{
    char* buffer = arg;
    debug("capitalize received string at: %p\n", buffer);
    size_t const len = strlen(buffer);
    debug("capitalize received string of length %d\n", len);
    for (int i = 0; i < len; ++i) {
        if ('a' <= buffer[i] && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 'a' + 'A';
        }
    }
    return buffer;
}

int main()
{
    // An end-to-end test that demonstrates the use of ThenFuture to chain futures.

    Executor* executor = executor_create(42);

    // Create a pipe that will be filled with `message` 3 bytes at a time,
    // then wait 2 seconds before closing the write end.
    const char* message = "aaabbbcccd\n";
    int read_fd = create_example_read_pipe_end(message, 3, 0, 2);

    // Make a future that reads the message from the pipe, then capitalizes it, the prints it.
    // This should succeed, because we read exactly all the bytes of message, including '\0'.
    uint8_t buffer[strlen(message) + 1];
    PipeReadFuture f1 = pipe_read_future_create(read_fd, buffer, sizeof(buffer));
    ApplyFuture f2 = apply_future_create(capitalize);
    ThenFuture f12 = future_then((Future*)&f1, (Future*)&f2);
    PipeWriteFuture f3 = pipe_write_future_create(STDOUT_FILENO, sizeof(buffer), true);
    ThenFuture f123 = future_then((Future*)&f12, (Future*)&f3);
    Future* good_future = (Future*)&f123;

    // Make another future for another pipe, as before, but with a larger buffer.
    // This should progress concurrently with good_future, but eventually fail
    // (only after the write-end of the pipe is closed), because there aren't enough bytes to read.
    read_fd = create_example_read_pipe_end(message, 3, 0, 2);
    uint8_t large_buffer[1000];
    PipeReadFuture f1_bad = pipe_read_future_create(read_fd, large_buffer, sizeof(large_buffer));
    ApplyFuture f2_bad = apply_future_create(capitalize);
    ThenFuture f12_bad = future_then((Future*)&f1_bad, (Future*)&f2_bad);
    PipeWriteFuture f3_bad = pipe_write_future_create(STDOUT_FILENO, sizeof(large_buffer), true);
    ThenFuture f123_bad = future_then((Future*)&f12_bad, (Future*)&f3_bad);

    executor_spawn(executor, (Future*)&f123_bad);
    executor_spawn(executor, good_future);

    // Run the executor
    executor_run(executor);

    // Check everything is as expected.
    assert(good_future->errcode == FUTURE_SUCCESS);
    assert(good_future->ok == buffer); // capitalize and PipeWriteFuture just return the buffer.
    assert(memcmp(buffer, "AAABBBCCCD\n", sizeof(buffer)) == 0); // the buffer was capitalized.

    assert(f123_bad.base.errcode == THEN_FUTURE_ERR_FUT1_FAILED); // f123_bad failed because f12_bad failed.
    assert(f12_bad.base.errcode == THEN_FUTURE_ERR_FUT1_FAILED); // f12_bad failed because f1_bad failed.
    assert(f1_bad.base.errcode == PIPE_FUTURE_ERR_EOF); // f1_bad failed because it reached EOF.
    assert(((ThenFuture*)f123_bad.fut1)->fut1 == (Future*)&f1_bad);
    assert(memcmp(large_buffer, message, sizeof(buffer)) == 0);  // f1_bad read until EOF, but capitalize was never called.

    // Destroy the executor
    executor_destroy(executor);

    return 0;
}
