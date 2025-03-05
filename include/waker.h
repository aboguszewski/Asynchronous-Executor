#ifndef WAKER_H
#define WAKER_H

#include "debug.h"

typedef struct Executor Executor;
typedef struct Future Future;

/**
 * A Waker is used to notify the executor that a future is ready to make progress.
 *
 * The Waker is a callback mechanism where the `waker_wake` function is invoked to
 * notify the executor to enqueue the associated future into its queue.
 */
typedef struct Waker {
    void* executor; // Executor to be notified about the future.
    Future* future; // Future to be requeued up by executor.
} Waker;

/** Invoked when the associated future becomes ready. */
void waker_wake(struct Waker* waker);

static inline void debug_print_waker(Waker const* waker)
{
    debug("Waker { fut = %p, executor = %p }", waker->future, waker->executor);
}

#endif // WAKER_H