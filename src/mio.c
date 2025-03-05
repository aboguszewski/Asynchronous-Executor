#include "mio.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "debug.h"
#include "executor.h"
#include "waker.h"
#include "err.h"

// Maximum number of events to handle per epoll_wait call.
#define MAX_EVENTS 64

#define MAX_DESCRIPTORS 1048576
#define DONT_WAIT 0

/* 
** Explanation for exploiting MAX_DESCRIPTORS, if anyone reads this.
** In the previous version checking if a descriptor is or isn't registered,
** as well as finding the appropriate waker to call when a descriptor becomes
** available was implemented brutally. Mio stored an array (of size max_queue_size)
** containing pairs (file descriptor, waker) and every single time register/unregister/poll
** went through the whole array to find what they needed.
** It looked really slow, but a more elegant solution required (or so I thought) a dictionary,
** which I assumed was not expected from this assignment. I hope that this solution with an
** array of every possible file descriptor works (it should if I understood 'ulimit -n' properly).
** 
** PS If it doesn't work, please note that I also have the old Mio code and I can provide it if possible. 
*/

typedef enum FdState {
    MONITORED, NOT_MONITORED
} FdState;

typedef struct FdNode {
    FdState state;
    int fd;
    uint32_t events;
    Waker waker;
    struct FdNode* next;
    struct FdNode* back;
} FdNode;

struct Mio {
    FdNode fd[MAX_DESCRIPTORS];
    FdNode* head;
    size_t monitored;
};

Mio* mio_create(Executor* executor) {
    Mio* mio = (Mio*) malloc(sizeof(Mio));
    if (mio == NULL) {
        fatal("Malloc failure in mio_create()");
    }

    for (size_t i = 0; i < MAX_DESCRIPTORS; i++) {
        mio->fd[i].state = NOT_MONITORED;
    }
    mio->head = NULL;
    mio->monitored = 0;
    return mio;
}

void mio_destroy(Mio* mio) {
    free(mio);
}

int mio_register(Mio* mio, int fd, uint32_t events, Waker waker)
{
    if (mio->fd[fd].state == MONITORED) {
        return -1;
    }

    mio->fd[fd] = (FdNode) {
        .state = MONITORED,
        .fd = fd,
        .events = events,
        .waker = waker,
        .next = mio->head,
        .back = NULL
    };

    mio->head = &mio->fd[fd];
    if (mio->head->next != NULL) {
        mio->head->next->back = mio->head;
    }
    mio->monitored++;
    return 0;
}

int mio_unregister(Mio* mio, int fd)
{
    if (mio->fd[fd].state == NOT_MONITORED) {
        return -1;
    }

    mio->fd[fd].state = NOT_MONITORED;
    if (mio->fd[fd].next == NULL && mio->fd[fd].back != NULL) {
        mio->fd[fd].back->next = NULL;
    } else if (mio->fd[fd].back == NULL && mio->fd[fd].next != NULL) {
        mio->fd[fd].next->back = NULL;
        mio->head = mio->fd[fd].next;
    } else if (mio->fd[fd].next != NULL && mio->fd[fd].back != NULL){
        mio->fd[fd].back->next = mio->fd[fd].next;
        mio->fd[fd].next->back = mio->fd[fd].back;
    } else {
        mio->head = NULL;
    }
    mio->monitored--;
    return 0;
}

void mio_poll(Mio* mio)
{
    size_t not_polled = mio->monitored;
    FdNode* curr_fd = mio->head;
    while (not_polled > 0) {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            syserr("epoll_create1()");
        }
        size_t bound = not_polled < MAX_EVENTS ? not_polled : MAX_EVENTS;
        for (int i = 0; i < bound; i++) {
            struct epoll_event event;
            event.data.fd = curr_fd->fd;
            event.events = curr_fd->events;
            int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, curr_fd->fd, &event);
            if (ret == -1) {
                syserr("epoll_ctl()");
            }
            curr_fd = curr_fd->next;
        }
        not_polled -= bound;

        struct epoll_event events[MAX_EVENTS];
        int events_count = epoll_wait(epoll_fd, events, MAX_EVENTS, DONT_WAIT);
        if (events_count == -1) {
            syserr("epoll_wait()");
        }
        for (int i = 0; i < events_count; i++) {
            int fd_num = events[i].data.fd;
            waker_wake(&mio->fd[fd_num].waker);
            mio_unregister(mio, fd_num);
        }
        close(epoll_fd);
    }
}