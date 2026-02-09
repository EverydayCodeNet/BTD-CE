#include "path.h"

#include <structs.h>

#include "utils.h"

position_t default_path[] = {{0, 113},   {64, 113},  {64, 54},  {140, 54},
                             {140, 174}, {36, 174},  {36, 216}, {288, 216},
                             {288, 149}, {206, 149}, {206, 94}, {290, 94},
                             {290, 28},  {180, 28},  {180, 0}};

int pathLength(path_t* path) {
    int len = 0;
    // loop over line segments
    for (size_t i = 0; i < (path->num_points - 1); i++) {
        // i => start of line segment, i+1 => end of line segment
        len += distance((path->points)[i], (path->points)[i + 1]);
    }
    return len;
}

/**
 * Build a new path based on an array of points, and a width
 *
 * The points aren't copied, and shouldn't be freed until the path is freed
 */
path_t* newPath(position_t* points, size_t num_points, int16_t width) {
    if (points == NULL || num_points == 0) {
        // use default path
        points = (position_t*)&default_path;
        num_points = sizeof(default_path) / sizeof(position_t);
    }
    path_t* path = safe_malloc(sizeof(path_t), __LINE__);
    path->width = width;
    path->num_points = num_points;
    path->points = points;
    path->length = pathLength(path);

    // get rectangles from points
    size_t num_rectangles = num_points - 1;
    path->rectangles =
        safe_malloc(sizeof(rectangle_t) * num_rectangles, __LINE__);
    for (size_t i = 0; i < num_rectangles; i++)
        initRectFromLineSeg(&((path->rectangles)[i]), path->points[i],
                            path->points[i + 1], width);

#ifdef DEBUG
    dbg_printf("Rectangles on path:\n");
    for (size_t i = 0; i < num_rectangles; i++) {
        rectangle_t r = (path->rectangles)[i];
        dbg_printf(
            "upper_left: (%d, %d) \t upper_right: (%d, %d)\n"
            "lower_left: (%d, %d) \t lower_right: (%d, %d)\n\n",
            r.upper_left.x, r.upper_left.y, r.upper_right.x, r.upper_right.y,
            r.lower_left.x, r.lower_left.y, r.lower_right.x, r.lower_right.y);
    }
    dbg_printf("... Done! (rect on path)\n");
#endif

    dbg_printf("The path has total length %d\n", path->length);
    return path;
}

/**
 * Frees the passed path
 *
 * Doesn't free the points which make up the path, since those could be an arr
 */
void freePath(path_t* path) {
    free(path->rectangles);
    free(path);
}

void drawGamePath(game_t* game) {
    path_t* path = game->path;
    dbg_printf("Drawing path...\n");
    gfx_SetColor(159);
    size_t numSegments = path->num_points - 1;
    position_t segStart;
    position_t segEnd;
    for (size_t i = 0; i < numSegments; i++) {
        segStart = path->points[i];
        segEnd = path->points[i + 1];

        // draw circle at start of line segment
        gfx_FillCircle(segStart.x, segStart.y, path->width / 2);

        // draw segment
        draw_rectangle(&(path->rectangles[i]));
    }

    // draw end circle
    if (numSegments > 0) gfx_FillCircle(segEnd.x, segEnd.y, path->width / 2);
    dbg_printf("... Done drawing path\n");
}

void initRectFromLineSeg(rectangle_t* rect, position_t p1, position_t p2,
                         int16_t width) {
    position_t* upper_left = &(rect->upper_left);
    position_t* upper_right = &(rect->upper_right);
    position_t* lower_left = &(rect->lower_left);
    position_t* lower_right = &(rect->lower_right);

    position_t tmp;

    // horiz line seg
    if (p1.x == p2.x) {
        rect->kind = HORZ;
        // make p1 upper, p2 lower
        if (p1.y > p2.y) {
            tmp.x = p1.x;
            tmp.y = p1.y;

            p1.x = p2.x;
            p1.y = p2.y;

            p2.x = tmp.x;
            p2.y = tmp.y;

            if (!(p1.y < p2.y))
                dbg_printf("ERROR: p1, p2 not swapped successfully! %d\n",
                           __LINE__);
        }
        // p1.y < p2.y => p1 is closer to top of canvas

        upper_left->x = p1.x - (width / 2);
        upper_left->y = p1.y;

        upper_right->x = p1.x + (width / 2);
        upper_right->y = p1.y;

        lower_left->x = p2.x - (width / 2);
        lower_left->y = p2.y;

        lower_right->x = p2.x + (width / 2);
        lower_right->y = p2.y;
        return;
    }
    // vert line seg
    if (p1.y == p2.y) {
        rect->kind = VERT;
        // make p1 left, p2 right
        if (p1.x > p2.x) {
            tmp.x = p1.x;
            tmp.y = p1.y;

            p1.x = p2.x;
            p1.y = p2.y;

            p2.x = tmp.x;
            p2.y = tmp.y;

            if (!(p1.x < p2.x))
                dbg_printf("ERROR: p1, p2 not swapped successfully! %d\n",
                           __LINE__);
        }
        // p1.x < p2.x => p1 is left

        upper_left->y = p1.y - (width / 2);
        upper_left->x = p1.x;

        lower_left->y = p1.y + (width / 2);
        lower_left->x = p1.x;

        upper_right->y = p2.y - (width / 2);
        upper_right->x = p2.x;

        lower_right->y = p2.y + (width / 2);
        lower_right->x = p2.x;
        return;
    }

    rect->kind = DIAG;
    // make p1 left, p2 right
    if (p1.x > p2.x) {
        tmp.x = p1.x;
        tmp.y = p1.y;

        p1.x = p2.x;
        p1.y = p2.y;

        p2.x = tmp.x;
        p2.y = tmp.y;
    }
    // p1.x < p2.x => p1 is more left

    // written by chat gpt, not yet checked
    // Calculate slope of the side
    double m = (double)(p1.x - p2.x) / (p2.y - p1.y);

    // Calculate displacements along axes
    double dx = (width / sqrt(1 + (m * m))) * 0.5;
    double dy = m * dx;

    // Rounding the results to get integer coordinates
    if (dy > 0) {
        // adding dy makes it the lower point
        // subing dy makes it the higher point

        // left point smaller y
        upper_left->x = round(p1.x - dx);
        upper_left->y = round(p1.y - dy);

        // left point larger y
        lower_left->x = round(p1.x + dx);
        lower_left->y = round(p1.y + dy);

        // right point smaller y
        upper_right->x = round(p2.x - dx);
        upper_right->y = round(p2.y - dy);

        // right point larger y
        lower_right->x = round(p2.x + dx);
        lower_right->y = round(p2.y + dy);

    } else {  // dy < 0
        // adding dy makes it the higher point
        // subing dy makes it the lower point

        // left point larger y
        lower_left->x = round(p1.x - dx);
        lower_left->y = round(p1.y - dy);

        // left point smaller y
        upper_left->x = round(p1.x + dx);
        upper_left->y = round(p1.y + dy);

        // right point smaller y
        upper_right->x = round(p2.x + dx);
        upper_right->y = round(p2.y + dy);

        // right point larger y
        lower_right->x = round(p2.x - dx);
        lower_right->y = round(p2.y - dy);
    }
}

const char* RECT_KINDS[] = {"HORZ", "VERT", "DIAG"};

void draw_rectangle(rectangle_t* rect) {
    switch (rect->kind) {
        case HORZ:
        case VERT:
            // dbg_printf(
            //     "Filling %s rectangle at at (%d, %d) with width %d and height
            //     %d\n", RECT_KINDS[rect->kind], rect->upper_left.x,
            //     rect->upper_left.y,
            //     rect->upper_right.x -  rect->upper_left.x,
            //     rect->lower_left.y - rect->upper_left.y);

            gfx_FillRectangle(rect->upper_left.x, rect->upper_left.y,
                              rect->upper_right.x - rect->upper_left.x,
                              rect->lower_left.y - rect->upper_left.y);
            break;

        case DIAG:
            // cut rectangle down center & draw two triangles
            gfx_FillTriangle(rect->upper_left.x, rect->upper_left.y,
                             rect->lower_left.x, rect->lower_left.y,
                             rect->upper_right.x, rect->upper_right.y);

            gfx_FillTriangle(rect->upper_right.x, rect->upper_right.y,
                             rect->lower_right.x, rect->lower_right.y,
                             rect->lower_left.x, rect->lower_left.y);
            break;

        default:
            dbg_printf("ERROR: bad rectangle kind\n");
            break;
    }
}