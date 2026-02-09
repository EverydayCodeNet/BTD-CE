#include <stdint.h>
#include <stdlib.h>
#include <debug.h>
#include <math.h>
#include "structs.h"


void* safe_malloc(size_t size, int line){
    (void)line;
    void* res = malloc(size);

    if (size == 0){
        dbg_printf("WARN: size=0 call to safe_malloc on line %d\n", line);
        return res;
    }

    if (res == NULL){
        dbg_printf("ERROR: call to safe_malloc FAILED on line %d\n", line);
    }
    return res;
}

int16_t distance(position_t p1, position_t p2) {
    if (p1.x == p2.x) {
        // vert line
        return (int16_t)abs(p1.y - p2.y);
    } else if (p1.y == p2.y) {
        // horiz line
        return (int16_t)abs(p1.x - p2.x);
    } else {
        // neither
        int dx = p2.x - p1.x;
        int dy = p2.y - p1.y;
        double dist = sqrt(dx * dx + dy * dy);
        return (int16_t)round(dist);
    }
}