#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stddef.h>

#include "mio.h"

typedef struct Future Future;

/**
 * Represents an executor that drives futures to completion.
 *
 * The executor maintains a queue of futures that can progress, calls future.progress, and waits
 * with mio.poll when all pending futures a blocked.
 */

typedef struct Executor Executor;

/** Creates a new executor (with a specified queue size). */
Executor* executor_create(size_t max_queue_size);

/**
 * Submits a future to be managed by the executor.
 *
 * The future will be progressed (in `executor_run()`) until complete.
 */
void executor_spawn(Executor* executor, Future* fut);

/**
 * Runs the executor, driving futures to completion.
 *
 * The executor continuously calls future.progress() and processes events from the MIO layer.
 * This function blocks until all spawned futures are completed.
 */
void executor_run(Executor* executor);

/** Destroys the executor and frees its resources. */
void executor_destroy(Executor* executor);

#endif // EXECUTOR_H
