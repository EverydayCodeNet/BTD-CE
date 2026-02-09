
#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>

typedef struct list_ele {
    void *value;

    struct list_ele *prev;
    struct list_ele *next;
} list_ele_t;

typedef struct {
    list_ele_t *head;
    list_ele_t *tail;
    size_t size;

} queue_t;

/// Create / Destroy

/* Create empty queue. */
queue_t *queue_new(void);

/* Free storage used by the queue; calls freer on each `void* value` in q */
void queue_free(queue_t *q, void (*freer)(void *));

/// Insert {Head / Tail}

/* insert element at head of queue. */
void queue_insert_head(queue_t *q, void *v);

/* insert element at tail of queue. */
void queue_insert_tail(queue_t *q, void *v);

/// Remove { Head, Tail, Elem }

/* Remove head from the queue */
void *queue_remove_head(queue_t *q);

/* Remove tail from the queue */
void *queue_remove_tail(queue_t *q);

/* Snip and arbitrary element which is in the q out of the q */
void remove_and_delete(queue_t *q, list_ele_t *elem, void (*freer)(void *));

/// Size

/* Return number of elements in queue. */
size_t queue_size(queue_t *q);

#ifdef __cplusplus
}
#endif

#endif