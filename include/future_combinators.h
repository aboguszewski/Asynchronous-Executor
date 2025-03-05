#ifndef FUTURE_COMBINATORS_H
#define FUTURE_COMBINATORS_H

#include <stdbool.h>

#include "future.h"

#define THEN_FUTURE_ERR_FUT1_FAILED 1
#define THEN_FUTURE_ERR_FUT2_FAILED 2

/**
 * A combinator that chains two futures sequentially.
 *
 * When the first future (fut1) completes, its result is passed to the second
 * future (fut2) (fut2->arg := fut1->ok). The ThenFuture is considered
 * COMPLETED when fut2 is COMPLETED. If either of the futures returns FAILURE,
 * ThenFuture shall return FAILURE, too, and set the proper errcode (see above).
 */
typedef struct ThenFuture {
    Future base; // Base future structure
    Future* fut1; // First future to complete
    Future* fut2; // Second future to execute after fut1
    bool fut1_completed; // Flag indicating whether fut1 has completed
} ThenFuture;

/** Creates a ThenFuture that chains two futures sequentially. */
ThenFuture future_then(Future* fut1, Future* fut2);

#define JOIN_FUTURE_ERR_FUT1_FAILED 1
#define JOIN_FUTURE_ERR_FUT2_FAILED 2
#define JOIN_FUTURE_ERR_BOTH_FUTS_FAILED 3

/**
 * A combinator that executes two futures concurrently.
 *
 * The JoinFuture is considered COMPLETED when both fut1 and fut2 are COMPLETED.
 * JoinFuture shall progress both futures until completion even if one of the
 * futures returns FAILURE. JoinFuture shall save corresponding returns and
 * errcodes. Upon completion of both futures, JoinFuture returns FAILURE if
 * either of the futures returns FAILURE; else it returns COMPLETED.
 */
typedef struct JoinFuture {
    Future base; // Base future structure
    Future* fut1; // One future to execute
    Future* fut2; // Another future to execute
    FutureState fut1_completed;
    FutureState fut2_completed;
    struct JoinResult {
        struct {
            int errcode;
            void* ok;
        } fut1; // Result of the first future.
        struct {
            int errcode;
            void* ok;
        } fut2; // Result of the second future.
    } result;
} JoinFuture;

/** Creates a JoinFuture that executes two futures concurrently. */
JoinFuture future_join(Future* fut1, Future* fut2);

/**
 * A combinator that executes two futures until one of them completes.
 *
 * The SelectFuture is considered COMPLETED when fut1 or fut2 are COMPLETED.
 * SelectFuture shall progress the other future even if one of the futures returned FAILURE.
 * SelectFuture shall only propagate error (of whichever future) if both futures fail.
 */
typedef struct SelectFuture {
    Future base; // Base future structure
    Future* fut1; // One future to execute
    Future* fut2; // Another future to execute
    enum {
        SELECT_COMPLETED_NONE, // No future has completed yet.
        SELECT_COMPLETED_FUT1, // Future 1 has completed first.
        SELECT_COMPLETED_FUT2, // Future 2 has completed first.
        SELECT_FAILED_FUT1, // Future 1 has failed and future 2 has not yet completed.
        SELECT_FAILED_FUT2, // Future 2 has failed and future 1 has not yet completed.
        SELECT_FAILED_BOTH, // Both futures have failed.
    } which_completed;
} SelectFuture;

/** Creates a SelectFuture that executes two futures until one of them completes successfully. */
SelectFuture future_select(Future* fut1, Future* fut2);

#endif // FUTURE_COMBINATORS_H
