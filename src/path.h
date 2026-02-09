#ifndef PATH_H
#define PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>

#include "structs.h"

#define DEFAULT_PATH_WIDTH 20  // path width in pixels

int pathLength(path_t* path);

path_t* newPath(position_t* points, size_t num_points, int16_t width);

void freePath(path_t* path);

void drawGamePath(game_t* game);

void initRectFromLineSeg(rectangle_t* rect, position_t p1, position_t p2,
                         int16_t width);

void draw_rectangle(rectangle_t* rect);

#ifdef __cplusplus
}
#endif

#endif