#include "executor.h"

#include <stdlib.h>

#include "debug.h"
#include "future.h"
#include "mio.h"
#include "waker.h"
#include "err.h"

typedef struct Queue {
    Future** array;
    size_t front;
    size_t back;
    size_t size;
    size_t max_size;
} Queue;

Queue* queue_create(size_t max_size) {
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    if (queue == NULL) {
        fatal("Malloc failure in queue_create()");
    }
    queue->array = (Future**) malloc(sizeof(Future*) * max_size);
    if (queue->array == NULL) {
        fatal("Malloc failure in queue_create()");
    }
    queue->max_size = max_size;
    queue->front = 0;
    queue->back = 0;
    queue->size = 0;
    return queue;
}

void queue_destroy(Queue* queue) {
    free(queue->array);
    free(queue);
}

void queue_push(Queue* queue, Future* fut) {
    if (queue->size == queue->max_size) {
        return;
    }

    queue->array[queue->back] = fut;
    queue->back = (queue->back + 1) % queue->max_size;
    queue->size++;
}

Future* queue_pop(Queue* queue) {
    if (queue->front == queue->back) {
        return NULL;
    }

    Future* ret = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->max_size;
    queue->size--;
    return ret;
}

int queue_empty(Queue* queue) {
    return queue->size == 0;
}

/**
 * @brief Structure to represent the current-thread executor.
 */
struct Executor {
    Queue* ready_tasks_queue;
    Mio* mio;
    size_t tasks_in_progress;
};

Executor* executor_create(size_t max_queue_size) {
    Executor* executor = (Executor*) malloc(sizeof(Executor));
    if (executor == NULL) {
        fatal("Malloc failure in executor_create()");
    }
    executor->ready_tasks_queue = queue_create(max_queue_size);
    executor->tasks_in_progress = 0;
    executor->mio = mio_create(executor);
    return executor;
}

void waker_wake(Waker* waker) {
    queue_push(((Executor*) waker->executor)->ready_tasks_queue, waker->future);
}

void executor_spawn(Executor* executor, Future* fut) {
    queue_push(executor->ready_tasks_queue, fut);
    executor->tasks_in_progress++;
    fut->is_active = true;
}

void executor_run(Executor* executor) { 
    while (executor->tasks_in_progress != 0) {
        if (queue_empty(executor->ready_tasks_queue)) {
            mio_poll(executor->mio);
        }

        while (!queue_empty(executor->ready_tasks_queue)) {
            Future* fut = queue_pop(executor->ready_tasks_queue);
            Waker waker = (Waker){executor, fut};
            FutureState status = fut->progress(fut, executor->mio, waker);
            if (status == FUTURE_COMPLETED || status == FUTURE_FAILURE) {
                executor->tasks_in_progress--;
                fut->is_active = false;
            } else {
                // Future returned PENDING and waker_wake() will be called eventually
            }
        }
    }
}

void executor_destroy(Executor* executor) {
    queue_destroy(executor->ready_tasks_queue);
    mio_destroy(executor->mio);
    free(executor);
}
