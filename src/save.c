#include "save.h"

#include <fileioc.h>
#include <string.h>
#include <debug.h>

#include "structs.h"
#include "towers.h"
#include "list.h"
#include "utils.h"

/* Apply upgrades from TOWER_DATA base + purchased upgrade deltas */
extern void apply_upgrades(tower_t* tower);

bool save_game(game_t* game) {
    ti_var_t slot = ti_Open(SAVE_APPVAR_NAME, "w");
    if (slot == 0) {
        dbg_printf("save_game: failed to open appvar\n");
        return false;
    }

    /* Count towers */
    uint8_t num_towers = 0;
    list_ele_t* curr = game->towers->head;
    while (curr != NULL) {
        num_towers++;
        curr = curr->next;
    }

    /* Write header */
    save_header_t header;
    header.version = SAVE_VERSION;
    header.round = game->round;
    header.max_round = game->max_round;
    header.difficulty = 0;
    header.hearts = game->hearts;
    header.coins = game->coins;
    header.num_towers = num_towers;
    header.sandbox = game->SANDBOX ? 1 : 0;

    if (ti_Write(&header, sizeof(save_header_t), 1, slot) != 1) {
        dbg_printf("save_game: failed to write header\n");
        ti_Close(slot);
        return false;
    }

    /* Write tower data */
    curr = game->towers->head;
    while (curr != NULL) {
        tower_t* tower = (tower_t*)(curr->value);
        tower_save_t ts;
        ts.x = tower->position.x;
        ts.y = tower->position.y;
        ts.type = tower->type;
        ts.upgrades[0] = tower->upgrades[0];
        ts.upgrades[1] = tower->upgrades[1];
        ts.target_mode = tower->target_mode;

        if (ti_Write(&ts, sizeof(tower_save_t), 1, slot) != 1) {
            dbg_printf("save_game: failed to write tower\n");
            ti_Close(slot);
            return false;
        }
        curr = curr->next;
    }

    ti_SetArchiveStatus(true, slot);
    ti_Close(slot);
    dbg_printf("save_game: saved round %d with %d towers\n",
               game->round, num_towers);
    return true;
}

bool load_game(game_t* game) {
    ti_var_t slot = ti_Open(SAVE_APPVAR_NAME, "r");
    if (slot == 0) {
        dbg_printf("load_game: no save found\n");
        return false;
    }

    /* Read header */
    save_header_t header;
    if (ti_Read(&header, sizeof(save_header_t), 1, slot) != 1) {
        dbg_printf("load_game: failed to read header\n");
        ti_Close(slot);
        return false;
    }

    if (header.version != SAVE_VERSION) {
        dbg_printf("load_game: version mismatch %d != %d\n",
                   header.version, SAVE_VERSION);
        ti_Close(slot);
        return false;
    }

    game->round = header.round;
    game->max_round = header.max_round;
    game->hearts = header.hearts;
    game->coins = header.coins;
    game->SANDBOX = header.sandbox ? true : false;

    /* Read and reconstruct towers */
    for (uint8_t i = 0; i < header.num_towers; i++) {
        tower_save_t ts;
        if (ti_Read(&ts, sizeof(tower_save_t), 1, slot) != 1) {
            dbg_printf("load_game: failed to read tower %d\n", i);
            ti_Close(slot);
            return false;
        }

        tower_t* tower = safe_malloc(sizeof(tower_t), __LINE__);
        memset(tower, 0, sizeof(tower_t));
        tower->position.x = ts.x;
        tower->position.y = ts.y;
        tower->type = ts.type;
        tower->upgrades[0] = ts.upgrades[0];
        tower->upgrades[1] = ts.upgrades[1];
        tower->target_mode = ts.target_mode;

        /* Compute effective stats from base + upgrades */
        apply_upgrades(tower);

        /* Calculate total invested for sell value */
        tower->total_invested = TOWER_DATA[tower->type].cost;
        for (int p = 0; p < 2; p++) {
            for (int l = 0; l < tower->upgrades[p]; l++) {
                tower->total_invested += TOWER_UPGRADES[tower->type][p][l].cost;
            }
        }

        queue_insert_head(game->towers, (void*)tower);
    }

    ti_Close(slot);

    /* Reset round state for the loaded round */
    memset(&game->round_state, 0, sizeof(round_state_t));
    game->round_state.spacing_timer = 1;
    game->round_active = true;

    dbg_printf("load_game: loaded round %d with %d towers\n",
               header.round, header.num_towers);
    return true;
}

void delete_save(void) {
    ti_Delete(SAVE_APPVAR_NAME);
    dbg_printf("delete_save: save deleted\n");
}
