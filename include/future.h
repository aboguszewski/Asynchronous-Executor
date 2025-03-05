#ifndef FUTURE_H
#define FUTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mio.h"
#include "waker.h"

/** Represents the possible states of a Future. */
typedef enum FutureState {
    FUTURE_PENDING, // Future is not yet complete and .progress() may need to called again.
    FUTURE_COMPLETED, // Future is complete, and its result can be retrieved.
    FUTURE_FAILURE, // Future has failed, and its result contains the error code.
} FutureState;

/** The type of a pointer to a function that progresses a future.
 *
 * The function defines what it means to make progress on the future (execute a stage of it).
 *
 * This is repeatedly called by the executor until FUTURE_COMPLETED or FUTURE_FAILURE is returned.
 * Otherwise, if FUTURE_PENDING is returned, the function must ensure that the provided waker is
 * called (now or later, directly or e.g. through Mio) to notify the executor when progress is
 * unblocked.
 *
 * BEWARE: `progress` should not be called by the executor anymore once FUTURE_COMPLETED or
 * FUTURE_FAILED is returned. Behaviour in such situation shall be considered undefined.
 *
 * @param self Pointer to the future instance.
 * @param mio  Pointer to the Mio instance.
 * @param waker Waker (the data needed to call waker_wake).
 * @return FutureState FUTURE_COMPLETED if the future is complete successfully,
 *                     FUTURE_FAILURE if execution failed,
 *                     FUTURE_PENDING if we need to be called again.
 */
typedef FutureState (*ProgressFn)(Future*, Mio*, Waker);

/** The no-error code. */
#define FUTURE_SUCCESS 0

/** Represents an asynchronous computation that produces a value in the future.
 *
 * Stores an input argument, the result and error code, and the state of the future's progress
 * (any state that needs to be remembered between calls to future.progress()).
 */
struct Future {
    /** Make progress towards the future's completion, see the `ProgressFn` typedef. */
    ProgressFn progress;

    /**
     * Tells whether the future is being executed by some executor (spawned but not yet finished).
     *
     * The flag has the following semantics:
     * - It should be initially unset (at construction).
     * - `executor_spawn()` must set the flag.
     * - When the executor completes the execution of the future (FUTURE_COMPLETED
     *   or FUTURE_FAILURE is returned from the progress() method), it unsets the flag.
     * - Only the executor is allowed to modify this flag.
     *
     * BEWARE: as the executor holds pointers to the future when it is executed,
     * the future must not be moved or deallocated (e.g., fall out of scope if
     * allocated on stack or be freed if allocated on heap). When this flag is
     * set, the future must be considered "pinned" - it must live until the
     * executor unsets the flag.
     */
    bool is_active;

    void* arg; // An optional input argument of the future.
    void* ok; // An optional result; only meaningful if `progress` returned FUTURE_COMPLETED.
    int errcode; // Only meaningful if `progress` returned FUTURE_FAILURE or FUTURE_COMPLETED.
};

static inline Future future_create(ProgressFn progress_fn)
{
    return (Future) {
        .progress = progress_fn,
        .is_active = false,
        .errcode = FUTURE_SUCCESS,
        .arg = NULL,
        .ok = NULL,
    };
}

#endif // FUTURE_H
