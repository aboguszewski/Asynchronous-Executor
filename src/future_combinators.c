#include "future_combinators.h"
#include <stdlib.h>

#include "future.h"
#include "waker.h"

#define THEN_FUTURE_ERR_SCHROEDINGERS_FUT1 424242

static FutureState then_future_progress(Future* fut, Mio* mio, Waker waker) {
    ThenFuture* self = (ThenFuture*) fut;
    if (self->fut1_completed == false) {
        FutureState fut1_state = self->fut1->progress(self->fut1, mio, waker);
        
        switch (fut1_state) {
            case FUTURE_COMPLETED:
                self->fut1_completed = true;
                break;
            case FUTURE_FAILURE:
                self->base.errcode = THEN_FUTURE_ERR_FUT1_FAILED;
                return FUTURE_FAILURE;
            default:
                /* 
                ** Future1 is PENDING and waker_wake() will be called eventually.
                ** Future1 has it's parent waker so ThenFuture will be woken up.
                */
                return FUTURE_PENDING;
        }
    } 
    
    if (self->fut1_completed == true) {
        self->fut2->arg = self->fut1->ok;
        FutureState fut2_state = self->fut2->progress(self->fut2, mio, waker);
        
        switch (fut2_state) {
            case FUTURE_COMPLETED:
                self->base.ok = self->fut2->ok;
                return FUTURE_COMPLETED; 
            case FUTURE_FAILURE:
                self->base.errcode = THEN_FUTURE_ERR_FUT2_FAILED;
                return FUTURE_FAILURE;
            default:
                /* 
                ** Future2 is PENDING and waker_wake() will be called eventually.
                ** Future2 has it's parent waker so ThenFuture will be woken up.
                */
                return FUTURE_PENDING;
        }
    }

    self->base.errcode = THEN_FUTURE_ERR_SCHROEDINGERS_FUT1;
    return FUTURE_FAILURE;
}

ThenFuture future_then(Future* fut1, Future* fut2) {
    return (ThenFuture) {
        .base = future_create(then_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = false
    };
}

static FutureState join_future_progress(Future* fut, Mio* mio, Waker waker) {
    JoinFuture* self = (JoinFuture*) fut;
    if (self->fut1_completed == false) {
        FutureState fut1_state = self->fut1->progress(self->fut1, mio, waker);
        switch (fut1_state) {
            case FUTURE_COMPLETED:
                self->fut1_completed = true;
                self->result.fut1.ok = self->fut1->ok;
                break;
            case FUTURE_FAILURE:
                self->fut1_completed = true;
                self->result.fut1.errcode = self->fut1->errcode;
                break;
            default:
                break;
        }
    }

    if (self->fut2_completed == false) {
        FutureState fut2_state = self->fut2->progress(self->fut2, mio, waker);
        switch (fut2_state) {
            case FUTURE_COMPLETED:
                self->fut2_completed = true;
                self->result.fut2.ok = self->fut2->ok;
                break;
            case FUTURE_FAILURE:
                self->fut2_completed = true;
                self->result.fut2.errcode = self->fut2->errcode;
                break;
            default:
                break;
        }
    }

    if (self->fut1_completed == false || self->fut2_completed == false) {
        /*
        ** At least one of the Futures is PENDING,
        ** so waker_wake() will eventually be called on the JoinFuture's waker.
        */
        return FUTURE_PENDING;
    } else if (self->result.fut1.errcode != FUTURE_SUCCESS || self->result.fut2.errcode != FUTURE_SUCCESS) {
        return FUTURE_FAILURE;
    } else {
        return FUTURE_COMPLETED;
    }
}

JoinFuture future_join(Future* fut1, Future* fut2) {
    struct JoinResult result = {
        .fut1 = {
            .errcode = FUTURE_SUCCESS,
            .ok = NULL
        },
        .fut2 = {
            .errcode = FUTURE_SUCCESS,
            .ok = NULL 
        }
    };

    return (JoinFuture) {
        .base = future_create(join_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = false,
        .fut2_completed = false,
        .result = result
    };
}

static FutureState select_future_progress(Future* fut, Mio* mio, Waker waker) {
    SelectFuture* self = (SelectFuture*) fut;

    if (self->which_completed == SELECT_COMPLETED_NONE 
        || self->which_completed == SELECT_FAILED_FUT2
        || self->fut1->is_active) {
        self->fut1->is_active = true;
        FutureState fut1_state = self->fut1->progress(self->fut1, mio, waker);
        switch (fut1_state) {
            case FUTURE_COMPLETED:
                self->fut1->is_active = false;
                if (self->which_completed != SELECT_COMPLETED_FUT2) {
                    self->which_completed = SELECT_COMPLETED_FUT1;
                }
                break;
            case FUTURE_FAILURE:
                self->fut1->is_active = false;
                if (self->which_completed == SELECT_COMPLETED_NONE) {
                    self->which_completed = SELECT_FAILED_FUT1;
                } else if (self->which_completed == SELECT_FAILED_FUT2) {
                    self->which_completed = SELECT_FAILED_BOTH;
                }
                break;
            default:
                break;
        }
    }

    if (self->which_completed == SELECT_COMPLETED_NONE 
        || self->which_completed == SELECT_FAILED_FUT1
        || self->fut2->is_active) {
        self->fut2->is_active = true;
        FutureState fut2_state = self->fut2->progress(self->fut2, mio, waker);
        switch (fut2_state) {
            case FUTURE_COMPLETED:
                self->fut2->is_active = false;
                if (self->which_completed != SELECT_COMPLETED_FUT1) {
                    self->which_completed = SELECT_COMPLETED_FUT2;
                }
                break;
            case FUTURE_FAILURE:
                self->fut2->is_active = false;
                if (self->which_completed == SELECT_COMPLETED_NONE) {
                    self->which_completed = SELECT_FAILED_FUT2;
                } else if (self->which_completed == SELECT_FAILED_FUT1) {
                    self->which_completed = SELECT_FAILED_BOTH;
                }
                break;
            default:
                break;
        }
    }

    if (self->fut1->is_active == false && self->fut2->is_active == false) {
        switch (self->which_completed) {
            case SELECT_COMPLETED_FUT1:
                self->base.ok = self->fut1->ok;
                return FUTURE_COMPLETED;
            case SELECT_COMPLETED_FUT2:
                self->base.ok = self->fut2->ok;
                return FUTURE_COMPLETED;
            case SELECT_FAILED_BOTH:
                self->base.errcode = self->fut1->errcode;
                return FUTURE_FAILURE;
            default:
                break;
        }
    }

    /*
    ** At least one of the Futures is PENDING,
    ** so waker_wake() will eventually be called on the SelectFuture's waker.
    */
    return FUTURE_PENDING;
}

SelectFuture future_select(Future* fut1, Future* fut2) {
    return (SelectFuture) {
        .base = future_create(select_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .which_completed = SELECT_COMPLETED_NONE
    };
}
