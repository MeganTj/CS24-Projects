/*
 * General implementation of semaphores.
 *
 *--------------------------------------------------------------------
 * Adapted from code for CS24 by Jason Hickey.
 * Copyright (C) 2003-2010, Caltech.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "sthread.h"
#include "semaphore.h"
#include "glue.h"
#include "queue.h"

/*
 * The semaphore data structure contains an integer value and a Queue
 * that holds blocked threads. Newly blocked threads are added to the
 * end of the queue, and the first thread in the queue is the first to
 * be unblocked.
 */
struct _semaphore {
    /* The value of the semaphore. */
    int value;

    /* The queue containing blocked threads. */
    Queue *blocked;
    
};

/************************************************************************
 * Top-level semaphore implementation.
 */

/*
 * Allocate a new semaphore.  The initial value of the semaphore is
 * specified by the argument.
 */
Semaphore *new_semaphore(int init) {
    Semaphore *semp = NULL;

    semp = (Semaphore *) malloc(sizeof(Semaphore));
    if (semp == NULL) {
        fprintf(stderr, "Error: out of memory when attempting to " \
                "allocate Semaphore");
        exit(1);
    }
    semp->value = init;
    semp->blocked = (Queue *) malloc(sizeof(Queue));
    if (semp->blocked == NULL) {
        fprintf(stderr, "Error: out of memory when attempting to " \
                "allocate blocked queue.");
        exit(1);
    }
    return semp;
}

/*
 * Decrement the semaphore.
 * This operation must be atomic, and it blocks iff the semaphore is zero.
 */
void semaphore_wait(Semaphore *semp) {
    /*
     * Thread acquires the lock so that the thread will not be interrupted 
     * while waiting for the semaphore's value to be greater than 0.
     */
    __sthread_lock();
    while (semp->value == 0) {
        /* Block the thread and add to the end of the queue. */
        queue_append(semp->blocked, sthread_current());
        sthread_block();
        /*
         * Thread acquires the lock so that the thread will not be interrupted 
         * while waiting for the semaphore's value to be greater than 0.
         */
        __sthread_lock();
    }
    semp->value--;
    /*
     * Thread releases the lock when it is done waiting.
     */
    __sthread_unlock();
}

/*
 * Increment the semaphore.
 * This operation must be atomic.
 */
void semaphore_signal(Semaphore *semp) {
    /*
     * Thread acquires the lock so that it will not be interrupted 
     * while the appropriate semaphore signals its status.
     */
    __sthread_lock();
    semp->value++;
    if (queue_empty(semp->blocked) == 0) {
        /* Unblock the thread at the beginning of the queue.*/
        Thread *unblocked = queue_take(semp->blocked);
        sthread_unblock(unblocked);
    }
    else {
        /* Thread releases the lock when the semaphore is done signaling. */
        __sthread_unlock();
    }
}

