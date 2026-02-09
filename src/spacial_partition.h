#ifndef SPACIAL_H
#define SPACIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "list.h"
#include "structs.h"

/// @brief Create a new spatially partitioned list
/// @param width width of space
/// @param height height of space
/// @param box_size the length of square boxes which space is tiled with
/// @return
multi_list_t *new_partitioned_list(size_t width, size_t height,
                                   size_t box_size);

void free_partitioned_list(multi_list_t *free_me, void (*freer)(void *));

/// @brief Get the list which this position corresponds to
/// @return `NULL` if the position is out of bounds; if the position is in
/// bounds it creates an empty list
queue_t *sp_hard_get_list(multi_list_t *l, position_t p);

/// @brief Get the list which this position corresponds to
/// @return `NULL` if a list hasn't been instantiated or the position is out of
/// bounds
queue_t *sp_soft_get_list(multi_list_t *l, position_t p);

void sp_insert(multi_list_t *l, position_t p, void *data);

void sp_remove(multi_list_t *l, position_t p, list_ele_t *elem,
               void (*freer)(void *));

void sp_fix(multi_list_t *l, list_ele_t *elem, position_t old_pos,
            position_t new_pos);

void sp_fix_box(multi_list_t *l, queue_t *old_box, list_ele_t *elem,
                position_t new_pos);

size_t sp_total_size(multi_list_t *l);

#ifdef __cplusplus
}
#endif

#endif