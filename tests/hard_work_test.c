#include <stdint.h> // For uint64_t
#include <stdio.h> // For printf
#include <stdlib.h> // For exit
#include <time.h>
#include <unistd.h> // For pipe, read, write

#include "assert.h"
#include "executor.h"
#include "future.h"
#include "waker.h"

#define MAX_COUNT 100

/** A future that simulates hard work in small stages. */
static FutureState hard_work_future_progress(Future* fut, Mio* mio, Waker waker)
{
    int* percentage_done = fut->arg;

    // Simulate long computation (each stage takes 0-200ms).
    usleep(random() % 200000);

    *percentage_done += 2;

    if (*percentage_done < MAX_COUNT) {
        // Yield (requeue us for later and allow other tasks to run).
        waker_wake(&waker);
        return FUTURE_PENDING;
    } else {
        return FUTURE_COMPLETED;
    }
}

/** A future that continuously displays the progress of the hard work. */
static FutureState ui_future_progress(Future* fut, Mio* mio, Waker waker)
{
    int* percentage_done = fut->arg;

    // Get the current time
    time_t raw_time;
    struct tm* time_info;
    time(&raw_time); // Get raw time in seconds since the epoch
    time_info = localtime(&raw_time); // Convert to local time

    printf("\033[H\033[2J" // Clear the screen.
           "percentage_done =% 3d%%" // Print progress of the hard work.
           " at %02d:%02d:%02d\n", // Print time in HH:MM:SS format.
        *percentage_done, time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    if (*percentage_done < MAX_COUNT) {
        // Yield.
        // (Ideally we would defer the waker to some screen update frequency,
        // but this would require Mio and timerfd, which we wanted to avoid here.)
        waker_wake(&waker);
        return FUTURE_PENDING;
    } else {
        return FUTURE_COMPLETED;
    }
}

int main()
{
    // A test that demonstrates the use of just futures and executors, without Mio.
    // In this example we have no I/O: just two futures that can always progress.
    // No threads, the tasks just yield to each other frequently enough to be seamless.

    Executor* executor = executor_create(42);

    int percentage_done = 0;

    struct Future hard_work_future = future_create(hard_work_future_progress);
    hard_work_future.arg = &percentage_done;
    struct Future ui_future = future_create(ui_future_progress);
    ui_future.arg = &percentage_done;

    executor_spawn(executor, (Future*)&hard_work_future);
    executor_spawn(executor, (Future*)&ui_future);

    executor_run(executor);

    executor_destroy(executor);

    return 0;
}
