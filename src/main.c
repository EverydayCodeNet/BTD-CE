/*
 *--------------------------------------
 * Program Name: BTDCE
 * Author: Everyday Code & EKB
 * License:
 * Description: "BTD remake for the TI-84 Plus CE."
 *--------------------------------------
 */

// standard libraries
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// graphing calc libraries
#include <debug.h>  // for dbg_printf()
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>

// converted graphics files
#include "gfx/gfx.h"        // palette only
#include "gfx/btdtw1_gfx.h" // tower sprites 1 (appvar)
#include "gfx/btdtw2_gfx.h" // tower sprites 2 (appvar)
#include "gfx/btdbln_gfx.h" // bloon sprites (appvar)
#include "gfx/btdui_gfx.h"  // ui sprites (appvar)

// our code
#include "list.h"
#include "path.h"
#include "spacial_partition.h"
#include "structs.h"
#include "utils.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

position_t predict_bloon_position(bloon_t* bloon, path_t* path) {
    // Simple position prediction based on bloon's current segment and speed
    position_t predicted_pos = bloon->position;
    
    // If bloon is moving to next point in path
    if (bloon->segment < path->num_points - 1) {
        position_t target = path->points[bloon->segment + 1];
        // Move prediction point ahead based on speed
        double dx = target.x - bloon->position.x;
        double dy = target.y - bloon->position.y;
        double dist = sqrt(dx * dx + dy * dy);
        
        if (dist > 0) {
            double scale = bloon->speed / dist;
            predicted_pos.x += (int)(dx * scale * 3); // Look ahead factor of 3
            predicted_pos.y += (int)(dy * scale * 3);
        }
    }
    
    return predicted_pos;
}

bloon_t* find_target_bloon(game_t* game, tower_t* tower) {
    bloon_t* target = NULL;
    int closest_dist_sq = tower->radius * tower->radius;

    // Check each spatial partition box for bloons
    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            bloon_t* bloon = (bloon_t*)(curr_elem->value);

            // Calculate squared distance to this bloon (avoids sqrt)
            int dx = bloon->position.x - tower->position.x;
            int dy = bloon->position.y - tower->position.y;
            int dist_sq = dx * dx + dy * dy;

            // Check if this bloon is in range and closer than current target
            if (dist_sq <= closest_dist_sq) {
                target = bloon;
                closest_dist_sq = dist_sq;
            }

            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }

    return target;
}

double calculate_angle(position_t start, position_t target) {
    double dx = target.x - start.x;
    double dy = target.y - start.y;
    // Returns angle in degrees, adjusted for TI-84 CE coordinate system
    return fmod((atan2(dy, dx) * 180.0 / M_PI) + 360.0, 360.0);
}

tower_t* initTower(game_t* game) {
    // in the future, add type of monkey being initialized
    tower_t* tower = safe_malloc(sizeof(tower_t), __LINE__);

    tower->cooldown = 10;
    tower->position = game->cursor;
    tower->target = FIRST;
    tower->radius = 50;
    tower->pierce = 1;
    return tower;
}

bloon_t* initBloon(game_t* game) {
    bloon_t* bloon = safe_malloc(sizeof(bloon_t), __LINE__);

    bloon->position.x = 0 - red_base->width;
    bloon->position.y = ((game->path)->points)[0].y;
    bloon->speed = 3;
    bloon->segment = 0;
    bloon->progress_along_segment = 0;
    bloon->sprite = red_base;
    return bloon;
}

projectile_t* initProjectile(game_t* game, tower_t* tower) {
    projectile_t* projectile = safe_malloc(sizeof(projectile_t), __LINE__);

    // origin of the projectile (starting tower)
    projectile->position.x = tower->position.x;
    projectile->position.y = tower->position.y;

    // TODO: case on tower to determine projectile details
    projectile->speed = 3;
    projectile->pierce = tower->pierce;
    projectile->sprite = big_dart;
    
    bloon_t* target = find_target_bloon(game, tower);
    if (target) {
        position_t predicted_pos = predict_bloon_position(target, game->path);
        projectile->angle = calculate_angle(projectile->position, predicted_pos);
    } else {
        // No target in range, don't fire
        free(projectile);
        return NULL;
    }

    return projectile;
}

void handleKeys(game_t* game) {
    dbg_printf("handleKeys!\n");
    kb_Scan();

    if (kb_Data[7] & kb_Up) {
        game->cursor.y--;
    } else if (kb_Data[7] & kb_Down) {
        game->cursor.y++;
    } else if (kb_Data[7] & kb_Left) {
        game->cursor.x--;
    } else if (kb_Data[7] & kb_Right) {
        game->cursor.x++;
    }

    if (kb_Data[6] & kb_Enter) {
        int base_cost = 100;

        if ((game->cursor_type == SELECTED) &&
            (game->coins >= base_cost)) {  // player wants to place a tower &
                                           // has sufficient funds

            // charge the player
            game->coins -= base_cost;

            // TODO: check (in_bounds() && not_on_path())

            // make a new tower & insert into game->towers
            // TODO: check the tower type
            tower_t* tower = initTower(game);
            queue_insert_head(game->towers, (void*)tower);
            game->cursor_type = NONE;
        }
    }

    if (kb_Data[6] & kb_Add) {
        game->cursor_type = SELECTED;
    }

    if (kb_Data[1] & kb_Mode) game->SHOW_STATS = !game->SHOW_STATS;

    if (kb_Data[6] & kb_Clear) game->exit = true;
}

void drawCursor(game_t* game) {
    dbg_printf("drawCursor!\n");
    int x = game->cursor.x;
    int y = game->cursor.y;
    gfx_TransparentSprite(dart1, x, y);
    gfx_SetColor(255);
    // replace with selected tower radius
    // center it based on the tower width/height
    gfx_Circle(x + (dart1->width / 2), y + (dart1->height / 2), 50);
}

void drawMap(game_t* game) {
    // draw the background
    gfx_SetColor(158);
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // draw the path
    drawGamePath(game);
}

void drawStats(game_t* game) {
    const size_t PADDING = 3;
    // size_t chars_drawn = 0;
    size_t x_start = 10;
    size_t x_off = x_start;
    size_t y_off = 10;
    const size_t number_width = gfx_GetStringWidth("xxx");

    const char* hearts_msg = "Hearts: ";
    gfx_PrintStringXY(hearts_msg, x_off, y_off);
    x_off += gfx_GetStringWidth(hearts_msg);
    gfx_PrintInt(game->hearts, 1);
    x_off += number_width + PADDING;

    const char* round_msg = "Round: ";
    gfx_PrintStringXY(round_msg, x_off, y_off);
    x_off += gfx_GetStringWidth(round_msg);
    gfx_PrintInt(game->round, 1);
    x_off += number_width + PADDING;

    const char* coinsMsg = "Coins: ";
    gfx_PrintStringXY(coinsMsg, x_off, y_off);
    x_off += gfx_GetStringWidth(coinsMsg);
    gfx_PrintInt(game->coins, 1);
    x_off += number_width + PADDING;

    // make a new line
    const uint8_t FONT_HEIGHT = 8;
    y_off += FONT_HEIGHT * 2;
    x_off = x_start;

    const char* bloonsMSg = "Bloons: ";
    gfx_PrintStringXY(bloonsMSg, x_off, y_off * 3);
    x_off += gfx_GetStringWidth(bloonsMSg);

    size_t num_bloons = sp_total_size(game->bloons);
    dbg_printf("There are %zu bloons on screen\n", num_bloons);
    gfx_PrintInt((int)num_bloons, 1);
}

void drawTowers(game_t* game) {
    list_ele_t* curr_elem = game->towers->head;
    while (curr_elem != NULL) {
        tower_t* tower = (tower_t*)(curr_elem->value);
        gfx_TransparentSprite(dart1, tower->position.x, tower->position.y);
        curr_elem = curr_elem->next;
    }
}

void drawBloons(game_t* game) {
    dbg_printf(
        "Drawing bloons with total bloons %zu in %zu different spatial boxes\n",
        sp_total_size(game->bloons), queue_size(game->bloons->inited_boxes));

    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            bloon_t* bloon = (bloon_t*)(curr_elem->value);
            gfx_TransparentSprite(red_base, bloon->position.x,
                                  bloon->position.y - (red_base->height / 2));

            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }
}

void drawProjectiles(game_t* game) {
    list_ele_t* curr_box = game->projectiles->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            projectile_t* projectile = (projectile_t*)(curr_elem->value);
            
            // Convert to 256-position format with proper orientation
            // Adjust by 64 (quarter turn) to match sprite's base orientation
            uint8_t rot_angle = (uint8_t)(((projectile->angle + 90) * 256) / 360);
            
            gfx_RotatedTransparentSprite_NoClip(
                projectile->sprite, 
                projectile->position.x - (projectile->sprite->width / 2),
                projectile->position.y - (projectile->sprite->height / 2),
                rot_angle
            );
            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }
}



void drawExitScreen(game_t* game) {
    (void)(game);
    // draw the amount of bloons that got through
    // count bloons

    // draw button
}

const int NUM_ROUNDS = 50;
round_t* initRounds(void) {
    round_t* rounds = safe_malloc(sizeof(round_t) * NUM_ROUNDS, __LINE__);

    size_t total_num_bloons = 0;
    (void)total_num_bloons;

    const int ROUND_ONE_BLOONS = 3;

    for (int i = 0; i < NUM_ROUNDS; i++) {
        rounds[i].num_bloons = 0;
        if (i == 0) {
            rounds[i].max_bloons = ROUND_ONE_BLOONS;
        } else {
            rounds[i].max_bloons =
                50;  //(double) (1.5 * rounds[i - 1].max_bloons);
        }

        rounds[i].delay = 10;
        rounds[i].tick = 0;
        rounds[i].bloons =
            safe_malloc(sizeof(bloon_t) * rounds[i].max_bloons, __LINE__);

        if (rounds[i].bloons == NULL) {
            dbg_printf("MALLOCED %zu BALOONS FUCK\n", total_num_bloons);
        }

        total_num_bloons += rounds[i].max_bloons;

        for (int j = 0; j < rounds[i].max_bloons; j++) {
            rounds[i].bloons[j].sprite = red_base;
        }
    }

    dbg_printf("MALLOCED %zu BALOONS OK\n", total_num_bloons);

    return rounds;
}

void freeRounds(round_t* rounds) {
    for (int i = 0; i < NUM_ROUNDS; i++) free(rounds[i].bloons);
    free(rounds);
}

/// @brief updates `current_position` to move in the direction of `target`,
/// until `max_dist` is reached or `target` is reached
/// @return the distance left to move (0 if no movement left)
double moveTowards(position_t* current_position, const position_t target,
                   double max_dist) {
    // move vertically
    if (current_position->x == target.x) {
        int dy = target.y - current_position->y;
        if (abs(dy) <= max_dist) {  // can move to target
            current_position->y = target.y;
            return max_dist - abs(dy);
        }

        // can move towards target but not reach it
        current_position->y += (int)(dy < 0 ? -max_dist : max_dist);
        return 0.0;
    }

    // move horizontally
    if (current_position->y == target.y) {
        int dx = target.x - current_position->x;
        if (abs(dx) <= max_dist) {  // can move to target
            current_position->x = target.x;
            return max_dist - abs(dx);
        }

        // can move towards target but not reach it
        current_position->x += (int)(dx < 0 ? -max_dist : max_dist);
        return 0.0;
    }

    // move on a diagonal (slow)
    // Calculate direction vector to next point
    int dx = target.x - current_position->x;
    int dy = target.y - current_position->y;
    double dist = sqrt(dx * dx + dy * dy);  // distance to point

    if (dist <= max_dist) {
        // can reach the point
        current_position->x = target.x;
        current_position->y = target.y;
        return max_dist - dist;
    }

    // can't quite reach the point

    // Normalize the direction vector and move the bloon along it
    current_position->x += (int)((max_dist / dist) * dx);
    current_position->y += (int)((max_dist / dist) * dy);
    return 0.0;
}

/// @brief move bloon based on its current location and speed
/// @return segment bloon is on after moving (equals bloon->segment)
int moveBloon(game_t* game, bloon_t* bloon) {
    int numSegments = game->path->num_points - 1;

    double movement_left = (double)bloon->speed;
    while (movement_left > 0.0) {
        int currSeg = bloon->segment;
        if (currSeg >= numSegments) {
            // on non-existent segment
            return currSeg;
        }

        // move towards end-point of current segment
        position_t target = (game->path->points)[currSeg + 1];
        movement_left = moveTowards(&(bloon->position), target, movement_left);
        if (bloon->position.x == target.x && bloon->position.y == target.y) {
            bloon->segment++;
        }
    }

    return bloon->segment;
}

game_t* newGame(position_t* points, size_t num_points) {
    game_t* game = safe_malloc(sizeof(game_t), __LINE__);
    game->path = newPath(points, num_points, DEFAULT_PATH_WIDTH);
    // choose hearts based on game difficulty, to be added
    game->hearts = 100;
    game->coins = 1000;

    game->towers = queue_new();
    game->bloons = new_partitioned_list(
        SCREEN_WIDTH, SCREEN_HEIGHT,
        20);  // break screen into 20 x 20 boxes for spatial partitioning
    game->projectiles = new_partitioned_list(SCREEN_WIDTH, SCREEN_HEIGHT, 20);

    dbg_printf("created mutli_l with inited length %zu and %zu\n",
               game->bloons->inited_boxes->size,
               game->projectiles->inited_boxes->size);

    game->exit = false;

    position_t start_cursor = {160, 120};
    game->cursor = start_cursor;
    game->cursor_type = NONE;

    game->round = 1;
    dbg_printf("Called init rounds!\n");

    game->rounds = initRounds();

    game->AUTOPLAY = false;
    game->SHOW_STATS = false;

    dbg_printf("Returned new game_t\n");

    return game;
}

bool boxesCollide(position_t p1, int width1, int height1, position_t p2,
                  int width2, int height2) {
    return (p1.x < p2.x + width2 && p1.x + width1 > p2.x &&
            p1.y < p2.y + height2 && p1.y + height1 > p2.y);
}

void checkProjPositions(game_t* game) {
    list_ele_t* curr_box = game->projectiles->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            projectile_t* projectile = (projectile_t*)(curr_elem->value);

            if (sp_soft_get_list(game->projectiles, projectile->position) !=
                ((queue_t*)(curr_box->value))) {
                dbg_printf(
                    "Proj with position x: %d, y: %d not in correct list!\n",
                    projectile->position.x, projectile->position.y);
            }

            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }
}

void checkBloonPositions(game_t* game) {
    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            bloon_t* bloon = (bloon_t*)(curr_elem->value);

            if (sp_soft_get_list(game->bloons, bloon->position) !=
                ((queue_t*)(curr_box->value))) {
                dbg_printf(
                    "Bloon with position x: %d, y: %d not in correct list!\n",
                    bloon->position.x, bloon->position.y);
            }

            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }
}

void checkBloonProjCollissions(game_t* game) {
    list_ele_t* next_bloon_elem = NULL;
    list_ele_t* next_proj_elem = NULL;

    list_ele_t* curr_bloon_box = game->bloons->inited_boxes->head;
    while (curr_bloon_box != NULL) {
        // check all bloons in each bloon box

        list_ele_t* curr_bloon_elem = ((queue_t*)(curr_bloon_box->value))->head;
        if (curr_bloon_elem == NULL) {
            curr_bloon_box = curr_bloon_box->next;
            continue;
        }  // no bloons in this box

        // all projectiles in same spatial box
        queue_t* same_box_projs = sp_soft_get_list(
            game->projectiles, ((bloon_t*)(curr_bloon_elem->value))->position);
        if (same_box_projs == NULL) {
            curr_bloon_box = curr_bloon_box->next;
            continue;
        }  // no projectiles in same box

        while (curr_bloon_elem != NULL) {
            next_bloon_elem = curr_bloon_elem->next;

            bloon_t* tmp_bloon = (bloon_t*)(curr_bloon_elem->value);
            int bloon_width = tmp_bloon->sprite->width;
            int bloon_height = tmp_bloon->sprite->height;

            // check all remaining projectiles in the projectile box
            list_ele_t* curr_proj_elem = same_box_projs->head;
            while (curr_proj_elem != NULL) {
                next_proj_elem = curr_proj_elem->next;

                projectile_t* tmp_projectile =
                    (projectile_t*)curr_proj_elem->value;

                int width = tmp_projectile->sprite->width;
                int height = tmp_projectile->sprite->height;

                // check for collision (axis aligned bounding box)
                if (boxesCollide(tmp_bloon->position, bloon_width, bloon_height,
                                 tmp_projectile->position, width, height)) {
                    // TODO: draw the explosion/pop

                    // remove bloon
                    sp_remove(game->bloons, tmp_bloon->position,
                              curr_bloon_elem, free);
                    // remove_and_delete(game->bloons, curr_bloon_elem,
                    // free);

                    // remove projectile
                    sp_remove(game->projectiles, tmp_projectile->position,
                              curr_proj_elem, free);

                    break;  // can't pop same bloon twice.
                }
                curr_proj_elem = next_proj_elem;
            }

            curr_bloon_elem = next_bloon_elem;
        }

        // while (curr_bloon_elem != NULL) {
        //     next_bloon_elem = curr_bloon_elem->next;

        //     bloon_t* tmp_bloon = (bloon_t*)(curr_bloon_elem->value);
        //     int bloon_width = tmp_bloon->sprite->width;
        //     int bloon_height = tmp_bloon->sprite->height;
        //     bool bloon_popped = false;

        //     list_ele_t* curr_proj_box =
        //     game->projectiles->inited_boxes->head; while (curr_proj_box
        //     != NULL) {
        //         if (bloon_popped) break;
        //         // loop over all projectiles
        //         list_ele_t* curr_proj_elem =
        //             ((queue_t*)(curr_proj_box->value))->head;
        //         while (curr_proj_elem != NULL) {
        //             next_proj_elem = curr_proj_elem->next;

        //             projectile_t* tmp_projectile =
        //                 (projectile_t*)curr_proj_elem->value;

        //             int width = tmp_projectile->sprite->width;
        //             int height = tmp_projectile->sprite->height;

        //             // check for collision (axis aligned bounding box)
        //             if (boxesCollide(tmp_bloon->position, bloon_width,
        //                              bloon_height,
        //                              tmp_projectile->position, width,
        //                              height)) {
        //                 // TODO: draw the explosion/pop
        //                 dbg_printf(
        //                     "bloon width: %d; bloon height: %d;
        //                     projectile " "width: "
        //                     "%d; "
        //                     "projectile height: %d\n",
        //                     bloon_width, bloon_height, width, height);

        //                 dbg_printf(
        //                     "bloon x: %d; bloon y: %d; proj x: %d; proj
        //                     y: "
        //                     "%d\n",
        //                     tmp_bloon->position.x, tmp_bloon->position.y,
        //                     tmp_projectile->position.x,
        //                     tmp_projectile->position.y);

        //                 // remove bloon
        //                 sp_remove(game->bloons, tmp_bloon->position,
        //                           curr_bloon_elem, free);
        //                 // remove_and_delete(game->bloons,
        //                 curr_bloon_elem,
        //                 // free);

        //                 // remove projectile
        //                 sp_remove(game->projectiles,
        //                 tmp_projectile->position,
        //                           curr_proj_elem, free);
        //                 // remove_and_delete(game->projectiles,
        //                 curr_proj_elem,
        //                 // free);
        //                 bloon_popped = true;
        //                 break;  // can't pop same bloon twice.
        //             }

        //             curr_proj_elem = next_proj_elem;
        //         }
        //         curr_proj_box = curr_proj_box->next;
        //     }
        //     curr_bloon_elem = next_bloon_elem;
        // }
        curr_bloon_box = curr_bloon_box->next;
    }
}

void updateBloons(game_t* game) {
    dbg_printf("called updateBloons!\n");

    const int BLOON_VALUE = 1;
    const int num_points = game->path->num_points;
    const int num_segments = num_points - 1;

    // update bloon's position on board
    list_ele_t* curr_bloon_box = game->bloons->inited_boxes->head;
    while (curr_bloon_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_bloon_box->value))->head;
        list_ele_t* tmp;
        while (curr_elem != NULL) {
            bloon_t* curr_bloon = (bloon_t*)(curr_elem->value);
            position_t pos_before_move = curr_bloon->position;
            int segBeforeMove = curr_bloon->segment;
            if (segBeforeMove >= num_segments ||             // off board now
                moveBloon(game, curr_bloon) >= num_segments  // off after moving
            ) {
                // delete bloon & count against health
                game->hearts -= BLOON_VALUE;
                tmp = curr_elem->next;

                sp_remove(game->bloons, pos_before_move, curr_elem, free);
                // remove_and_delete(game->bloons, curr_elem, free);  //
                // frees list_elem_t and bloon_t
                curr_elem = tmp;
                continue;
            }
            curr_elem = curr_elem->next;
        }
        curr_bloon_box = curr_bloon_box->next;
    }

    // since bloon positions changed, we need to fix the boxes
    curr_bloon_box = game->bloons->inited_boxes->head;
    while (curr_bloon_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_bloon_box->value))->head;
        list_ele_t* next_elem;
        while (curr_elem != NULL) {
            next_elem = curr_elem->next;

            bloon_t* curr_bloon = (bloon_t*)(curr_elem->value);
            sp_fix_box(game->bloons, (queue_t*)curr_bloon_box->value, curr_elem,
                       curr_bloon->position);

            curr_elem = next_elem;
        }
        curr_bloon_box = curr_bloon_box->next;
    }
}

void updateTowers(game_t* game) {
    dbg_printf("called updateTowers!\n");
    list_ele_t* curr_elem = game->towers->head;
    while (curr_elem != NULL) {
        tower_t* tower = (tower_t*)(curr_elem->value);
        if ((++tower->tick) % tower->cooldown == 0) {
            // tower can attack; it spawns a new projectile
            projectile_t* new_proj = initProjectile(game, tower);
            if (new_proj != NULL) {
                sp_insert(game->projectiles, new_proj->position, new_proj);
            }
        }
        curr_elem = curr_elem->next;
    }
}

bool offScreen(position_t p) {
    return (p.x < 0 || p.y < 0 || p.x > SCREEN_WIDTH || p.y > SCREEN_HEIGHT);
}

void updateProjectiles(game_t* game) {
    dbg_printf("called updateProjectiles!\n");
    dbg_printf("there are %zu projectiles\n", sp_total_size(game->projectiles));
    dbg_printf("game->projectiles->innited_boxes has size: %zu\n",
               game->projectiles->inited_boxes->size);
               
    list_ele_t* curr_box = game->projectiles->inited_boxes->head;
    while (curr_box != NULL) {
        dbg_printf("0..");

        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        list_ele_t* tmp;
        while (curr_elem != NULL) {
            projectile_t* proj = (projectile_t*)(curr_elem->value);
            tmp = curr_elem->next;

            // DESPAWN IF:
            // check if projectile has gone off screen
            if (offScreen(proj->position)) {
                dbg_printf("1..");
                // Remove the projectile from the linked list & free it
                sp_remove(game->projectiles, proj->position, curr_elem, free);
                dbg_printf("2..");
                curr_elem = tmp;
                continue;
            }

            // Update position based on angle and speed
            double rad_angle = (proj->angle * M_PI) / 180.0;
            proj->position.x += (int)(cos(rad_angle) * proj->speed);
            proj->position.y += (int)(sin(rad_angle) * proj->speed);
            
            // Update angle based on movement direction if needed
            // proj->angle = atan2(-dy, -dx) * 180.0 / M_PI;
            
            dbg_printf("3..");
            dbg_printf("\n");

            curr_elem = tmp;
        }
        curr_box = curr_box->next;
    }

    // since we moved projectiles, fix which box they're in
    curr_box = game->projectiles->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        list_ele_t* next_elem;
        while (curr_elem != NULL) {
            next_elem = curr_elem->next;

            projectile_t* proj = (projectile_t*)(curr_elem->value);
            sp_fix_box(game->projectiles, (queue_t*)curr_box->value, curr_elem,
                       proj->position);

            curr_elem = next_elem;
        }
        curr_box = curr_box->next;
    }
}

void handleGame(game_t* game) {
    handleKeys(game);

    if (game->hearts <= 0) game->exit = true;

    if (game->rounds[game->round].num_bloons ==
        game->rounds[game->round].max_bloons) {
        game->round++;
    }

    if (game->round == NUM_ROUNDS) game->exit = true;

    game->rounds[game->round].tick++;
    if ((game->rounds[game->round].tick % game->rounds[game->round].delay ==
         0) &&
        game->rounds[game->round].num_bloons <
            game->rounds[game->round].max_bloons) {
        bloon_t* new_bloon = initBloon(game);
        dbg_printf("Inserting bloon...");
        sp_insert(game->bloons, new_bloon->position, new_bloon);
        dbg_printf("... bloon inserted\n");
        // queue_insert_head(game->bloons, initBloon(game));
        game->rounds[game->round].num_bloons += 1;
    }

    // move all projectiles, removing any that go off screen
    updateProjectiles(game);

    // move bloons; remove any that go off screen & game health by 1
    updateBloons(game);

    // spawn new projectiles (TODO: special towers like ice monkey)
    updateTowers(game);

    checkBloonPositions(game);

    // check for collissions between projectiles and bloons
    checkBloonProjCollissions(game);
}

void exitGame(game_t* game) {
    // free bloons, towers, projectiles
    // queue_free(game->bloons, free);
    // queue_free(game->projectiles, free);
    free_partitioned_list(game->bloons, free);
    free_partitioned_list(game->projectiles, free);
    queue_free(game->towers, free);

    freePath(game->path);
    freeRounds(game->rounds);

    // save any game elements before freeing

    free(game);
}

void runGame(void) {
    game_t* game = newGame(NULL, 0);  // use default path

    // ugly code for clarity
    while (game->exit == false) {
        handleGame(game);

        drawMap(game);
        drawStats(game);
        drawTowers(game);
        drawBloons(game);
        drawProjectiles(game);
        drawCursor(game);

        gfx_BlitBuffer();
    }

    drawExitScreen(game);

    exitGame(game);
}

int main(void) {
    // Load sprite appvars (must be before gfx_Begin)
    if (BTDTW1_init() == 0 ||
        BTDTW2_init() == 0 ||
        BTDBLN_init() == 0 ||
        BTDUI_init() == 0) {
        return 1;  // appvar missing
    }

    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(1);

    gfx_SetDrawBuffer();

    runGame();

    gfx_End();

    return 0;
}