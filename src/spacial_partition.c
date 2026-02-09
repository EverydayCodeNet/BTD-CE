#include "spacial_partition.h"

#include <debug.h>
#include <stdlib.h>

#include "list.h"
#include "structs.h"

size_t ceil_st(size_t a, size_t b) { return (a + b - 1) / b; }

static inline size_t sp_box_index(multi_list_t *l, position_t p) {
    // negative coordinates are out of bounds
    if (p.x < 0 || p.y < 0) return l->num_boxes_in_range;

    // find which box the position is in
    size_t box_col = (size_t)p.x / l->box_size;
    size_t box_row = (size_t)p.y / l->box_size;

    if (box_row < l->height && box_col < l->width) {
        // in bounds
        return (box_row * l->width) + box_col;
    }

    // out of bounds
    return l->num_boxes_in_range;
}

multi_list_t *new_partitioned_list(size_t width, size_t height,
                                   size_t box_size) {
    multi_list_t *multi_l = malloc(sizeof(multi_list_t));
    multi_l->total_size = 0UL;
    multi_l->box_size = box_size;
    multi_l->height = ceil_st(height, box_size);
    multi_l->width = ceil_st(width, box_size);

    // list of all inited boxes
    multi_l->inited_boxes = queue_new();

    // array of (lazily created) boxes for O(1) position => box lookup
    multi_l->num_boxes_in_range = (multi_l->height * multi_l->width);

    // create an additional box for out-of-range positions
    multi_l->n = multi_l->num_boxes_in_range + 1;
    multi_l->boxes = calloc(sizeof(queue_t *), multi_l->n);

    return multi_l;
}

void free_partitioned_list(multi_list_t *free_me, void (*freer)(void *)) {
    // free boxes
    queue_t *curr_q;
    for (size_t i = 0; i < free_me->n; i++) {
        if ((curr_q = (free_me->boxes)[i]) != NULL) queue_free(curr_q, freer);
    }

    // free array
    free(free_me->boxes);

    // free inited_boxes (list of boxes)
    queue_free(free_me->inited_boxes, NULL);

    free(free_me);
}

queue_t *sp_hard_get_list(multi_list_t *l, position_t p) {
    size_t box_ind = sp_box_index(l, p);

    // lazily create boxes
    queue_t *box = l->boxes[box_ind];
    if (box == NULL) {
        box = queue_new();
        l->boxes[box_ind] = box;
        queue_insert_head(l->inited_boxes, box);
    }

    return box;
}

queue_t *sp_soft_get_list(multi_list_t *l, position_t p) {
    return l->boxes[sp_box_index(l, p)];
}

// to see if insertion was successful, check if total size changed
void sp_insert(multi_list_t *l, position_t p, void *v) {
    queue_t *box = sp_hard_get_list(l, p);
    queue_insert_head(box, v);
    l->total_size++;
}

void sp_remove(multi_list_t *l, position_t p, list_ele_t *elem,
               void (*freer)(void *)) {
    // NO-OP if list doesn't exist yet
    queue_t *box = sp_soft_get_list(l, p);
    if (box == NULL) return;

    size_t old_box_size = queue_size(box);
    remove_and_delete(box, elem, freer);
    l->total_size = (l->total_size - old_box_size + queue_size(box));
}

void sp_fix(multi_list_t *l, list_ele_t *elem, position_t old_pos,
            position_t new_pos) {
    queue_t *old_box = sp_soft_get_list(l, old_pos);
    queue_t *new_box = sp_soft_get_list(l, new_pos);

    // no need to do anything, since it will be in the same box
    if (old_box != NULL && old_box == new_box) return;

    void *v = elem->value;
    sp_insert(l, new_pos, v);
    sp_remove(l, old_pos, elem, NULL);
}

void sp_fix_box(multi_list_t *l, queue_t *old_box, list_ele_t *elem,
                position_t new_pos) {
    queue_t *new_box = sp_soft_get_list(l, new_pos);

    // no need to do anything, since it will be in the same box
    if (old_box != NULL && old_box == new_box) return;

    void *v = elem->value;
    sp_insert(l, new_pos, v);

    size_t old_box_size = queue_size(old_box);
    remove_and_delete(old_box, elem, NULL);
    l->total_size = (l->total_size - old_box_size + queue_size(old_box));
}

size_t sp_total_size(multi_list_t *l) { return l->total_size; }
