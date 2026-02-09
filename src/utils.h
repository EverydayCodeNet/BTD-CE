#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <debug.h>
#include "structs.h"

void* safe_malloc(size_t size, int line);

int16_t distance(position_t p1, position_t p2);

#ifdef __cplusplus
}
#endif

#endif