#include "list.h"

#include <debug.h>
#include <stdlib.h>

queue_t *queue_new(void) {
    queue_t *q = malloc(sizeof(queue_t));
    if (q == NULL) {
        return q;
    }

    // invariant: if head is NULL then so is tail, and vice versa
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

void queue_free(queue_t *q, void (*freer)(void *)) {
    if (q == NULL) return;

    list_ele_t *prev_elem;
    for (list_ele_t *curr_elem = q->head; curr_elem != NULL;) {
        if (freer != NULL) freer(curr_elem->value);

        prev_elem = curr_elem;
        curr_elem = curr_elem->next;
        free(prev_elem);
    }
    free(q);
}

size_t queue_size(queue_t *q) {
    if (q == NULL) return 0;
    return q->size;
}

// to check if insert is successful, check that size changed
void queue_insert_head(queue_t *q, void *v) {
    if (q == NULL) return;

    list_ele_t *new_elem = malloc(sizeof(list_ele_t));
    if (new_elem == NULL) {
        return;
    }

    // attach v to new_elem
    new_elem->value = v;

    // insert new_elem into q
    switch (q->size) {
        case 0:
            // q is empty
            new_elem->next = new_elem->prev = NULL;
            q->head = q->tail = new_elem;
            break;
        case 1:
        default:

            // doubly link new_elem <=> old_head
            new_elem->next = q->head;
            new_elem->next->prev = new_elem;

            // make new_elem new head
            new_elem->prev = NULL;
            q->head = new_elem;

            // don't need to change tail since there was at least one elem
            break;
    }
    q->size++;
}

void queue_insert_tail(queue_t *q, void *v) {
    if (q == NULL) return;

    list_ele_t *new_elem = malloc(sizeof(list_ele_t));
    if (new_elem == NULL) {
        return;
    }

    // attach v to new_elem
    new_elem->value = v;

    // insert new_elem into q

    switch (q->size) {
        case 0:
            // empty q
            new_elem->prev = new_elem->next = NULL;
            q->head = q->tail = new_elem;
            break;

        default:
            // doubly link old tail to new_elem
            new_elem->prev = q->tail;
            new_elem->prev->next = new_elem;

            // make new_elem the tail
            new_elem->next = NULL;
            q->tail = new_elem;

            // don't need to change head since there's at least one other elem
            break;
    }

    q->size++;
}

void *queue_remove_head(queue_t *q) {
    if (q == NULL) return NULL;

    switch (q->size) {
        case 0:
            return NULL;
            break;

        case 1: {  // q will be empty after this

            list_ele_t *to_remove = q->head;
            void *to_ret = to_remove->value;

            q->head = q->tail = NULL;
            free(to_remove);
            q->size--;
            return to_ret;
            break;
        }

        default: {  // will be non-empty after removal

            list_ele_t *to_remove = q->head;
            void *to_ret = to_remove->value;

            // promote second elem to head
            to_remove->next->prev = NULL;
            q->head = to_remove->next;

            free(to_remove);
            q->size--;
            return to_ret;
            break;
        }
    }
}

void *queue_remove_tail(queue_t *q) {
    if (q == NULL) return NULL;

    switch (q->size) {
        case 0:
        case 1:
            return queue_remove_head(q);

        default: {
            // will be non-empty after removal

            list_ele_t *to_remove = q->tail;
            void *to_ret = to_remove->value;

            // promote 2nd to last elem to tail
            to_remove->prev->next = NULL;
            q->tail = to_remove->prev;

            free(to_remove);
            q->size--;
            return to_ret;
            break;
        }
    }
}

// breaks everything if `elem` isn't in `q`
void remove_and_delete(queue_t *q, list_ele_t *elem, void (*freer)(void *)) {
    if (q == NULL) return;

    if (elem == q->head) {
        queue_remove_head(q);
        return;
    }

    if (elem == q->tail) {
        queue_remove_tail(q);
        return;
    }

    // elem is neither head nor tail, so both its prev & next are non-NULL
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    if (freer != NULL) freer(elem->value);
    free(elem);
    q->size--;
}