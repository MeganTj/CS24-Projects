/*
 * Define a bounded buffer containing records that describe the
 * results in a producer thread.
 *
 *--------------------------------------------------------------------
 * Adapted from code for CS24 by Jason Hickey.
 * Copyright (C) 2003-2010, Caltech.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "sthread.h"
#include "bounded_buffer.h"
#include "semaphore.h"

/*
 * The bounded buffer data.
 */
struct _bounded_buffer {
    /* The maximum number of elements in the buffer */
    int length;

    /* The index of the first element in the buffer */
    int first;

    /* The number of elements currently stored in the buffer */
    int count;

    /* The values in the buffer */
    BufferElem *buffer;

    /* Keeps track of whether the buffer has space or not. */
    Semaphore *space_avail;

    /* 
     * Keeps track of whether there are elements in the buffer to take or not.
     */
    Semaphore *elem_take;

    /* A mutex that ensures only one thread is modifying the buffer. */
    Semaphore *mutex;
};


#define EMPTY -1

/*
 * Allocate a new bounded buffer.
 */
BoundedBuffer *new_bounded_buffer(int length) {
    BoundedBuffer *bufp;
    BufferElem *buffer;
    int i;
   
    /* Allocate the buffer */
    buffer = (BufferElem *) malloc(length * sizeof(BufferElem));
    bufp = (BoundedBuffer *) malloc(sizeof(BoundedBuffer));
    if (buffer == 0 || bufp == 0) {
        fprintf(stderr, "new_bounded_buffer: out of memory\n");
        exit(1);
    }

    /* Initialize */
    memset(bufp, 0, sizeof(BoundedBuffer));

    for (i = 0; i != length; i++) {
        buffer[i].id = EMPTY;
        buffer[i].arg = EMPTY;
        buffer[i].val = EMPTY;
    }

    bufp->length = length;
    bufp->buffer = buffer;
    /* Initialize semaphores */
    bufp->space_avail = new_semaphore(length);
    bufp->elem_take = new_semaphore(0);
    bufp->mutex = new_semaphore(1);

    return bufp;
}

/*
 * Add an integer to the buffer.  Yield control to another
 * thread if the buffer is full.
 */
void bounded_buffer_add(BoundedBuffer *bufp, const BufferElem *elem) {
    /* Wait until the buffer has space. */
    semaphore_wait(bufp->space_avail);
    /* Wait until there are no other threads modifying the buffer. */
    semaphore_wait(bufp->mutex);
    
    /* Now the buffer has space.  Copy the element data over. */
    int idx = (bufp->first + bufp->count) % bufp->length;
    bufp->buffer[idx].id  = elem->id;
    bufp->buffer[idx].arg = elem->arg;
    bufp->buffer[idx].val = elem->val;

    bufp->count = bufp->count + 1;

    /* 
     * Signals that the current thread is finished modifying the buffer,
     * so there are no threads modifying it at this point.
     */
    semaphore_signal(bufp->mutex);
    /* Signals that an element has been added to the buffer.*/
    semaphore_signal(bufp->elem_take);
}

/*
 * Get an integer from the buffer.  Yield control to another
 * thread if the buffer is empty.
 */
void bounded_buffer_take(BoundedBuffer *bufp, BufferElem *elem) {
    /* Wait until the buffer has a value to retrieve */
    semaphore_wait(bufp->elem_take);
    /* Wait until there are no other threads modifying the buffer. */
    semaphore_wait(bufp->mutex);
    
    /* Copy the element from the buffer, and clear the record. */
    elem->id  = bufp->buffer[bufp->first].id;
    elem->arg = bufp->buffer[bufp->first].arg;
    elem->val = bufp->buffer[bufp->first].val;

    bufp->buffer[bufp->first].id  = -1;
    bufp->buffer[bufp->first].arg = -1;
    bufp->buffer[bufp->first].val = -1;

    bufp->count = bufp->count - 1;
    bufp->first = (bufp->first + 1) % bufp->length;

    /* 
     * Signals that the current thread is finished modifying the buffer,
     * so there are no threads modifying it at this point.
     */
    semaphore_signal(bufp->mutex);
    /* Signals that an element has been removed from the buffer.*/
    semaphore_signal(bufp->space_avail);
}

