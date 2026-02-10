#ifndef TOWERS_H
#define TOWERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include <stdint.h>

#include "gfx/btdtw1_gfx.h"
#include "gfx/btdtw2_gfx.h"
#include "gfx/btdui_gfx.h"
#include "bloons.h"

/* ── Tower Types ─────────────────────────────────────────────────────── */

typedef enum {
    TOWER_DART = 0,
    TOWER_TACK,
    TOWER_SNIPER,
    TOWER_BOMB,
    TOWER_BOOMERANG,
    TOWER_NINJA,
    TOWER_ICE,
    TOWER_GLUE,
    NUM_TOWER_TYPES
} tower_type_t;

/* ── Base Tower Data ─────────────────────────────────────────────────── */

typedef struct {
    uint16_t cost;
    uint8_t  atk_frames;      /* frames between attacks */
    uint8_t  range;            /* pixels */
    uint8_t  damage;
    uint8_t  pierce;
    uint8_t  damage_type;      /* damage_type_t bitmask */
    uint8_t  can_see_camo;
    uint8_t  projectile_count; /* tack fires 8 */
    uint8_t  projectile_speed; /* 0 = hitscan or area */
    uint8_t  is_hitscan;
    uint8_t  is_area;
} tower_data_t;

static const tower_data_t TOWER_DATA[NUM_TOWER_TYPES] = {
    /*           Cost  Atk  Rng  Dmg  Prc  DmgType        Camo #Prj Spd  Hit  Area */
    /* Dart  */ { 200,  21,  40,   1,   2,  DMG_SHARP,       0,  1,   5,   0,   0 },
    /* Tack  */ { 280,  20,  28,   1,   1,  DMG_SHARP,       0,  8,   4,   0,   0 },
    /* Snipe */ { 350,  48, 255,   2,   1,  DMG_SHARP,       0,  1,   0,   1,   0 },
    /* Bomb  */ { 525,  24,  40,   1,  18,  DMG_EXPLOSION,   0,  1,   3,   0,   0 },
    /* Boom  */ { 325,  21,  40,   1,   4,  DMG_SHARP,       0,  1,   4,   0,   0 },
    /* Ninja */ { 500,  17,  40,   1,   2,  DMG_SHARP,       1,  1,   6,   0,   0 },
    /* Ice   */ { 500,  90,  30,   0,  40,  DMG_FREEZE,      0,  0,   0,   0,   1 },
    /* Glue  */ { 275,  24,  38,   0,   1,  DMG_NORMAL,      0,  1,   4,   0,   0 },
};

/* ── Tower Names ─────────────────────────────────────────────────────── */

static const char* const TOWER_NAMES[NUM_TOWER_TYPES] = {
    "Dart", "Tack", "Sniper", "Bomb",
    "Boomerang", "Ninja", "Ice", "Glue"
};

/* ── Upgrade Data ────────────────────────────────────────────────────── */

typedef struct {
    int8_t   delta_damage;
    int8_t   delta_pierce;
    int8_t   delta_range;
    int8_t   delta_atk_pct;        /* negative = faster; -20 means 20% faster */
    int8_t   delta_proj_count;
    uint8_t  grants_camo;
    uint8_t  damage_type_override; /* 0 = no change */
    uint16_t cost;
} upgrade_t;

/* TOWER_UPGRADES[tower_type][path][level] */
static const upgrade_t TOWER_UPGRADES[NUM_TOWER_TYPES][2][4] = {
    /* ── Dart Monkey ─────────────────────────────────────────────────── */
    [TOWER_DART] = {
        /* Path 0: Long Range Darts -> Enhanced Eyesight -> Spike-o-pult -> Juggernaut */
        {
            { 0,  0,  12,   0,  0, 0, 0,          90 },  /* Long Range Darts */
            { 0,  0,   8,   0,  0, 1, 0,         120 },  /* Enhanced Eyesight */
            { 1,  4,   0,   0,  0, 0, DMG_NORMAL, 500 },  /* Spike-o-pult */
            { 3,  8,   0,   0,  0, 0, 0,        1800 },  /* Juggernaut */
        },
        /* Path 1: Sharp Shots -> Razor Sharp -> Triple Shot -> Super Monkey Fan Club */
        {
            { 0,  1,   0,   0,  0, 0, 0,         140 },  /* Sharp Shots */
            { 0,  2,   0,   0,  0, 0, 0,         200 },  /* Razor Sharp */
            { 0,  0,   0,   0,  2, 0, 0,         400 },  /* Triple Shot */
            { 0,  0,   0, -50,  0, 0, 0,        8000 },  /* Super Monkey FC */
        },
    },
    /* ── Tack Shooter ────────────────────────────────────────────────── */
    [TOWER_TACK] = {
        /* Path 0: Faster Shooting -> Even Faster -> Hot Shots -> Ring of Fire */
        {
            { 0,  0,   0, -15,  0, 0, 0,         210 },
            { 0,  0,   0, -15,  0, 0, 0,         300 },
            { 1,  0,   0,   0,  0, 0, DMG_NORMAL, 550 },  /* Hot Shots */
            { 2,  0,   4,   0,  0, 0, 0,        2500 },  /* Ring of Fire */
        },
        /* Path 1: Extra Range -> Extra Spread -> Blade Shooter -> Blade Maelstrom */
        {
            { 0,  0,   6,   0,  0, 0, 0,         100 },
            { 0,  0,   0,   0,  4, 0, 0,         250 },  /* +4 proj = 12 total */
            { 1,  1,   0,   0,  0, 0, 0,         500 },  /* Blade Shooter */
            { 1,  2,   0, -25,  0, 0, 0,        2800 },  /* Blade Maelstrom */
        },
    },
    /* ── Sniper Monkey ───────────────────────────────────────────────── */
    [TOWER_SNIPER] = {
        /* Path 0: Full Metal Jacket -> Point Five Oh -> Deadly Precision -> Cripple MOAB */
        {
            { 2,  0,   0,   0,  0, 0, DMG_NORMAL, 350 },  /* FMJ: pops Lead */
            { 3,  0,   0,   0,  0, 0, 0,         500 },
            { 10, 0,   0,   0,  0, 0, 0,        3000 },
            { 30, 0,   0,   0,  0, 0, 0,       12000 },
        },
        /* Path 1: Faster Firing -> Night Vision -> Semi-Auto -> Full Auto */
        {
            { 0,  0,   0, -30,  0, 0, 0,         300 },
            { 0,  0,   0,   0,  0, 1, 0,         350 },  /* Night Vision: camo */
            { 0,  0,   0, -30,  0, 0, 0,        3500 },
            { 0,  0,   0, -40,  0, 0, 0,        8000 },
        },
    },
    /* ── Bomb Tower ──────────────────────────────────────────────────── */
    [TOWER_BOMB] = {
        /* Path 0: Bigger Bombs -> Missile Launcher -> MOAB Mauler -> MOAB Assassin */
        {
            { 0,  8,   4,   0,  0, 0, 0,         400 },
            { 0,  6,   0, -20,  0, 0, 0,         500 },
            { 8,  0,   0,   0,  0, 0, 0,         800 },
            { 20, 0,   0, -15,  0, 0, 0,        3200 },
        },
        /* Path 1: Frag Bombs -> Cluster Bombs -> Bloon Impact -> MOAB Elim */
        {
            { 1,  2,   0,   0,  0, 0, DMG_NORMAL, 300 },  /* Frag: pops Black */
            { 0,  4,   0,   0,  0, 0, 0,         600 },
            { 1,  0,   0,   0,  0, 0, 0,        2500 },
            { 15, 0,   0,   0,  0, 0, 0,       10000 },
        },
    },
    /* ── Boomerang Thrower ───────────────────────────────────────────── */
    [TOWER_BOOMERANG] = {
        /* Path 0: Multi-Target -> Glaive Thrower -> Glaive Ricochet -> Glaive Lord */
        {
            { 0,  3,   0,   0,  0, 0, 0,         200 },
            { 0,  2,   6,   0,  0, 0, 0,         350 },
            { 1,  6,   0, -15,  0, 0, 0,        1600 },
            { 3,  8,   0, -20,  0, 0, 0,        5000 },
        },
        /* Path 1: Sonic Boom -> Red Hot Rangs -> Bionic Boomer -> Turbo Charge */
        {
            { 0,  0,   0,   0,  0, 0, DMG_NORMAL, 250 },  /* Sonic: pops Lead/Frozen */
            { 1,  0,   0,   0,  0, 0, 0,         300 },
            { 0,  0,   0, -35,  0, 0, 0,        1600 },
            { 2,  2,   0, -30,  0, 0, 0,        3200 },
        },
    },
    /* ── Ninja Monkey ────────────────────────────────────────────────── */
    [TOWER_NINJA] = {
        /* Path 0: Ninja Discipline -> Sharp Shurikens -> Double Shot -> Bloonjitsu */
        {
            { 0,  0,   8, -10,  0, 0, 0,         300 },
            { 0,  2,   0,   0,  0, 0, 0,         350 },
            { 0,  0,   0,   0,  1, 0, 0,         750 },
            { 1,  2,   0,   0,  2, 0, 0,        2750 },
        },
        /* Path 1: Seeking Shuriken -> Distraction -> Counter-Espionage -> Sabotage Supply Lines */
        {
            { 0,  0,   0,   0,  0, 0, 0,         250 },  /* homing (no stat delta) */
            { 0,  0,   0,   0,  0, 0, 0,         350 },  /* distraction (no stat delta) */
            { 0,  0,   0,   0,  0, 0, 0,         700 },  /* Counter-Esp (no stat delta) */
            { 0,  0,   0,   0,  0, 0, 0,        5000 },  /* Sabotage (ability, no stat) */
        },
    },
    /* ── Ice Tower ───────────────────────────────────────────────────── */
    [TOWER_ICE] = {
        /* Path 0: Enhanced Freeze -> Snap Freeze -> Arctic Wind -> Viral Frost */
        {
            { 0,  0,   0, -15,  0, 0, 0,         200 },
            { 1,  0,   0,   0,  0, 0, 0,         350 },  /* 1 dmg on freeze */
            { 0, 20,  10,   0,  0, 0, 0,        1800 },
            { 0,  0,   0,   0,  0, 0, 0,        2500 },  /* viral (ability) */
        },
        /* Path 1: Permafrost -> Cold Snap -> Ice Shards -> Absolute Zero */
        {
            { 0,  0,   0,   0,  0, 0, 0,         100 },  /* longer freeze (no stat) */
            { 0,  0,   8,   0,  0, 0, 0,         225 },
            { 0,  0,   0,   0,  0, 0, 0,        1500 },  /* ice shards (ability) */
            { 0, 40,   0, -20,  0, 0, 0,        3500 },
        },
    },
    /* ── Glue Gunner ─────────────────────────────────────────────────── */
    [TOWER_GLUE] = {
        /* Path 0: Glue Soak -> Corrosive Glue -> Bloon Dissolver -> Bloon Liquifier */
        {
            { 0,  0,   0,   0,  0, 0, 0,         200 },  /* soak (all layers) */
            { 1,  0,   0,   0,  0, 0, 0,         300 },  /* corrosive: 1 dmg/3s */
            { 2,  0,   0,   0,  0, 0, 0,        2500 },  /* dissolve */
            { 4,  0,   0,   0,  0, 0, 0,        5000 },  /* liquify */
        },
        /* Path 1: Stickier Glue -> Glue Splatter -> Glue Hose -> Glue Striker */
        {
            { 0,  0,   0,   0,  0, 0, 0,         120 },  /* longer slow */
            { 0,  2,   0,   0,  0, 0, 0,         400 },  /* splatter: +2 pierce */
            { 0,  0,   0, -50,  0, 0, 0,        3000 },  /* hose: much faster */
            { 0,  3,   6, -20,  0, 0, 0,        4500 },  /* striker */
        },
    },
};

/* ── Upgrade Names ───────────────────────────────────────────────────── */

static const char* const UPGRADE_NAMES[NUM_TOWER_TYPES][2][4] = {
    [TOWER_DART] = {
        { "Long Range", "Enh. Sight", "Spike-o-pult", "Juggernaut" },
        { "Sharp Shots", "Razor Sharp", "Triple Shot", "SM Fan Club" },
    },
    [TOWER_TACK] = {
        { "Fast Shoot", "Even Faster", "Hot Shots", "Ring o Fire" },
        { "Extra Range", "Extra Spread", "Blade Shoot", "Blade Mael" },
    },
    [TOWER_SNIPER] = {
        { "Full Metal", "Point Five", "Deadly Prec", "Cripple" },
        { "Fast Fire", "Night Vis.", "Semi-Auto", "Full Auto" },
    },
    [TOWER_BOMB] = {
        { "Bigger Bomb", "Missile", "MOAB Maul", "MOAB Assn" },
        { "Frag Bombs", "Cluster", "Bloon Impct", "MOAB Elim" },
    },
    [TOWER_BOOMERANG] = {
        { "Multi-Tgt", "Glaive Thr", "Glv Ricoch", "Glaive Lord" },
        { "Sonic Boom", "Red Hot", "Bionic Boom", "Turbo Chrg" },
    },
    [TOWER_NINJA] = {
        { "Discipline", "Sharp Shur", "Double Shot", "Bloonjitsu" },
        { "Seeking", "Distract", "Counter-Esp", "Sabotage" },
    },
    [TOWER_ICE] = {
        { "Enh. Freeze", "Snap Freeze", "Arctic Wind", "Viral Frost" },
        { "Permafrost", "Cold Snap", "Ice Shards", "Abs. Zero" },
    },
    [TOWER_GLUE] = {
        { "Glue Soak", "Corrosive", "Dissolver", "Liquifier" },
        { "Stickier", "Splatter", "Glue Hose", "Glue Strike" },
    },
};

/* ── Tower Sprite Table ──────────────────────────────────────────────── */

extern gfx_sprite_t* tower_sprite_table[NUM_TOWER_TYPES];
extern gfx_sprite_t* tower_projectile_table[NUM_TOWER_TYPES];

static inline void init_tower_sprites(void) {
    tower_sprite_table[TOWER_DART]      = dart1;
    tower_sprite_table[TOWER_TACK]      = tack1;
    tower_sprite_table[TOWER_SNIPER]    = sniper1;
    tower_sprite_table[TOWER_BOMB]      = bomber1;
    tower_sprite_table[TOWER_BOOMERANG] = boomerang1;
    tower_sprite_table[TOWER_NINJA]     = ninja1;
    tower_sprite_table[TOWER_ICE]       = ice1;
    tower_sprite_table[TOWER_GLUE]      = glue1;

    tower_projectile_table[TOWER_DART]      = big_dart;
    tower_projectile_table[TOWER_TACK]      = tack;
    tower_projectile_table[TOWER_SNIPER]    = NULL;  /* hitscan */
    tower_projectile_table[TOWER_BOMB]      = bomb_small;
    tower_projectile_table[TOWER_BOOMERANG] = wood_rang_right;
    tower_projectile_table[TOWER_NINJA]     = ninja_star1;
    tower_projectile_table[TOWER_ICE]       = NULL;  /* area effect */
    tower_projectile_table[TOWER_GLUE]      = NULL;  /* glue blob (drawn as circle) */
}

#ifdef __cplusplus
}
#endif

#endif
