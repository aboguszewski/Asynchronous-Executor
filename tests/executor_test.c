#include <assert.h>
#include <stdio.h>

#include "executor.h"
#include "future_examples.h"

void* increment(void* arg)
{
    intptr_t number = (intptr_t)arg;
    return (void*)(number + 1);
}

int main()
{
    // A trivial example where we just call a function as a future, in the executor.

    Executor* executor = executor_create(1024);

    ApplyFuture fut = apply_future_create(increment);
    fut.base.arg = (void*)42;

    executor_spawn(executor, (Future*)&fut);

    executor_run(executor);

    intptr_t result = (intptr_t)fut.base.ok;
    printf("Result: %ld\n", (long)result);
    assert(result == 43);

    executor_destroy(executor);

    return 0;
}
