#ifndef SAVE_H
#define SAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declare game_t to avoid circular dependency */
struct game_t_tag;

#define SAVE_APPVAR_NAME "BTDSAVE"
#define SAVE_VERSION 1

typedef struct {
    uint8_t  version;
    uint8_t  round;
    uint8_t  max_round;
    uint8_t  difficulty;
    int16_t  hearts;
    int16_t  coins;
    uint8_t  num_towers;
    uint8_t  sandbox;
} save_header_t;

typedef struct {
    int16_t  x;
    int16_t  y;
    uint8_t  type;
    uint8_t  upgrades[2];
    uint8_t  target_mode;
} tower_save_t;

bool save_game(struct game_t_tag* game);
bool load_game(struct game_t_tag* game);
void delete_save(void);

#ifdef __cplusplus
}
#endif

#endif
