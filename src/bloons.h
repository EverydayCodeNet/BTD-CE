#ifndef BLOONS_H
#define BLOONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include <stdint.h>
#include <string.h>

#include "gfx/btdbln_gfx.h"

/* ── Bloon Types ──────────────────────────────────────────────────────── */

typedef enum {
    BLOON_RED = 0,
    BLOON_BLUE,
    BLOON_GREEN,
    BLOON_YELLOW,
    BLOON_PINK,
    BLOON_BLACK,
    BLOON_WHITE,
    BLOON_LEAD,
    BLOON_ZEBRA,
    BLOON_RAINBOW,
    BLOON_CERAMIC,
    BLOON_MOAB,
    NUM_BLOON_TYPES
} bloon_type_t;

/* ── Damage Types (projectile property) ──────────────────────────────── */

typedef enum {
    DMG_SHARP     = 0x01,
    DMG_EXPLOSION = 0x02,
    DMG_FREEZE    = 0x04,
    DMG_NORMAL    = 0x08,
    DMG_ENERGY    = 0x10,
} damage_type_t;

/* ── Immunity Flags (bloon property) ─────────────────────────────────── */

typedef enum {
    IMMUNE_SHARP     = 0x01,
    IMMUNE_EXPLOSION = 0x02,
    IMMUNE_FREEZE    = 0x04,
} immunity_t;

/* ── Bloon Modifiers ─────────────────────────────────────────────────── */

typedef enum {
    MOD_NONE   = 0x00,
    MOD_CAMO   = 0x01,
    MOD_REGROW = 0x02,
} bloon_modifier_t;

#define NUM_ROUNDS      80
#define REGROW_INTERVAL 90   /* frames between regrow ticks (~3s at 30fps) */

/* ── Bloon Data Table ────────────────────────────────────────────────── */

typedef struct {
    uint8_t  hp;
    uint16_t speed_fp;      /* fixed-point speed (x256) */
    uint8_t  child_type;    /* bloon_type_t or 0xFF = none */
    uint8_t  child_count;
    uint8_t  child_type2;   /* second child type (0xFF = none) */
    uint8_t  child_count2;
    uint8_t  immunities;    /* bitmask of immunity_t */
    uint16_t rbe;           /* Red Bloon Equivalent */
} bloon_data_t;

static const bloon_data_t BLOON_DATA[NUM_BLOON_TYPES] = {
    /*  HP  Speed   Child1          Cnt1  Child2          Cnt2  Immune  RBE */
    {   1,  256,    0xFF,            0,    0xFF,            0,   0x00,    1 }, /* Red     */
    {   1,  358,    BLOON_RED,       1,    0xFF,            0,   0x00,    2 }, /* Blue    */
    {   1,  461,    BLOON_BLUE,      1,    0xFF,            0,   0x00,    3 }, /* Green   */
    {   1,  819,    BLOON_GREEN,     1,    0xFF,            0,   0x00,    4 }, /* Yellow  */
    {   1,  896,    BLOON_YELLOW,    1,    0xFF,            0,   0x00,    5 }, /* Pink    */
    {   1,  461,    BLOON_PINK,      2,    0xFF,            0,   0x02,   11 }, /* Black   */
    {   1,  512,    BLOON_PINK,      2,    0xFF,            0,   0x04,   11 }, /* White   */
    {   1,  256,    BLOON_BLACK,     2,    0xFF,            0,   0x01,   23 }, /* Lead    */
    {   1,  461,    BLOON_BLACK,     1,    BLOON_WHITE,     1,   0x06,   23 }, /* Zebra   */
    {   1,  563,    BLOON_ZEBRA,     2,    0xFF,            0,   0x00,   47 }, /* Rainbow */
    {  10,  640,    BLOON_RAINBOW,   2,    0xFF,            0,   0x00,  104 }, /* Ceramic */
    { 200,  256,    BLOON_CERAMIC,   4,    0xFF,            0,   0x00,  616 }, /* MOAB    */
};

/* ── Bloon Sprite Lookup ─────────────────────────────────────────────── */

extern gfx_sprite_t* bloon_sprite_table[NUM_BLOON_TYPES];

static inline void init_bloon_sprites(void) {
    bloon_sprite_table[BLOON_RED]     = red_base;
    bloon_sprite_table[BLOON_BLUE]    = blue;
    bloon_sprite_table[BLOON_GREEN]   = green;
    bloon_sprite_table[BLOON_YELLOW]  = yellow;
    bloon_sprite_table[BLOON_PINK]    = pink;
    bloon_sprite_table[BLOON_BLACK]   = black;
    bloon_sprite_table[BLOON_WHITE]   = white;
    bloon_sprite_table[BLOON_LEAD]    = lead_base1;
    bloon_sprite_table[BLOON_ZEBRA]   = zebra;
    bloon_sprite_table[BLOON_RAINBOW] = rainbow;
    bloon_sprite_table[BLOON_CERAMIC] = ceramic_normal;
    bloon_sprite_table[BLOON_MOAB]    = moab_undamaged;
}

/* ── Round Data ──────────────────────────────────────────────────────── */

typedef struct {
    uint8_t  bloon_type;   /* bloon_type_t */
    uint8_t  modifiers;    /* MOD_CAMO | MOD_REGROW bitmask */
    uint16_t count;        /* how many of this bloon type */
    uint8_t  spacing;      /* frames between spawns of this group */
} round_group_t;

typedef struct {
    const round_group_t *groups;
    uint8_t num_groups;
} round_def_t;

/* ── Rounds 1-80 ─────────────────────────────────────────────────────── */

static const round_group_t R1[] = {
    { BLOON_RED, 0, 20, 25 },
};

static const round_group_t R2[] = {
    { BLOON_RED, 0, 30, 20 },
};

static const round_group_t R3[] = {
    { BLOON_RED, 0, 25, 15 },
    { BLOON_BLUE, 0, 5, 25 },
};

static const round_group_t R4[] = {
    { BLOON_RED, 0, 30, 15 },
    { BLOON_BLUE, 0, 15, 20 },
};

static const round_group_t R5[] = {
    { BLOON_RED, 0, 5, 15 },
    { BLOON_BLUE, 0, 27, 15 },
};

static const round_group_t R6[] = {
    { BLOON_RED, 0, 15, 10 },
    { BLOON_BLUE, 0, 15, 15 },
    { BLOON_GREEN, 0, 4, 25 },
};

static const round_group_t R7[] = {
    { BLOON_RED, 0, 20, 10 },
    { BLOON_BLUE, 0, 20, 12 },
    { BLOON_GREEN, 0, 5, 20 },
};

static const round_group_t R8[] = {
    { BLOON_RED, 0, 10, 10 },
    { BLOON_BLUE, 0, 20, 12 },
    { BLOON_GREEN, 0, 14, 15 },
};

static const round_group_t R9[] = {
    { BLOON_GREEN, 0, 30, 10 },
};

static const round_group_t R10[] = {
    { BLOON_BLUE, 0, 20, 8 },
    { BLOON_GREEN, 0, 10, 12 },
    { BLOON_YELLOW, 0, 2, 30 },
};

static const round_group_t R11[] = {
    { BLOON_BLUE, 0, 10, 10 },
    { BLOON_GREEN, 0, 12, 12 },
    { BLOON_YELLOW, 0, 8, 18 },
};

static const round_group_t R12[] = {
    { BLOON_BLUE, 0, 15, 8 },
    { BLOON_GREEN, 0, 15, 10 },
    { BLOON_YELLOW, 0, 5, 15 },
    { BLOON_PINK, 0, 2, 20 },
};

static const round_group_t R13[] = {
    { BLOON_BLUE, 0, 30, 5 },
    { BLOON_GREEN, 0, 10, 10 },
    { BLOON_YELLOW, 0, 8, 12 },
    { BLOON_PINK, 0, 5, 18 },
};

static const round_group_t R14[] = {
    { BLOON_RED, 0, 30, 5 },
    { BLOON_BLUE, 0, 20, 5 },
    { BLOON_GREEN, 0, 15, 8 },
    { BLOON_YELLOW, 0, 10, 10 },
    { BLOON_PINK, 0, 5, 12 },
};

static const round_group_t R15[] = {
    { BLOON_RED, 0, 20, 5 },
    { BLOON_BLUE, 0, 15, 5 },
    { BLOON_GREEN, 0, 12, 8 },
    { BLOON_YELLOW, 0, 10, 10 },
    { BLOON_PINK, 0, 10, 12 },
};

static const round_group_t R16[] = {
    { BLOON_GREEN, 0, 20, 5 },
    { BLOON_YELLOW, 0, 15, 8 },
    { BLOON_PINK, 0, 12, 10 },
};

static const round_group_t R17[] = {
    { BLOON_YELLOW, 0, 25, 6 },
    { BLOON_PINK, 0, 8, 10 },
};

static const round_group_t R18[] = {
    { BLOON_GREEN, 0, 30, 5 },
    { BLOON_YELLOW, 0, 10, 8 },
    { BLOON_PINK, 0, 8, 10 },
};

static const round_group_t R19[] = {
    { BLOON_GREEN, 0, 20, 5 },
    { BLOON_YELLOW, 0, 15, 6 },
    { BLOON_PINK, 0, 12, 8 },
};

static const round_group_t R20[] = {
    { BLOON_BLACK, 0, 6, 15 },
};

static const round_group_t R21[] = {
    { BLOON_YELLOW, 0, 20, 5 },
    { BLOON_PINK, 0, 15, 8 },
    { BLOON_BLACK, 0, 8, 12 },
};

static const round_group_t R22[] = {
    { BLOON_WHITE, 0, 8, 12 },
    { BLOON_BLACK, 0, 8, 12 },
};

static const round_group_t R23[] = {
    { BLOON_YELLOW, 0, 15, 5 },
    { BLOON_WHITE, 0, 10, 10 },
    { BLOON_BLACK, 0, 10, 10 },
};

static const round_group_t R24[] = {
    { BLOON_GREEN, MOD_CAMO, 20, 8 },
    { BLOON_PINK, 0, 15, 8 },
    { BLOON_BLACK, 0, 5, 15 },
    { BLOON_WHITE, 0, 5, 15 },
};

static const round_group_t R25[] = {
    { BLOON_YELLOW, MOD_REGROW, 25, 5 },
    { BLOON_BLACK, 0, 10, 10 },
    { BLOON_WHITE, 0, 10, 10 },
};

static const round_group_t R26[] = {
    { BLOON_PINK, 0, 30, 4 },
    { BLOON_BLACK, 0, 10, 8 },
    { BLOON_WHITE, 0, 6, 10 },
    { BLOON_ZEBRA, 0, 4, 18 },
};

static const round_group_t R27[] = {
    { BLOON_YELLOW, 0, 25, 4 },
    { BLOON_BLACK, 0, 12, 8 },
    { BLOON_WHITE, 0, 12, 8 },
    { BLOON_LEAD, 0, 3, 30 },
};

static const round_group_t R28[] = {
    { BLOON_LEAD, 0, 4, 25 },
    { BLOON_BLACK, 0, 10, 10 },
    { BLOON_ZEBRA, 0, 5, 15 },
};

static const round_group_t R29[] = {
    { BLOON_PINK, 0, 18, 5 },
    { BLOON_BLACK, 0, 8, 8 },
    { BLOON_WHITE, 0, 8, 8 },
    { BLOON_ZEBRA, 0, 4, 15 },
    { BLOON_RAINBOW, 0, 2, 30 },
};

static const round_group_t R30[] = {
    { BLOON_LEAD, 0, 5, 20 },
    { BLOON_ZEBRA, 0, 6, 12 },
    { BLOON_RAINBOW, 0, 3, 20 },
};

static const round_group_t R31[] = {
    { BLOON_PINK, MOD_CAMO, 12, 8 },
    { BLOON_BLACK, 0, 8, 8 },
    { BLOON_WHITE, 0, 8, 8 },
    { BLOON_ZEBRA, 0, 5, 12 },
    { BLOON_RAINBOW, 0, 3, 18 },
};

static const round_group_t R32[] = {
    { BLOON_YELLOW, MOD_REGROW, 15, 5 },
    { BLOON_ZEBRA, 0, 6, 12 },
    { BLOON_RAINBOW, 0, 4, 15 },
};

static const round_group_t R33[] = {
    { BLOON_BLACK, MOD_REGROW, 8, 8 },
    { BLOON_WHITE, MOD_REGROW, 8, 8 },
    { BLOON_RAINBOW, 0, 5, 12 },
};

static const round_group_t R34[] = {
    { BLOON_ZEBRA, 0, 10, 8 },
    { BLOON_RAINBOW, 0, 6, 12 },
    { BLOON_LEAD, 0, 3, 20 },
};

static const round_group_t R35[] = {
    { BLOON_BLACK, MOD_CAMO | MOD_REGROW, 6, 10 },
    { BLOON_PINK, 0, 18, 5 },
    { BLOON_RAINBOW, 0, 6, 10 },
};

static const round_group_t R36[] = {
    { BLOON_PINK, 0, 25, 3 },
    { BLOON_BLACK, 0, 10, 8 },
    { BLOON_RAINBOW, 0, 6, 10 },
    { BLOON_LEAD, MOD_CAMO, 2, 30 },
};

static const round_group_t R37[] = {
    { BLOON_ZEBRA, MOD_REGROW, 8, 10 },
    { BLOON_RAINBOW, 0, 6, 10 },
    { BLOON_CERAMIC, 0, 2, 45 },
};

static const round_group_t R38[] = {
    { BLOON_RAINBOW, 0, 8, 10 },
    { BLOON_CERAMIC, 0, 3, 35 },
    { BLOON_WHITE, MOD_REGROW, 10, 8 },
};

static const round_group_t R39[] = {
    { BLOON_BLACK, MOD_REGROW, 15, 5 },
    { BLOON_RAINBOW, 0, 8, 8 },
    { BLOON_CERAMIC, 0, 3, 30 },
};

static const round_group_t R40[] = {
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_CERAMIC, 0, 3, 20 },
    { BLOON_RAINBOW, MOD_REGROW, 4, 12 },
};

static const round_group_t R41[] = {
    { BLOON_CERAMIC, MOD_REGROW, 4, 20 },
    { BLOON_RAINBOW, 0, 10, 8 },
    { BLOON_ZEBRA, MOD_CAMO, 8, 8 },
};

static const round_group_t R42[] = {
    { BLOON_RAINBOW, MOD_REGROW, 8, 8 },
    { BLOON_CERAMIC, 0, 4, 18 },
    { BLOON_BLACK, MOD_CAMO, 10, 8 },
};

static const round_group_t R43[] = {
    { BLOON_CERAMIC, 0, 6, 15 },
    { BLOON_LEAD, 0, 5, 15 },
    { BLOON_RAINBOW, MOD_CAMO, 5, 10 },
};

static const round_group_t R44[] = {
    { BLOON_CERAMIC, MOD_REGROW, 5, 15 },
    { BLOON_RAINBOW, 0, 12, 6 },
};

static const round_group_t R45[] = {
    { BLOON_CERAMIC, MOD_CAMO, 4, 18 },
    { BLOON_CERAMIC, 0, 6, 15 },
    { BLOON_PINK, MOD_CAMO | MOD_REGROW, 15, 5 },
};

static const round_group_t R46[] = {
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_CERAMIC, 0, 5, 15 },
};

static const round_group_t R47[] = {
    { BLOON_CERAMIC, MOD_REGROW, 8, 12 },
    { BLOON_RAINBOW, MOD_CAMO, 8, 10 },
    { BLOON_LEAD, MOD_CAMO, 5, 15 },
};

static const round_group_t R48[] = {
    { BLOON_CERAMIC, 0, 8, 12 },
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_RAINBOW, MOD_REGROW, 8, 8 },
};

static const round_group_t R49[] = {
    { BLOON_CERAMIC, MOD_REGROW, 10, 10 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 4, 18 },
    { BLOON_RAINBOW, 0, 12, 6 },
};

static const round_group_t R50[] = {
    { BLOON_MOAB, 0, 2, 60 },
    { BLOON_CERAMIC, MOD_CAMO, 6, 12 },
};

static const round_group_t R51[] = {
    { BLOON_CERAMIC, 0, 12, 8 },
    { BLOON_RAINBOW, MOD_REGROW, 10, 8 },
    { BLOON_LEAD, MOD_CAMO, 5, 15 },
};

static const round_group_t R52[] = {
    { BLOON_CERAMIC, MOD_CAMO, 6, 12 },
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 6, 10 },
};

static const round_group_t R53[] = {
    { BLOON_CERAMIC, MOD_REGROW, 8, 10 },
    { BLOON_CERAMIC, MOD_CAMO, 6, 12 },
    { BLOON_LEAD, MOD_CAMO, 6, 12 },
};

static const round_group_t R54[] = {
    { BLOON_MOAB, 0, 2, 50 },
    { BLOON_CERAMIC, 0, 8, 10 },
};

static const round_group_t R55[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 8, 10 },
    { BLOON_RAINBOW, 0, 15, 5 },
    { BLOON_MOAB, 0, 1, 60 },
};

static const round_group_t R56[] = {
    { BLOON_MOAB, 0, 2, 45 },
    { BLOON_CERAMIC, MOD_REGROW, 8, 10 },
    { BLOON_LEAD, MOD_CAMO, 4, 18 },
};

static const round_group_t R57[] = {
    { BLOON_CERAMIC, MOD_CAMO, 10, 8 },
    { BLOON_RAINBOW, MOD_REGROW, 12, 6 },
    { BLOON_MOAB, 0, 1, 60 },
};

static const round_group_t R58[] = {
    { BLOON_MOAB, 0, 2, 40 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 6, 12 },
    { BLOON_LEAD, MOD_CAMO, 5, 15 },
};

static const round_group_t R59[] = {
    { BLOON_CERAMIC, 0, 12, 6 },
    { BLOON_CERAMIC, MOD_REGROW, 8, 8 },
    { BLOON_MOAB, 0, 2, 40 },
};

static const round_group_t R60[] = {
    { BLOON_MOAB, 0, 3, 35 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 8, 10 },
};

static const round_group_t R61[] = {
    { BLOON_MOAB, 0, 2, 40 },
    { BLOON_CERAMIC, MOD_CAMO, 12, 8 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 8, 8 },
};

static const round_group_t R62[] = {
    { BLOON_MOAB, 0, 2, 35 },
    { BLOON_CERAMIC, MOD_REGROW, 10, 8 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 5, 15 },
};

static const round_group_t R63[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 15, 6 },
    { BLOON_LEAD, MOD_CAMO, 8, 10 },
    { BLOON_MOAB, 0, 2, 40 },
};

static const round_group_t R64[] = {
    { BLOON_MOAB, 0, 3, 30 },
    { BLOON_CERAMIC, 0, 12, 6 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 8, 8 },
};

static const round_group_t R65[] = {
    { BLOON_MOAB, 0, 3, 28 },
    { BLOON_CERAMIC, MOD_CAMO, 8, 8 },
};

static const round_group_t R66[] = {
    { BLOON_CERAMIC, MOD_REGROW, 15, 5 },
    { BLOON_MOAB, 0, 2, 35 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 6, 12 },
};

static const round_group_t R67[] = {
    { BLOON_MOAB, 0, 3, 28 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 10, 8 },
};

static const round_group_t R68[] = {
    { BLOON_MOAB, 0, 3, 25 },
    { BLOON_CERAMIC, MOD_CAMO, 10, 8 },
    { BLOON_LEAD, MOD_CAMO, 6, 12 },
};

static const round_group_t R69[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 15, 5 },
    { BLOON_MOAB, 0, 3, 28 },
};

static const round_group_t R70[] = {
    { BLOON_MOAB, 0, 3, 25 },
    { BLOON_CERAMIC, MOD_REGROW, 12, 6 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 5, 15 },
};

static const round_group_t R71[] = {
    { BLOON_MOAB, 0, 3, 22 },
    { BLOON_CERAMIC, MOD_CAMO, 12, 6 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 10, 6 },
};

static const round_group_t R72[] = {
    { BLOON_MOAB, 0, 3, 22 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 12, 6 },
};

static const round_group_t R73[] = {
    { BLOON_MOAB, 0, 4, 20 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 8, 10 },
    { BLOON_CERAMIC, 0, 12, 5 },
};

static const round_group_t R74[] = {
    { BLOON_MOAB, 0, 4, 20 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 12, 5 },
    { BLOON_RAINBOW, MOD_REGROW, 10, 6 },
};

static const round_group_t R75[] = {
    { BLOON_MOAB, 0, 4, 18 },
    { BLOON_CERAMIC, MOD_CAMO, 10, 6 },
};

static const round_group_t R76[] = {
    { BLOON_MOAB, 0, 4, 18 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 12, 5 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 6, 12 },
};

static const round_group_t R77[] = {
    { BLOON_MOAB, 0, 5, 16 },
    { BLOON_CERAMIC, MOD_REGROW, 15, 5 },
};

static const round_group_t R78[] = {
    { BLOON_MOAB, 0, 5, 16 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 12, 5 },
    { BLOON_LEAD, MOD_CAMO, 8, 10 },
};

static const round_group_t R79[] = {
    { BLOON_MOAB, 0, 5, 15 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 15, 5 },
};

static const round_group_t R80[] = {
    { BLOON_MOAB, 0, 6, 14 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 12, 6 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 6, 10 },
};

static const round_def_t ROUND_DEFS[NUM_ROUNDS] = {
    { R1,  1 }, { R2,  1 }, { R3,  2 }, { R4,  2 }, { R5,  2 },
    { R6,  3 }, { R7,  3 }, { R8,  3 }, { R9,  1 }, { R10, 3 },
    { R11, 3 }, { R12, 4 }, { R13, 4 }, { R14, 5 }, { R15, 5 },
    { R16, 3 }, { R17, 2 }, { R18, 3 }, { R19, 3 }, { R20, 1 },
    { R21, 3 }, { R22, 2 }, { R23, 3 }, { R24, 4 }, { R25, 3 },
    { R26, 4 }, { R27, 4 }, { R28, 3 }, { R29, 5 }, { R30, 3 },
    { R31, 5 }, { R32, 3 }, { R33, 3 }, { R34, 3 }, { R35, 3 },
    { R36, 4 }, { R37, 3 }, { R38, 3 }, { R39, 3 }, { R40, 3 },
    { R41, 3 }, { R42, 3 }, { R43, 3 }, { R44, 2 }, { R45, 3 },
    { R46, 2 }, { R47, 3 }, { R48, 3 }, { R49, 3 }, { R50, 2 },
    { R51, 3 }, { R52, 3 }, { R53, 3 }, { R54, 2 }, { R55, 3 },
    { R56, 3 }, { R57, 3 }, { R58, 3 }, { R59, 3 }, { R60, 2 },
    { R61, 3 }, { R62, 3 }, { R63, 3 }, { R64, 3 }, { R65, 2 },
    { R66, 3 }, { R67, 2 }, { R68, 3 }, { R69, 2 }, { R70, 3 },
    { R71, 3 }, { R72, 2 }, { R73, 3 }, { R74, 3 }, { R75, 2 },
    { R76, 3 }, { R77, 2 }, { R78, 3 }, { R79, 2 }, { R80, 3 },
};

#ifdef __cplusplus
}
#endif

#endif
