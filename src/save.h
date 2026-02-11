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
#define SETTINGS_APPVAR_NAME "BTDCFG"
#define SAVE_VERSION 3

typedef struct {
    uint8_t  version;
    uint16_t round;
    uint8_t  max_round;
    uint8_t  difficulty;
    int16_t  hearts;
    int24_t  coins;
    uint8_t  num_towers;
    uint8_t  sandbox;
    uint8_t  freeplay;
} save_header_t;

typedef struct {
    uint8_t version;
    uint8_t show_start_menu;  // 1=on, 0=off
    uint8_t auto_start;       // 1=on, 0=off
} settings_t;

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
bool save_exists(void);
bool save_settings(struct game_t_tag* game);
bool load_settings(struct game_t_tag* game);

#ifdef __cplusplus
}
#endif

#endif
