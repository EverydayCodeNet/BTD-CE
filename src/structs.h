#ifndef STRUCTS_H
#define STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "list.h"

/*
Point on the canvas
(0, 0) => top left corner of canvas
(319, 239) => bottom right corner of canvas
*/
typedef struct {
    int16_t x;
    int16_t y;
} position_t;

typedef struct {
    // four points
    position_t upper_left;
    position_t lower_left;
    position_t upper_right;
    position_t lower_right;

    enum { HORZ, VERT, DIAG } kind;

} rectangle_t;

typedef struct {
    position_t* points;  // the points which make up the piecewise path
    rectangle_t* rectangles;
    size_t num_points;  // length of points
    int length;         // sum of lengths of line segments
    int width;          // width of the path
} path_t;

typedef struct bloon_t {
    position_t position;
    uint8_t type;           // bloon_type_t index into BLOON_DATA[]
    uint8_t modifiers;      // MOD_CAMO | MOD_REGROW bitmask
    int16_t hp;             // remaining HP for this layer
    uint8_t regrow_timer;   // frames until next regrow tick
    uint8_t regrow_max;     // highest type this bloon can regrow to
    uint16_t segment;       // current path segment index
    int16_t progress;       // sub-pixel progress along segment (fixed-point x256)
    uint8_t freeze_timer;   // frames remaining frozen (0 = not frozen)
    uint8_t slow_timer;     // frames remaining slowed by glue (0 = not slowed)
    uint8_t stun_timer;     // frames stunned (can't move)
    uint8_t dot_damage;     // damage-over-time per tick
    uint8_t dot_timer;      // frames remaining for DoT
    uint8_t dot_interval;   // frames between DoT ticks
    uint8_t dot_tick;       // current tick countdown
    uint8_t frozen_by_permafrost; // was frozen by tower with permafrost
} bloon_t;

typedef struct {
    position_t position;
    uint8_t  type;              // tower_type_t
    uint8_t  upgrades[2];       // path 0 and 1 levels (0-4)
    uint8_t  target_mode;       // 0=FIRST 1=LAST 2=STRONG 3=CLOSE
    uint16_t cooldown;          // effective frames between attacks
    uint16_t tick;              // frame counter
    uint8_t  damage;            // effective damage
    uint8_t  pierce;            // effective pierce
    uint8_t  range;             // effective range pixels
    uint8_t  damage_type;       // effective bitmask
    uint8_t  can_see_camo;      // effective
    uint8_t  projectile_count;  // effective
    uint8_t  projectile_speed;  // effective
    uint16_t total_invested;    // for sell value (80%)
    uint16_t pop_count;
    uint8_t  facing_angle;      // 0-255 LUT angle tower is facing
    gfx_sprite_t* sprite;       // current tower sprite
    uint8_t  splash_radius;     // passed to projectiles
    uint8_t  is_homing;         // passed to projectiles
    uint8_t  stun_on_hit;       // frames of stun passed to projectiles
    uint8_t  has_aura;          // Arctic Wind: slow bloons in range each frame
    uint8_t  dot_damage;        // Corrosive Glue: DoT damage per tick
    uint8_t  dot_interval;      // DoT tick rate
    uint8_t  slow_duration;     // Stickier Glue: longer slow
    uint8_t  moab_damage_mult;  // MOAB Mauler: multiplier vs MOAB-class
    uint8_t  permafrost;        // applies slow after freeze wears off
    uint8_t  distraction;       // chance to knock bloon back on hit
    uint8_t  glue_soak;         // glue applies to children on pop
} tower_t;

typedef struct {
    position_t position;
    gfx_sprite_t* sprite;
    uint8_t speed;
    uint8_t angle;              // 0-255 LUT angle
    uint8_t pierce;
    uint8_t damage;
    uint8_t damage_type;        // damage_type_t bitmask
    uint8_t lifetime;           // frames remaining before despawn
    void* owner;                // tower_t* that fired this (for pop count)
    uint8_t splash_radius;      // 0 = no splash, >0 = damage all bloons within radius
    uint8_t is_homing;          // 1 = seeks nearest bloon each frame
    uint8_t stun_duration;      // frames to stun bloon on hit
    uint8_t can_see_camo;       // projectile can hit camo
    uint8_t dot_damage;         // DoT to apply on hit (glue)
    uint8_t dot_interval;       // DoT tick interval to apply
    uint8_t glue_soak;          // glue applies to children
} projectile_t;

typedef struct {
    uint8_t group_index;    // which group in this round we're spawning
    uint16_t spawned;       // how many spawned in current group
    uint8_t spacing_timer;  // countdown to next spawn
    bool complete;          // all groups finished spawning
} round_state_t;

typedef struct {
    size_t width;               // width of space in terms of `box_size`
    size_t height;              // height of the space in terms of `box_size`
    size_t box_size;            // size of the squares we break the space into
    size_t num_boxes_in_range;  // width * height
    queue_t** boxes;            // boxes which collectively contain all inserted
    size_t n;                   // length of boxes
    size_t total_size;          // number of elements across all boxes
    queue_t* inited_boxes;  // since boxes are lazily calculated, this contains
                            // only the ones which have been actually inited;
                            // contains a pointer to box (which is a list)
} multi_list_t;

typedef enum {
    SCREEN_TITLE,
    SCREEN_SETTINGS,
    SCREEN_DIFFICULTY,
    SCREEN_PLAYING,
    SCREEN_BUY_MENU,
    SCREEN_UPGRADE,
    SCREEN_GAME_OVER,
    SCREEN_VICTORY,
    SCREEN_SPECTATE
} game_screen_t;

typedef enum {
    TARGET_FIRST = 0,
    TARGET_LAST,
    TARGET_STRONG,
    TARGET_CLOSE
} target_mode_t;

typedef enum {
    CURSOR_SELECTED,    // placing a tower
    CURSOR_NONE         // default circle cursor
} cursor_type_t;

typedef struct game_t_tag {
    path_t* path;
    int16_t hearts;
    int16_t coins;
    queue_t* towers;
    multi_list_t* bloons;
    multi_list_t* projectiles;
    round_state_t round_state;
    bool exit;
    cursor_type_t cursor_type;
    position_t cursor;
    uint16_t round;         // 0-indexed (round 0 = "Round 1")
    uint8_t max_round;      // 39 (easy), 59 (medium), 79 (hard)
    bool round_active;      // bloons still spawning or on screen

    game_screen_t screen;
    uint8_t buy_menu_cursor;        // 0-7 tower selection index
    tower_t* selected_tower;        // tower being upgraded
    uint8_t selected_tower_type;    // tower_type_t when placing
    uint8_t upgrade_path_sel;       // 0 or 1 - selected path in upgrade screen
    uint8_t key_delay;              // frames until next key input accepted

    uint8_t menu_cursor;        // title/settings/difficulty menu selection
    bool show_start_menu;       // persistent setting (default true)
    bool auto_start;            // persistent setting: auto-start rounds (default true)
    bool freeplay;              // in freeplay mode after victory
    bool spectate;              // spectate mode
    uint8_t difficulty;         // 0=easy, 1=medium, 2=hard

    bool AUTOPLAY;
    bool SANDBOX;
    bool fast_forward;          // 2x speed toggle
} game_t;

#ifdef __cplusplus
}
#endif

#endif
