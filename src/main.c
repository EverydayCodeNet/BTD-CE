/*
 *--------------------------------------
 * Program Name: BTDCE
 * Author: Everyday Code & EKB
 * License:
 * Description: "BTD remake for the TI-84 Plus CE."
 *--------------------------------------
 */

// standard libraries
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
#include "angle_lut.h"
#include "bloons.h"
#include "list.h"
#include "path.h"
#include "save.h"
#include "spacial_partition.h"
#include "structs.h"
#include "towers.h"
#include "utils.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define FREEZE_DURATION 30   /* frames bloon stays frozen (~0.5s) */
#define SLOW_DURATION   90   /* frames bloon stays slowed */
#define SLOW_FACTOR     2    /* speed divisor when glued */
#define KEY_DELAY       8    /* frames between menu key repeats (~150ms) */

/* Speed button position */
#define SPEED_BTN_X (SCREEN_WIDTH - 10 - 32)
#define SPEED_BTN_Y (SCREEN_HEIGHT - 10 - 32)
#define SPEED_BTN_W 32
#define SPEED_BTN_H 32

gfx_sprite_t* bloon_sprite_table[NUM_BLOON_TYPES];
gfx_sprite_t* tower_sprite_table[NUM_TOWER_TYPES];
gfx_sprite_t* tower_projectile_table[NUM_TOWER_TYPES];

/* Projectile sprites mostly face UP (192). Boomerang faces RIGHT (0). */
static const uint8_t proj_native_angle[NUM_TOWER_TYPES] = {
    192,  /* DART:      big_dart faces up */
    192,  /* TACK:      tack spike faces up */
    0,    /* SNIPER:    N/A (hitscan) */
    192,  /* BOMB:      bomb_small faces up */
    0,    /* BOOMERANG: wood_rang_right faces right */
    192,  /* NINJA:     ninja_star faces up (radial) */
    0,    /* ICE:       N/A (area) */
    0,    /* GLUE:      N/A (no sprite) */
};

/* ── Apply Upgrades ──────────────────────────────────────────────────── */

void apply_upgrades(tower_t* tower) {
    const tower_data_t* base = &TOWER_DATA[tower->type];

    /* Start from base stats */
    tower->damage = base->damage;
    tower->pierce = base->pierce;
    tower->range = base->range;
    tower->damage_type = base->damage_type;
    tower->can_see_camo = base->can_see_camo;
    tower->projectile_count = base->projectile_count;
    tower->projectile_speed = base->projectile_speed;
    tower->sprite = tower_sprite_table[tower->type];

    int atk_pct_mod = 0;  /* cumulative attack speed % modifier */

    /* Apply upgrades from both paths */
    for (int path = 0; path < 2; path++) {
        for (int level = 0; level < tower->upgrades[path]; level++) {
            const upgrade_t* upg = &TOWER_UPGRADES[tower->type][path][level];
            tower->damage += upg->delta_damage;
            tower->pierce += upg->delta_pierce;
            tower->range += upg->delta_range;
            atk_pct_mod += upg->delta_atk_pct;
            tower->projectile_count += upg->delta_proj_count;
            if (upg->grants_camo) tower->can_see_camo = 1;
            if (upg->damage_type_override) tower->damage_type = upg->damage_type_override;
        }
    }

    /* Compute effective cooldown from base attack frames + percentage modifier */
    int effective_frames = (int)base->atk_frames;
    if (atk_pct_mod != 0) {
        effective_frames = (effective_frames * (100 + atk_pct_mod)) / 100;
        if (effective_frames < 2) effective_frames = 2;  /* minimum 2 frames */
    }
    tower->cooldown = (uint16_t)effective_frames;
}

/* ── Prediction & Targeting ──────────────────────────────────────────── */

position_t predict_bloon_position(bloon_t* bloon, path_t* path) {
    position_t predicted_pos = bloon->position;

    if (bloon->segment < path->num_points - 1) {
        position_t target = path->points[bloon->segment + 1];
        int dx = target.x - bloon->position.x;
        int dy = target.y - bloon->position.y;
        int dist = abs(dx) + abs(dy);  // Manhattan distance (fast)

        if (dist > 0) {
            int speed = BLOON_DATA[bloon->type].speed_fp >> 8;
            // Look ahead factor of 3
            predicted_pos.x += (dx * speed * 3) / dist;
            predicted_pos.y += (dy * speed * 3) / dist;
        }
    }

    return predicted_pos;
}

bloon_t* find_target_bloon(game_t* game, tower_t* tower) {
    bloon_t* target = NULL;
    int best_dist_sq = (int)tower->range * (int)tower->range;

    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            bloon_t* bloon = (bloon_t*)(curr_elem->value);

            /* Skip camo bloons if tower can't see camo */
            if ((bloon->modifiers & MOD_CAMO) && !tower->can_see_camo) {
                curr_elem = curr_elem->next;
                continue;
            }

            int dx = bloon->position.x - tower->position.x;
            int dy = bloon->position.y - tower->position.y;
            int dist_sq = dx * dx + dy * dy;

            if (dist_sq <= best_dist_sq) {
                target = bloon;
                best_dist_sq = dist_sq;
            }

            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }

    return target;
}

/* ── Integer Angle Calculation ───────────────────────────────────────── */

uint8_t calculate_angle_int(position_t start, position_t target) {
    int16_t dx = target.x - start.x;
    int16_t dy = target.y - start.y;
    return iatan2(dy, dx);
}

/* ── Init Functions ──────────────────────────────────────────────────── */

tower_t* initTower(game_t* game, uint8_t type) {
    tower_t* tower = safe_malloc(sizeof(tower_t), __LINE__);
    memset(tower, 0, sizeof(tower_t));

    tower->type = type;
    tower->position = game->cursor;
    tower->target_mode = TARGET_FIRST;
    tower->upgrades[0] = 0;
    tower->upgrades[1] = 0;
    tower->total_invested = TOWER_DATA[type].cost;
    tower->pop_count = 0;
    tower->facing_angle = 0;
    tower->tick = 0;

    apply_upgrades(tower);
    return tower;
}

bloon_t* initBloon(game_t* game, uint8_t type, uint8_t modifiers) {
    bloon_t* bloon = safe_malloc(sizeof(bloon_t), __LINE__);
    bloon->type = type;
    bloon->modifiers = modifiers;
    bloon->hp = BLOON_DATA[type].hp;
    bloon->regrow_max = (modifiers & MOD_REGROW) ? type : 0;
    bloon->regrow_timer = REGROW_INTERVAL;
    bloon->segment = 0;
    bloon->progress = 0;
    bloon->position.x = 0 - 16;  // start offscreen
    bloon->position.y = game->path->points[0].y;
    bloon->freeze_timer = 0;
    bloon->slow_timer = 0;
    return bloon;
}

projectile_t* initProjectile(game_t* game, tower_t* tower, uint8_t angle) {
    projectile_t* projectile = safe_malloc(sizeof(projectile_t), __LINE__);

    projectile->position.x = tower->position.x;
    projectile->position.y = tower->position.y;
    projectile->speed = tower->projectile_speed;
    projectile->pierce = tower->pierce;
    projectile->damage = tower->damage;
    projectile->damage_type = tower->damage_type;
    projectile->sprite = tower_projectile_table[tower->type];
    projectile->angle = angle;
    projectile->lifetime = 120;  /* ~2 seconds at 60fps, despawn after */
    projectile->owner = (void*)tower;

    (void)game;
    return projectile;
}

/* ── Path Collision Check ────────────────────────────────────────────── */

bool boxesCollide(position_t p1, int width1, int height1, position_t p2,
                  int width2, int height2) {
    return (p1.x < p2.x + width2 && p1.x + width1 > p2.x &&
            p1.y < p2.y + height2 && p1.y + height1 > p2.y);
}

bool on_path(game_t* game, position_t pos, int w, int h) {
    size_t num_rects = game->path->num_points - 1;
    for (size_t i = 0; i < num_rects; i++) {
        rectangle_t* r = &game->path->rectangles[i];
        /* Use bounding box of the rectangle */
        int rx = r->upper_left.x;
        int ry = r->upper_left.y;
        int rw = r->lower_right.x - r->upper_left.x;
        int rh = r->lower_right.y - r->upper_left.y;
        if (rw < 0) { rx += rw; rw = -rw; }
        if (rh < 0) { ry += rh; rh = -rh; }
        if (boxesCollide(pos, w, h, (position_t){rx, ry}, rw, rh)) {
            return true;
        }
    }
    return false;
}

bool overlaps_tower(game_t* game, position_t pos, int w, int h) {
    list_ele_t* curr = game->towers->head;
    while (curr != NULL) {
        tower_t* t = (tower_t*)(curr->value);
        int half = t->sprite->width / 2;
        position_t tl = { t->position.x - half, t->position.y - half };
        if (boxesCollide(pos, w, h, tl, t->sprite->width, t->sprite->height)) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

/* ── Key Handling ────────────────────────────────────────────────────── */

void handlePlayingKeys(game_t* game) {
    kb_Scan();

    /* Movement - always responsive, no debounce */
    if (kb_Data[7] & kb_Up)    game->cursor.y -= 2;
    if (kb_Data[7] & kb_Down)  game->cursor.y += 2;
    if (kb_Data[7] & kb_Left)  game->cursor.x -= 2;
    if (kb_Data[7] & kb_Right) game->cursor.x += 2;

    /* Clamp cursor to screen */
    if (game->cursor.x < 0) game->cursor.x = 0;
    if (game->cursor.y < 0) game->cursor.y = 0;
    if (game->cursor.x >= SCREEN_WIDTH) game->cursor.x = SCREEN_WIDTH - 1;
    if (game->cursor.y >= SCREEN_HEIGHT) game->cursor.y = SCREEN_HEIGHT - 1;

    /* Debounce action keys */
    if (game->key_delay > 0) { game->key_delay--; return; }

    /* + key: open buy menu */
    if (kb_Data[6] & kb_Add) {
        game->screen = SCREEN_BUY_MENU;
        game->buy_menu_cursor = 0;
        game->key_delay = KEY_DELAY;
        return;
    }

    /* Enter key */
    if (kb_Data[6] & kb_Enter) {
        /* Check speed button first */
        if (game->cursor.x >= SPEED_BTN_X && game->cursor.x < SPEED_BTN_X + SPEED_BTN_W &&
            game->cursor.y >= SPEED_BTN_Y && game->cursor.y < SPEED_BTN_Y + SPEED_BTN_H) {
            game->fast_forward = !game->fast_forward;
            game->key_delay = KEY_DELAY;
        } else if (game->cursor_type == CURSOR_SELECTED) {
            /* Try to place tower (cursor = center of tower) */
            uint8_t type = game->selected_tower_type;
            uint16_t cost = TOWER_DATA[type].cost;
            gfx_sprite_t* spr = tower_sprite_table[type];
            int half_w = spr->width / 2;
            int half_h = spr->height / 2;
            position_t tl = { game->cursor.x - half_w, game->cursor.y - half_h };

            bool can_afford = game->SANDBOX || (game->coins >= (int16_t)cost);
            bool valid_pos = !on_path(game, tl, spr->width, spr->height) &&
                             !overlaps_tower(game, tl, spr->width, spr->height);

            if (can_afford && valid_pos) {
                if (!game->SANDBOX) game->coins -= cost;
                tower_t* tower = initTower(game, type);
                queue_insert_head(game->towers, (void*)tower);
                game->cursor_type = CURSOR_NONE;
            }
        } else {
            /* Try to select existing tower for upgrade (positions are centers) */
            list_ele_t* curr = game->towers->head;
            while (curr != NULL) {
                tower_t* t = (tower_t*)(curr->value);
                int half = t->sprite->width / 2;
                if (game->cursor.x >= t->position.x - half &&
                    game->cursor.x < t->position.x + half &&
                    game->cursor.y >= t->position.y - half &&
                    game->cursor.y < t->position.y + half) {
                    game->selected_tower = t;
                    game->upgrade_path_sel = 0;
                    game->screen = SCREEN_UPGRADE;
                    game->key_delay = KEY_DELAY;
                    return;
                }
                curr = curr->next;
            }
        }
        game->key_delay = KEY_DELAY;
    }

    /* Del key: deselect tower placement */
    if (kb_Data[1] & kb_Del) {
        game->cursor_type = CURSOR_NONE;
        game->key_delay = KEY_DELAY;
    }

    /* Mode key: toggle sandbox */
    if (kb_Data[1] & kb_Mode) {
        game->SANDBOX = !game->SANDBOX;
        game->key_delay = KEY_DELAY;
    }

    /* Graph key: skip round (sandbox only) */
    if ((kb_Data[1] & kb_Graph) && game->SANDBOX) {
        /* Clear all bloons */
        list_ele_t* curr_box = game->bloons->inited_boxes->head;
        while (curr_box != NULL) {
            list_ele_t* next_box = curr_box->next;
            list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
            while (curr_elem != NULL) {
                list_ele_t* next = curr_elem->next;
                sp_remove(game->bloons, ((bloon_t*)(curr_elem->value))->position,
                          curr_elem, free);
                curr_elem = next;
            }
            curr_box = next_box;
        }
        game->round_state.complete = true;
        game->key_delay = KEY_DELAY;
    }

    /* Clear key: exit */
    if (kb_Data[6] & kb_Clear) game->exit = true;
}

void handleBuyMenu(game_t* game) {
    kb_Scan();
    if (game->key_delay > 0) { game->key_delay--; return; }

    /* Arrow navigation in 2x4 grid */
    if (kb_Data[7] & kb_Right) {
        if (game->buy_menu_cursor < NUM_TOWER_TYPES - 1)
            game->buy_menu_cursor++;
        game->key_delay = KEY_DELAY;
    }
    if (kb_Data[7] & kb_Left) {
        if (game->buy_menu_cursor > 0)
            game->buy_menu_cursor--;
        game->key_delay = KEY_DELAY;
    }
    if (kb_Data[7] & kb_Down) {
        if (game->buy_menu_cursor + 4 < NUM_TOWER_TYPES)
            game->buy_menu_cursor += 4;
        game->key_delay = KEY_DELAY;
    }
    if (kb_Data[7] & kb_Up) {
        if (game->buy_menu_cursor >= 4)
            game->buy_menu_cursor -= 4;
        game->key_delay = KEY_DELAY;
    }

    /* Enter: select tower to place */
    if (kb_Data[6] & kb_Enter) {
        game->selected_tower_type = game->buy_menu_cursor;
        game->cursor_type = CURSOR_SELECTED;
        game->screen = SCREEN_PLAYING;
        game->key_delay = KEY_DELAY;
        return;
    }

    /* Del or Clear: cancel */
    if ((kb_Data[1] & kb_Del) || (kb_Data[6] & kb_Clear)) {
        game->cursor_type = CURSOR_NONE;
        game->screen = SCREEN_PLAYING;
        game->key_delay = KEY_DELAY;
    }
}

void handleUpgradeScreen(game_t* game) {
    kb_Scan();
    if (game->key_delay > 0) { game->key_delay--; return; }

    tower_t* tower = game->selected_tower;

    /* Left/Right: switch selected upgrade path */
    if (kb_Data[7] & kb_Left) {
        game->upgrade_path_sel = 0;
        game->key_delay = KEY_DELAY;
    }
    if (kb_Data[7] & kb_Right) {
        game->upgrade_path_sel = 1;
        game->key_delay = KEY_DELAY;
    }

    /* Enter: buy next upgrade on selected path */
    if (kb_Data[6] & kb_Enter) {
        uint8_t path = game->upgrade_path_sel;
        uint8_t other = 1 - path;
        /* Max 4 on primary path, but secondary path capped at 2.
         * If the OTHER path already has 3+, this path can only go to 2. */
        uint8_t max_level = (tower->upgrades[other] >= 3) ? 2 : 4;
        if (tower->upgrades[path] < max_level) {
            uint16_t cost = TOWER_UPGRADES[tower->type][path][tower->upgrades[path]].cost;
            bool can_afford = game->SANDBOX || (game->coins >= (int16_t)cost);
            if (can_afford) {
                if (!game->SANDBOX) game->coins -= cost;
                tower->total_invested += cost;
                tower->upgrades[path]++;
                apply_upgrades(tower);
            }
        }
        game->key_delay = KEY_DELAY;
    }

    /* Del or Clear: back to playing */
    if ((kb_Data[1] & kb_Del) || (kb_Data[6] & kb_Clear)) {
        game->selected_tower = NULL;
        game->screen = SCREEN_PLAYING;
        game->key_delay = KEY_DELAY;
    }
}

/* ── Drawing Helpers ──────────────────────────────────────────────────── */

/* Get appropriate bloon sprite based on type + state (damage, glue) */
gfx_sprite_t* get_bloon_sprite(bloon_t* bloon) {
    if (bloon->type == BLOON_MOAB) {
        if (bloon->slow_timer > 0) return moab_acid;
        if (bloon->hp <= 50)  return moab_damaged_3;
        if (bloon->hp <= 100) return moab_damaged_2;
        if (bloon->hp <= 150) return moab_damaged_1;
        return moab_undamaged;
    }
    if (bloon->type == BLOON_RED && bloon->slow_timer > 0) return red_acid;
    return bloon_sprite_table[bloon->type];
}

/* Get travel direction angle (0-255) from current path segment */
uint8_t bloon_direction(bloon_t* bloon, path_t* path) {
    uint16_t seg = bloon->segment;
    if (seg >= path->num_points - 1) {
        if (path->num_points >= 2) seg = path->num_points - 2;
        else return 0;
    }
    int16_t dx = path->points[seg + 1].x - path->points[seg].x;
    int16_t dy = path->points[seg + 1].y - path->points[seg].y;
    return iatan2(dy, dx);
}

/* HUD bar: HP, Round, Coins -- drawn on ALL screens */
void drawHUD(game_t* game) {
    gfx_SetColor(24);  /* dark gray bar */
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, 14);
    gfx_SetColor(80);
    gfx_HorizLine(0, 14, SCREEN_WIDTH);

    gfx_SetTextFGColor(255);  /* yellow - visible in this palette */
    gfx_PrintStringXY("HP:", 4, 3);
    gfx_PrintInt(game->hearts, 1);

    gfx_PrintStringXY("Round:", 80, 3);
    gfx_PrintInt(game->round + 1, 1);

    gfx_PrintStringXY("$", 180, 3);
    gfx_PrintInt(game->coins, 1);

    if (game->SANDBOX) {
        gfx_PrintStringXY("SBX", 280, 3);
    }
}

/* ── Drawing Functions ───────────────────────────────────────────────── */

void drawCursor(game_t* game) {
    int x = game->cursor.x;
    int y = game->cursor.y;

    if (game->cursor_type == CURSOR_SELECTED) {
        gfx_sprite_t* spr = tower_sprite_table[game->selected_tower_type];
        int half = spr->width / 2;
        gfx_TransparentSprite(spr, x - half, y - half);
        /* Range circle: red if invalid placement, white if valid */
        uint8_t range = TOWER_DATA[game->selected_tower_type].range;
        position_t tl = { x - half, y - half };
        bool valid = !on_path(game, tl, spr->width, spr->height) &&
                     !overlaps_tower(game, tl, spr->width, spr->height);
        gfx_SetColor(valid ? 30 : 133);  /* green or red */
        gfx_Circle(x, y, range);
    } else {
        /* Small selection circle */
        gfx_SetColor(255);
        gfx_Circle(x, y, 5);
    }
}

void drawMap(game_t* game) {
    gfx_SetColor(158);
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    drawGamePath(game);
}

void drawSpeedButton(game_t* game) {
    /* Check if cursor is hovering over the button */
    bool hover = (game->cursor.x >= SPEED_BTN_X &&
                  game->cursor.x < SPEED_BTN_X + SPEED_BTN_W &&
                  game->cursor.y >= SPEED_BTN_Y &&
                  game->cursor.y < SPEED_BTN_Y + SPEED_BTN_H);

    /* Background */
    gfx_SetColor(game->fast_forward ? 40 : 8);
    gfx_FillRectangle(SPEED_BTN_X, SPEED_BTN_Y, SPEED_BTN_W, SPEED_BTN_H);

    /* Border: white when toggled, gold on hover, gray default */
    if (game->fast_forward) {
        gfx_SetColor(255);  /* white = active */
    } else if (hover) {
        gfx_SetColor(148);  /* bright gold = hover */
    } else {
        gfx_SetColor(80);   /* dim = default */
    }
    gfx_Rectangle(SPEED_BTN_X, SPEED_BTN_Y, SPEED_BTN_W, SPEED_BTN_H);

    /* Text: ">>" when fast, ">" when normal */
    gfx_SetTextScale(2, 2);
    gfx_SetTextFGColor(game->fast_forward ? 255 : (hover ? 148 : 80));
    if (game->fast_forward) {
        gfx_PrintStringXY(">>", SPEED_BTN_X + 4, SPEED_BTN_Y + 8);
    } else {
        gfx_PrintStringXY(">", SPEED_BTN_X + 10, SPEED_BTN_Y + 8);
    }
    gfx_SetTextScale(1, 1);  /* reset to normal */
}

void drawStats(game_t* game) {
    drawHUD(game);
}

void drawTowers(game_t* game) {
    list_ele_t* curr_elem = game->towers->head;
    while (curr_elem != NULL) {
        tower_t* tower = (tower_t*)(curr_elem->value);
        int half = tower->sprite->width / 2;

        if (tower->type == TOWER_TACK || tower->type == TOWER_ICE) {
            /* Radial/area towers: don't rotate, looks weird */
            gfx_TransparentSprite(tower->sprite,
                                   tower->position.x - half,
                                   tower->position.y - half);
        } else {
            /* Use projectile native angle: same offset that makes projectiles correct.
             * Most tower sprites face similar direction to their projectile sprite. */
            uint8_t rot = (uint8_t)(tower->facing_angle - proj_native_angle[tower->type]);
            gfx_RotatedScaledTransparentSprite(
                tower->sprite,
                tower->position.x - half,
                tower->position.y - half,
                rot,
                64  /* 100% scale */
            );
        }
        curr_elem = curr_elem->next;
    }
}

void drawBloons(game_t* game) {
    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            bloon_t* bloon = (bloon_t*)(curr_elem->value);
            gfx_sprite_t* spr = get_bloon_sprite(bloon);

            /* Center sprite on bloon position */
            int draw_x = bloon->position.x - (spr->width / 2);
            int draw_y = bloon->position.y - (spr->height / 2);

            if (bloon->type == BLOON_MOAB) {
                /* MOABs rotate to face travel direction.
                 * MOAB sprite native orientation = facing left (128).
                 * rotation = travel_angle - 128 */
                uint8_t dir = bloon_direction(bloon, game->path);
                uint8_t rot = (uint8_t)(dir - 128);  /* MOAB native=left(128), CW rotation */
                gfx_RotatedScaledTransparentSprite(spr, draw_x, draw_y,
                                                    rot, 64);
            } else {
                gfx_TransparentSprite(spr, draw_x, draw_y);
            }

            /* Freeze indicator: blue border */
            if (bloon->freeze_timer > 0) {
                gfx_SetColor(0x5F);
                gfx_Rectangle(draw_x - 1, draw_y - 1,
                              spr->width + 2, spr->height + 2);
            }
            /* Glue indicator: green dot (non-MOAB, non-red which have acid sprite) */
            if (bloon->slow_timer > 0 && bloon->type != BLOON_RED && bloon->type != BLOON_MOAB) {
                gfx_SetColor(0x07);
                gfx_FillCircle(bloon->position.x, bloon->position.y - (spr->height / 2) - 3, 2);
            }

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

            if (projectile->sprite != NULL) {
                int half = projectile->sprite->width / 2;
                /* SDK rotation is CW: rot = travel_angle - native */
                uint8_t native = 192;  /* default: up */
                if (projectile->owner != NULL) {
                    native = proj_native_angle[((tower_t*)projectile->owner)->type];
                }
                uint8_t rot = (uint8_t)(projectile->angle - native);
                gfx_RotatedScaledTransparentSprite(
                    projectile->sprite,
                    projectile->position.x - half,
                    projectile->position.y - half,
                    rot,
                    64  /* 100% scale */
                );
            } else {
                /* Glue: draw small green circle */
                gfx_SetColor(0x07);  /* green-ish */
                gfx_FillCircle(projectile->position.x, projectile->position.y, 3);
            }
            curr_elem = curr_elem->next;
        }
        curr_box = curr_box->next;
    }
}

void drawBuyMenu(game_t* game) {
    /* Full-screen black background */
    gfx_SetColor(0);
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    /* HUD at top */
    drawHUD(game);

    /* 2 rows x 4 columns grid */
    const int cell_w = 78;
    const int cell_h = 92;
    const int grid_x = 2;
    const int grid_y = 18;

    for (int i = 0; i < NUM_TOWER_TYPES; i++) {
        int col = i % 4;
        int row = i / 4;
        int cx = grid_x + col * (cell_w + 1);
        int cy = grid_y + row * (cell_h + 2);

        /* Cell border */
        gfx_SetColor(80);  /* gray border */
        gfx_Rectangle(cx, cy, cell_w, cell_h);

        /* Highlight selected cell */
        if (i == game->buy_menu_cursor) {
            gfx_SetColor(148);  /* yellow highlight */
            gfx_Rectangle(cx, cy, cell_w, cell_h);
            gfx_Rectangle(cx + 1, cy + 1, cell_w - 2, cell_h - 2);
        }

        /* Tower sprite centered horizontally, near top of cell */
        gfx_sprite_t* spr = tower_sprite_table[i];
        int sx = cx + (cell_w - spr->width) / 2;
        int sy = cy + 4;
        gfx_TransparentSprite(spr, sx, sy);

        /* Tower name centered below sprite */
        gfx_SetTextFGColor(255);
        int name_w = gfx_GetStringWidth(TOWER_NAMES[i]);
        gfx_PrintStringXY(TOWER_NAMES[i], cx + (cell_w - name_w) / 2,
                           cy + cell_h - 24);

        /* Cost centered below name */
        {
            char buf[8];
            buf[0] = '$';
            /* int to string for cost */
            uint16_t c = TOWER_DATA[i].cost;
            int len = 1;
            if (c >= 1000) buf[len++] = '0' + (c / 1000) % 10;
            if (c >= 100)  buf[len++] = '0' + (c / 100) % 10;
            if (c >= 10)   buf[len++] = '0' + (c / 10) % 10;
            buf[len++] = '0' + c % 10;
            buf[len] = '\0';
            int cost_w = gfx_GetStringWidth(buf);
            gfx_PrintStringXY(buf, cx + (cell_w - cost_w) / 2, cy + cell_h - 12);
        }
    }

    /* Bottom hint */
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("[Enter] Buy  [Del] Back", 80, SCREEN_HEIGHT - 10);
}

void drawUpgradeScreen(game_t* game) {
    tower_t* tower = game->selected_tower;
    if (tower == NULL) return;

    /* Full-screen black background */
    gfx_SetColor(0);
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    /* HUD at top */
    drawHUD(game);

    /* Tower name + sprite header */
    gfx_SetTextFGColor(255);
    int name_w = gfx_GetStringWidth(TOWER_NAMES[tower->type]);
    gfx_PrintStringXY(TOWER_NAMES[tower->type], (SCREEN_WIDTH - name_w) / 2, 18);

    gfx_TransparentSprite(tower->sprite,
                           (SCREEN_WIDTH - tower->sprite->width) / 2, 30);

    /* Stats row */
    int sy = 30 + tower->sprite->height + 2;
    gfx_SetColor(24);
    gfx_FillRectangle(0, sy, SCREEN_WIDTH, 12);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Dmg:", 4, sy + 2);
    gfx_SetTextXY(32, sy + 2); gfx_PrintInt(tower->damage, 1);
    gfx_PrintStringXY("Prc:", 56, sy + 2);
    gfx_SetTextXY(84, sy + 2); gfx_PrintInt(tower->pierce, 1);
    gfx_PrintStringXY("Rng:", 108, sy + 2);
    gfx_SetTextXY(136, sy + 2); gfx_PrintInt(tower->range, 1);
    gfx_PrintStringXY("Spd:", 164, sy + 2);
    gfx_SetTextXY(192, sy + 2); gfx_PrintInt(tower->cooldown, 1);
    gfx_PrintStringXY("Pops:", 220, sy + 2);
    gfx_SetTextXY(256, sy + 2); gfx_PrintInt(tower->pop_count, 1);

    /* Two columns for upgrade paths */
    const int col_x[2] = { 4, 162 };
    const int col_w = 154;
    int path_y = sy + 16;
    uint8_t sel = game->upgrade_path_sel;

    for (int path = 0; path < 2; path++) {
        bool is_sel = (path == (int)sel);

        /* Path header - highlighted if selected */
        gfx_SetColor(is_sel ? 40 : 16);
        gfx_FillRectangle(col_x[path], path_y, col_w, 12);
        if (is_sel) {
            gfx_SetColor(148);
            gfx_Rectangle(col_x[path], path_y, col_w, 12);
        }
        gfx_SetTextFGColor(is_sel ? 255 : 80);
        gfx_PrintStringXY(path == 0 ? "< Path 1" : "Path 2 >",
                           col_x[path] + 4, path_y + 2);

        /* Cap: if other path has 3+, this path maxes at 2 */
        int other = 1 - path;
        uint8_t max_level = (tower->upgrades[other] >= 3) ? 2 : 4;

        for (int level = 0; level < 4; level++) {
            int y = path_y + 14 + level * 22;
            const char* name = UPGRADE_NAMES[tower->type][path][level];

            if (level < tower->upgrades[path]) {
                /* Purchased: green border + text */
                gfx_SetColor(16);
                gfx_FillRectangle(col_x[path], y, col_w, 20);
                gfx_SetColor(0x07);
                gfx_Rectangle(col_x[path], y, col_w, 20);
                gfx_SetTextFGColor(0x07);
                gfx_PrintStringXY(name, col_x[path] + 4, y + 6);
                gfx_PrintStringXY("OK", col_x[path] + col_w - 22, y + 6);
            } else if (level == tower->upgrades[path] && level < max_level) {
                /* Next available upgrade */
                uint16_t cost = TOWER_UPGRADES[tower->type][path][level].cost;
                gfx_SetColor(8);
                gfx_FillRectangle(col_x[path], y, col_w, 20);
                gfx_SetColor(is_sel ? 255 : 60);
                gfx_Rectangle(col_x[path], y, col_w, 20);
                gfx_SetTextFGColor(is_sel ? 255 : 80);
                gfx_PrintStringXY(name, col_x[path] + 4, y + 2);
                gfx_SetTextXY(col_x[path] + 4, y + 11);
                gfx_PrintChar('$');
                gfx_PrintInt(cost, 1);
            } else {
                /* Locked */
                gfx_SetColor(4);
                gfx_FillRectangle(col_x[path], y, col_w, 20);
                gfx_SetColor(24);
                gfx_Rectangle(col_x[path], y, col_w, 20);
                gfx_SetTextFGColor(24);
                gfx_PrintStringXY(name, col_x[path] + 4, y + 6);
            }
        }
    }

    /* Bottom bar */
    gfx_SetColor(24);
    gfx_FillRectangle(0, SCREEN_HEIGHT - 14, SCREEN_WIDTH, 14);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Sell:$", 4, SCREEN_HEIGHT - 12);
    gfx_SetTextXY(52, SCREEN_HEIGHT - 12);
    gfx_PrintInt((tower->total_invested * 80) / 100, 1);
    gfx_PrintStringXY("[Enter]Buy [Del]Back", 140, SCREEN_HEIGHT - 12);
}

/* ── Bloon Movement ──────────────────────────────────────────────────── */

int moveBloon(game_t* game, bloon_t* bloon) {
    int num_segments = game->path->num_points - 1;
    int speed_fp = BLOON_DATA[bloon->type].speed_fp;

    /* Frozen bloons don't move */
    if (bloon->freeze_timer > 0) {
        bloon->freeze_timer--;
        return bloon->segment;
    }

    /* Slowed bloons move at half speed */
    if (bloon->slow_timer > 0) {
        speed_fp /= SLOW_FACTOR;
        bloon->slow_timer--;
    }

    int movement = speed_fp;

    while (movement > 0 && bloon->segment < num_segments) {
        position_t cur = bloon->position;
        position_t target = game->path->points[bloon->segment + 1];
        int dx = target.x - cur.x;
        int dy = target.y - cur.y;
        int dist_fp = (abs(dx) + abs(dy)) << 8;

        if (movement >= dist_fp) {
            bloon->position = target;
            movement -= dist_fp;
            bloon->segment++;
        } else {
            if (abs(dx) >= abs(dy)) {
                int step = (dx > 0) ? (movement >> 8) : -(movement >> 8);
                bloon->position.x += step;
            } else {
                int step = (dy > 0) ? (movement >> 8) : -(movement >> 8);
                bloon->position.y += step;
            }
            movement = 0;
        }
    }
    return bloon->segment;
}

/* ── Bloon Popping ───────────────────────────────────────────────────── */

void popBloon(game_t* game, bloon_t* bloon, position_t pos) {
    const bloon_data_t* data = &BLOON_DATA[bloon->type];
    game->coins += 1;

    for (int i = 0; i < data->child_count; i++) {
        bloon_t* child = safe_malloc(sizeof(bloon_t), __LINE__);
        child->type = data->child_type;
        child->modifiers = bloon->modifiers;
        child->hp = BLOON_DATA[data->child_type].hp;
        child->regrow_max = bloon->regrow_max;
        child->regrow_timer = REGROW_INTERVAL;
        child->segment = bloon->segment;
        child->progress = 0;
        child->position = pos;
        child->freeze_timer = 0;
        child->slow_timer = 0;
        sp_insert(game->bloons, child->position, child);
    }

    if (data->child_type2 != 0xFF) {
        for (int i = 0; i < data->child_count2; i++) {
            bloon_t* child = safe_malloc(sizeof(bloon_t), __LINE__);
            child->type = data->child_type2;
            child->modifiers = bloon->modifiers;
            child->hp = BLOON_DATA[data->child_type2].hp;
            child->regrow_max = bloon->regrow_max;
            child->regrow_timer = REGROW_INTERVAL;
            child->segment = bloon->segment;
            child->progress = 0;
            child->position = pos;
            child->freeze_timer = 0;
            child->slow_timer = 0;
            sp_insert(game->bloons, child->position, child);
        }
    }
}

/* ── Spawn ───────────────────────────────────────────────────────────── */

void spawnBloons(game_t* game) {
    round_state_t* rs = &game->round_state;
    if (rs->complete) return;

    const round_def_t* rd = &ROUND_DEFS[game->round];
    const round_group_t* group = &rd->groups[rs->group_index];

    if (rs->spacing_timer > 0) {
        rs->spacing_timer--;
        return;
    }

    bloon_t* bloon = initBloon(game, group->bloon_type, group->modifiers);
    sp_insert(game->bloons, bloon->position, bloon);
    rs->spawned++;
    rs->spacing_timer = group->spacing;

    if (rs->spawned >= group->count) {
        rs->group_index++;
        rs->spawned = 0;
        if (rs->group_index >= rd->num_groups) {
            rs->complete = true;
        }
    }
}

/* ── Update Functions ────────────────────────────────────────────────── */

bool offScreen(position_t p) {
    return (p.x < -16 || p.y < -16 || p.x > SCREEN_WIDTH + 16 || p.y > SCREEN_HEIGHT + 16);
}

void updateBloons(game_t* game) {
    const int num_segments = game->path->num_points - 1;

    list_ele_t* curr_bloon_box = game->bloons->inited_boxes->head;
    while (curr_bloon_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_bloon_box->value))->head;
        list_ele_t* tmp;
        while (curr_elem != NULL) {
            bloon_t* curr_bloon = (bloon_t*)(curr_elem->value);
            position_t pos_before_move = curr_bloon->position;
            int segBeforeMove = curr_bloon->segment;
            if (segBeforeMove >= num_segments ||
                moveBloon(game, curr_bloon) >= num_segments) {
                game->hearts -= BLOON_DATA[curr_bloon->type].rbe;
                tmp = curr_elem->next;
                sp_remove(game->bloons, pos_before_move, curr_elem, free);
                curr_elem = tmp;
                continue;
            }

            /* Regrow mechanic */
            if (curr_bloon->modifiers & MOD_REGROW) {
                if (curr_bloon->type < curr_bloon->regrow_max) {
                    curr_bloon->regrow_timer--;
                    if (curr_bloon->regrow_timer == 0) {
                        curr_bloon->type++;
                        curr_bloon->hp = BLOON_DATA[curr_bloon->type].hp;
                        curr_bloon->regrow_timer = REGROW_INTERVAL;
                    }
                }
            }

            curr_elem = curr_elem->next;
        }
        curr_bloon_box = curr_bloon_box->next;
    }

    /* Fix spatial partition boxes after movement */
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
    list_ele_t* curr_elem = game->towers->head;
    while (curr_elem != NULL) {
        tower_t* tower = (tower_t*)(curr_elem->value);
        tower->tick++;

        if (tower->tick >= tower->cooldown) {
            tower->tick = 0;
            const tower_data_t* base = &TOWER_DATA[tower->type];

            if (base->is_area) {
                /* ── Ice Tower: area freeze ────────────────────────── */
                int range_sq = (int)tower->range * (int)tower->range;
                int hit_count = 0;

                list_ele_t* bx = game->bloons->inited_boxes->head;
                while (bx != NULL) {
                    list_ele_t* be = ((queue_t*)(bx->value))->head;
                    while (be != NULL) {
                        bloon_t* bloon = (bloon_t*)(be->value);

                        /* Skip camo if can't see */
                        if ((bloon->modifiers & MOD_CAMO) && !tower->can_see_camo) {
                            be = be->next;
                            continue;
                        }

                        /* Check immunity to freeze, or already frozen */
                        if ((BLOON_DATA[bloon->type].immunities & IMMUNE_FREEZE) ||
                            bloon->freeze_timer > 0) {
                            be = be->next;
                            continue;
                        }

                        int dx = bloon->position.x - tower->position.x;
                        int dy = bloon->position.y - tower->position.y;
                        if (dx * dx + dy * dy <= range_sq && hit_count < tower->pierce) {
                            bloon->freeze_timer = FREEZE_DURATION;
                            if (tower->damage > 0) {
                                bloon->hp -= tower->damage;
                                tower->pop_count++;
                            }
                            hit_count++;
                        }
                        be = be->next;
                    }
                    bx = bx->next;
                }
            } else if (base->is_hitscan) {
                /* ── Sniper: instant damage ────────────────────────── */
                bloon_t* target = find_target_bloon(game, tower);
                if (target) {
                    tower->facing_angle = calculate_angle_int(tower->position, target->position);
                    /* Check immunity */
                    if (!(BLOON_DATA[target->type].immunities & tower->damage_type) ||
                        tower->damage_type == DMG_NORMAL) {
                        target->hp -= tower->damage;
                        tower->pop_count += tower->damage;
                    }
                }
            } else if (tower->type == TOWER_GLUE) {
                /* ── Glue: shoot glue projectile ───────────────────── */
                bloon_t* target = find_target_bloon(game, tower);
                if (target) {
                    position_t predicted = predict_bloon_position(target, game->path);
                    uint8_t angle = calculate_angle_int(tower->position, predicted);
                    tower->facing_angle = angle;
                    projectile_t* proj = initProjectile(game, tower, angle);
                    sp_insert(game->projectiles, proj->position, proj);
                }
            } else {
                /* ── Normal projectile towers ──────────────────────── */
                bloon_t* target = find_target_bloon(game, tower);
                if (target) {
                    position_t predicted = predict_bloon_position(target, game->path);
                    uint8_t base_angle = calculate_angle_int(tower->position, predicted);
                    tower->facing_angle = base_angle;

                    if (tower->projectile_count == 1) {
                        projectile_t* proj = initProjectile(game, tower, base_angle);
                        sp_insert(game->projectiles, proj->position, proj);
                    } else {
                        /* Multi-projectile (tack shooter, etc) */
                        uint8_t step = 256 / tower->projectile_count;
                        for (int i = 0; i < tower->projectile_count; i++) {
                            uint8_t angle = (uint8_t)(i * step);
                            projectile_t* proj = initProjectile(game, tower, angle);
                            sp_insert(game->projectiles, proj->position, proj);
                        }
                    }
                }
            }
        }
        curr_elem = curr_elem->next;
    }
}

void updateProjectiles(game_t* game) {
    list_ele_t* curr_box = game->projectiles->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        list_ele_t* tmp;
        while (curr_elem != NULL) {
            projectile_t* proj = (projectile_t*)(curr_elem->value);
            tmp = curr_elem->next;

            /* Despawn if off-screen or lifetime expired */
            if (offScreen(proj->position) || proj->lifetime == 0) {
                sp_remove(game->projectiles, proj->position, curr_elem, free);
                curr_elem = tmp;
                continue;
            }
            proj->lifetime--;

            /* Integer movement using LUT */
            proj->position.x += (int16_t)((cos_lut[proj->angle] * (int16_t)proj->speed) >> 8);
            proj->position.y += (int16_t)((sin_lut[proj->angle] * (int16_t)proj->speed) >> 8);

            curr_elem = tmp;
        }
        curr_box = curr_box->next;
    }

    /* Fix spatial partition boxes */
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

void checkBloonProjCollissions(game_t* game) {
    list_ele_t* next_bloon_elem = NULL;
    list_ele_t* next_proj_elem = NULL;

    list_ele_t* curr_bloon_box = game->bloons->inited_boxes->head;
    while (curr_bloon_box != NULL) {
        list_ele_t* curr_bloon_elem = ((queue_t*)(curr_bloon_box->value))->head;
        if (curr_bloon_elem == NULL) {
            curr_bloon_box = curr_bloon_box->next;
            continue;
        }

        queue_t* same_box_projs = sp_soft_get_list(
            game->projectiles, ((bloon_t*)(curr_bloon_elem->value))->position);
        if (same_box_projs == NULL) {
            curr_bloon_box = curr_bloon_box->next;
            continue;
        }

        while (curr_bloon_elem != NULL) {
            next_bloon_elem = curr_bloon_elem->next;

            bloon_t* tmp_bloon = (bloon_t*)(curr_bloon_elem->value);
            gfx_sprite_t* bspr = bloon_sprite_table[tmp_bloon->type];
            int bw = bspr->width;
            int bh = bspr->height;
            /* Centered bounding box (matches visual centering) */
            position_t bloon_tl = { tmp_bloon->position.x - bw / 2,
                                    tmp_bloon->position.y - bh / 2 };

            list_ele_t* curr_proj_elem = same_box_projs->head;
            while (curr_proj_elem != NULL) {
                next_proj_elem = curr_proj_elem->next;

                projectile_t* tmp_proj = (projectile_t*)curr_proj_elem->value;

                int pw = tmp_proj->sprite ? tmp_proj->sprite->width : 6;
                int ph = tmp_proj->sprite ? tmp_proj->sprite->height : 6;
                position_t proj_tl = { tmp_proj->position.x - pw / 2,
                                       tmp_proj->position.y - ph / 2 };

                if (boxesCollide(bloon_tl, bw, bh, proj_tl, pw, ph)) {

                    /* Check immunity: if projectile's damage type is blocked
                     * by bloon's immunities, skip (unless DMG_NORMAL which bypasses all) */
                    if (tmp_proj->damage_type != DMG_NORMAL &&
                        (BLOON_DATA[tmp_bloon->type].immunities & tmp_proj->damage_type)) {
                        curr_proj_elem = next_proj_elem;
                        continue;
                    }

                    /* Glue projectile: pass through already-slowed bloons */
                    if (tmp_proj->damage_type == DMG_NORMAL && tmp_proj->damage == 0 &&
                        tmp_bloon->slow_timer > 0) {
                        curr_proj_elem = next_proj_elem;
                        continue;
                    }

                    /* Glue projectile: apply slow to un-slowed bloons */
                    if (tmp_proj->damage_type == DMG_NORMAL && tmp_proj->damage == 0) {
                        tmp_bloon->slow_timer = SLOW_DURATION;
                    }

                    /* Apply damage + track pops on owner tower */
                    tmp_bloon->hp -= tmp_proj->damage;
                    if (tmp_proj->owner != NULL) {
                        ((tower_t*)tmp_proj->owner)->pop_count += tmp_proj->damage;
                    }
                    if (tmp_bloon->hp <= 0) {
                        popBloon(game, tmp_bloon, tmp_bloon->position);
                        sp_remove(game->bloons, tmp_bloon->position,
                                  curr_bloon_elem, free);
                    }

                    /* Reduce projectile pierce */
                    tmp_proj->pierce--;
                    if (tmp_proj->pierce <= 0) {
                        sp_remove(game->projectiles, tmp_proj->position,
                                  curr_proj_elem, free);
                    }

                    break;  // move to next bloon
                }
                curr_proj_elem = next_proj_elem;
            }

            curr_bloon_elem = next_bloon_elem;
        }

        curr_bloon_box = curr_bloon_box->next;
    }
}

/* Check for bloons with hp <= 0 from hitscan/ice damage */
void checkHitscanPops(game_t* game) {
    list_ele_t* curr_box = game->bloons->inited_boxes->head;
    while (curr_box != NULL) {
        list_ele_t* curr_elem = ((queue_t*)(curr_box->value))->head;
        while (curr_elem != NULL) {
            list_ele_t* next = curr_elem->next;
            bloon_t* bloon = (bloon_t*)(curr_elem->value);
            if (bloon->hp <= 0) {
                popBloon(game, bloon, bloon->position);
                sp_remove(game->bloons, bloon->position, curr_elem, free);
            }
            curr_elem = next;
        }
        curr_box = curr_box->next;
    }
}

/* ── Game Logic ──────────────────────────────────────────────────────── */

void handleGame(game_t* game) {
    handlePlayingKeys(game);

    if (game->hearts <= 0) {
        delete_save();
        game->exit = true;
        return;
    }

    /* Check for round completion */
    round_state_t* rs = &game->round_state;
    if (rs->complete && sp_total_size(game->bloons) == 0) {
        game->coins += 100 + game->round;

        /* Save at end of each round */
        save_game(game);

        if (game->round >= game->max_round) {
            delete_save();
            game->exit = true;
            return;
        }

        game->round++;
        memset(&game->round_state, 0, sizeof(round_state_t));
        game->round_state.spacing_timer = 1;
    }

    spawnBloons(game);
    updateProjectiles(game);
    updateBloons(game);
    updateTowers(game);
    checkBloonProjCollissions(game);
    checkHitscanPops(game);
}

/* ── Game Creation ───────────────────────────────────────────────────── */

game_t* newGame(position_t* points, size_t num_points) {
    game_t* game = safe_malloc(sizeof(game_t), __LINE__);
    memset(game, 0, sizeof(game_t));

    game->path = newPath(points, num_points, DEFAULT_PATH_WIDTH);
    game->hearts = 100;
    game->coins = 650;

    game->towers = queue_new();
    game->bloons = new_partitioned_list(SCREEN_WIDTH, SCREEN_HEIGHT, 20);
    game->projectiles = new_partitioned_list(SCREEN_WIDTH, SCREEN_HEIGHT, 20);

    game->exit = false;
    game->cursor = (position_t){160, 120};
    game->cursor_type = CURSOR_NONE;

    game->round = 0;
    game->max_round = 79;
    game->round_active = true;
    game->round_state.spacing_timer = 1;

    game->screen = SCREEN_PLAYING;
    game->buy_menu_cursor = 0;
    game->selected_tower = NULL;
    game->selected_tower_type = TOWER_DART;
    game->upgrade_path_sel = 0;
    game->key_delay = 0;

    game->AUTOPLAY = false;
    game->SANDBOX = false;
    game->fast_forward = false;

    return game;
}

void exitGame(game_t* game) {
    free_partitioned_list(game->bloons, free);
    free_partitioned_list(game->projectiles, free);
    queue_free(game->towers, free);
    freePath(game->path);
    free(game);
}

/* ── Main Loop ───────────────────────────────────────────────────────── */

void runGame(void) {
    game_t* game = newGame(NULL, 0);

    /* Try to load a saved game */
    load_game(game);

    while (!game->exit) {
        switch (game->screen) {
            case SCREEN_PLAYING:
                handleGame(game);
                /* Fast forward: run a second game tick (no input/draw) */
                if (game->fast_forward && game->screen == SCREEN_PLAYING) {
                    spawnBloons(game);
                    updateProjectiles(game);
                    updateBloons(game);
                    updateTowers(game);
                    checkBloonProjCollissions(game);
                    checkHitscanPops(game);
                }
                drawMap(game);
                drawTowers(game);
                drawBloons(game);
                drawProjectiles(game);
                drawStats(game);
                drawSpeedButton(game);
                drawCursor(game);
                break;

            case SCREEN_BUY_MENU:
                handleBuyMenu(game);
                drawBuyMenu(game);
                break;

            case SCREEN_UPGRADE:
                handleUpgradeScreen(game);
                drawUpgradeScreen(game);
                break;
        }

        gfx_SwapDraw();
    }

    exitGame(game);
}

int main(void) {
    /* Load sprite appvars (must be before gfx_Begin) */
    if (BTDTW1_init() == 0 ||
        BTDTW2_init() == 0 ||
        BTDBLN_init() == 0 ||
        BTDUI_init() == 0) {
        return 1;
    }

    init_bloon_sprites();
    init_tower_sprites();

    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(1);
    gfx_SetTextTransparentColor(1);  /* default 255=white, change so white text works */
    gfx_SetTextBGColor(1);           /* text bg = transparent index = see-through */

    gfx_SetDrawBuffer();

    runGame();

    gfx_End();

    return 0;
}
