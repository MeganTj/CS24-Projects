#ifndef QUEUE_H
#define QUEUE_H

#include "virtualmem.h"

/*! A single node of a queue of pages. */  
typedef struct _queuenode {
    page_t page;
    struct _queuenode *prev;
    struct _queuenode *next;
} QueueNode;


/*!
 * A queue of threads.  This type is used to keep track of pages.
 */
typedef struct _queue {
    QueueNode *head;  /*!< The first page in the queue. */
    QueueNode *tail;  /*!< The last page in the queue. */
} Queue;


int queue_empty(Queue *queuep);
void queue_append(Queue *queuep, page_t page);
page_t queue_take(Queue *queuep);
int queue_remove(Queue *queuep, page_t page);
void queue_clear(Queue *queuep);

#endif /* QUEUE_H */

