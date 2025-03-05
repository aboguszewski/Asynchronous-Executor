#include "future_examples.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "debug.h"
#include "mio.h"
#include "waker.h"

/** Progress function for ApplyFuture */
static FutureState apply_future_progress(Future* fut, Mio* mio, Waker waker)
{
    ApplyFuture* self = (ApplyFuture*)fut;
    debug("ApplyFuture %p progress. Arg=%p\n", self, self->base.arg);

    self->base.ok = self->func(self->base.arg);

    return FUTURE_COMPLETED;
}

ApplyFuture apply_future_create(void* (*func)(void*))
{
    ;
    ApplyFuture apply_future = {
        .base = future_create(apply_future_progress),
        .func = func,
    };
    return apply_future;
}

/** Progress function for PipeReadFuture */
static FutureState pipe_read_progress(Future* base, Mio* mio, Waker waker)
{
    PipeReadFuture* self = (PipeReadFuture*)base;
    debug("PipeReadFuture %p progress. read_so_far=%zu, n=%zu\n", self, self->read_so_far, self->n);

    while (self->read_so_far < self->n) {
        // There are some bytes yet to be read. Try reading from the pipe.
        ssize_t const bytes_read
            = read(self->fd, self->buffer + self->read_so_far, self->n - self->read_so_far);
        debug("PipeReadFuture %p: read %zd, errno %s\n", self, bytes_read,
            strerror(bytes_read == -1 ? errno : 0));

        if (bytes_read == 0) {
            mio_unregister(mio, self->fd);
            self->base.errcode = PIPE_FUTURE_ERR_EOF;
            return FUTURE_FAILURE;
        } else if (bytes_read > 0) {
            self->read_so_far += bytes_read;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Could not read from pipe.
            // Register the FD with MIO to watch for readability.
            mio_register(mio, self->fd, EPOLLIN, waker);
            return FUTURE_PENDING;
        }
    }

    // Read enough bytes.
    mio_unregister(mio, self->fd);
    self->base.ok = self->buffer;
    return FUTURE_COMPLETED;
}

PipeReadFuture pipe_read_future_create(int fd, uint8_t* buffer, size_t n)
{
    return (PipeReadFuture) {
        .base = future_create(pipe_read_progress),
        .fd = fd,
        .buffer = buffer,
        .n = n,
        .read_so_far = 0,
    };
}

/** Progress function for PipeWriteFuture */
static FutureState pipe_write_progress(Future* base, Mio* mio, Waker waker)
{
    PipeWriteFuture* self = (PipeWriteFuture*)base;
    const char* buffer = self->base.arg;
    debug("PipeWriteFuture %p progress. written_so_far=%zu, n=%zu\n", self, self->written_so_far,
        self->n);

    // If the future was created with stop_on_zero_byte=true,
    // we interpret the input argument as a c-string and adjust n accordingly.
    if (self->stop_on_zero_byte) {
        size_t len = strnlen(buffer, self->n);
        if (len < self->n) {
            self->n = len + 1; // Include the zero byte.
        }
        self->stop_on_zero_byte = false;
    }

    while (self->written_so_far < self->n) {
        // There are some bytes yet to be written. Try writing to the pipe.
        ssize_t const bytes_written
            = write(self->fd, buffer + self->written_so_far, self->n - self->written_so_far);
        debug("PipeReadFuture %p: write %zd, errno %s\n", self, bytes_written,
            strerror(bytes_written == -1 ? errno : 0));

        if (bytes_written == 0) {
            mio_unregister(mio, self->fd);
            self->base.errcode = PIPE_FUTURE_ERR_EOF;
            return FUTURE_FAILURE;
        } else if (bytes_written > 0) {
            self->written_so_far += bytes_written;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Could not write from pipe.
            // Register the FD with MIO to watch for writeability.
            mio_register(mio, self->fd, EPOLLIN, waker);
            return FUTURE_PENDING;
        }
    }

    // Read enough bytes.
    mio_unregister(mio, self->fd);
    self->base.ok = (void*)buffer;
    return FUTURE_COMPLETED;
}

PipeWriteFuture pipe_write_future_create(int fd, size_t n, bool stop_on_zero_byte)
{
    return (PipeWriteFuture) {
        .base = future_create(pipe_write_progress),
        .fd = fd,
        .n = n,
        .written_so_far = 0,
        .stop_on_zero_byte = stop_on_zero_byte,
    };
}
