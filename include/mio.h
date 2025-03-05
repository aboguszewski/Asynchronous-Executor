#ifndef MIO_H
#define MIO_H

#include <stdint.h> // For uint32_t

typedef struct Executor Executor;

/** Represents the MIO event loop instance. */
typedef struct Mio Mio;

/** Represents a mechanism to wake up a task when an event occurs. */
typedef struct Waker Waker;

/** Creates a new MIO event loop instance (NULL on failure). */
Mio* mio_create(Executor* executor);

/** Destroys a MIO instance and releases its resources. */
void mio_destroy(Mio* mio);

/**
 * Registers a file descriptor with MIO to monitor specific events.
 *
 * When the specified events occur on the file descriptor, the associated Waker is invoked.
 *
 * @param mio Pointer to the Mio instance.
 * @param fd File descriptor to register.
 * @param events Events to monitor (EPOLLIN or EPOLLOUT for read or write availability).
 * @param waker Waker that will be notified on events.
 * @return 0 on success, -1 on failure.
 */
int mio_register(Mio* mio, int fd, uint32_t events, Waker waker);

/** Unregisters a file descriptor from MIO. Returns 0 on success, -1 on failure. */
int mio_unregister(Mio* mio, int fd);

/** Waits for any ready event and invokes their Wakers. */
void mio_poll(Mio* mio);

#endif // MIO_H
