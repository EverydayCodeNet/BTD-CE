#ifndef FREEPLAY_H
#define FREEPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bloons.h"

#define FREEPLAY_MAX_GROUPS 6

static round_group_t freeplay_groups[FREEPLAY_MAX_GROUPS];
static uint8_t freeplay_num_groups;

/* Generate a freeplay round for rounds >= 80 */
static void generate_freeplay_round(uint16_t round) {
    uint16_t offset = round - 80;
    uint16_t scale = 100 + offset * 8;  /* 108%, 116%, ... */
    uint8_t gi = 0;

    /* Group 1: MOABs */
    {
        uint16_t moab_count = (8 * scale) / 100;
        if (moab_count > 255) moab_count = 255;
        uint8_t spacing = (offset / 3 < 8) ? (uint8_t)(10 - offset / 3) : 2;
        freeplay_groups[gi].bloon_type = BLOON_MOAB;
        freeplay_groups[gi].modifiers = 0;
        freeplay_groups[gi].count = (uint16_t)moab_count;
        freeplay_groups[gi].spacing = spacing;
        gi++;
    }

    /* Group 2: Ceramics (CAMO+REGROW) */
    {
        uint16_t count = (30 * scale) / 100;
        if (count > 255) count = 255;
        freeplay_groups[gi].bloon_type = BLOON_CERAMIC;
        freeplay_groups[gi].modifiers = MOD_CAMO | MOD_REGROW;
        freeplay_groups[gi].count = (uint16_t)count;
        freeplay_groups[gi].spacing = 3;
        gi++;
    }

    /* Group 3: Lead (CAMO+REGROW) after offset >= 3 */
    if (offset >= 3) {
        uint16_t count = (15 * scale) / 100;
        if (count > 255) count = 255;
        freeplay_groups[gi].bloon_type = BLOON_LEAD;
        freeplay_groups[gi].modifiers = MOD_CAMO | MOD_REGROW;
        freeplay_groups[gi].count = (uint16_t)count;
        freeplay_groups[gi].spacing = 6;
        gi++;
    }

    /* Group 4: Rainbows after offset >= 10 */
    if (offset >= 10) {
        uint16_t count = (25 * scale) / 100;
        if (count > 255) count = 255;
        freeplay_groups[gi].bloon_type = BLOON_RAINBOW;
        freeplay_groups[gi].modifiers = MOD_CAMO | MOD_REGROW;
        freeplay_groups[gi].count = (uint16_t)count;
        freeplay_groups[gi].spacing = 3;
        gi++;
    }

    /* Group 5: Zebras after offset >= 15 */
    if (offset >= 15) {
        uint16_t count = (20 * scale) / 100;
        if (count > 255) count = 255;
        freeplay_groups[gi].bloon_type = BLOON_ZEBRA;
        freeplay_groups[gi].modifiers = MOD_CAMO | MOD_REGROW;
        freeplay_groups[gi].count = (uint16_t)count;
        freeplay_groups[gi].spacing = 4;
        gi++;
    }

    freeplay_num_groups = gi;
}

/* Get round data: normal rounds for 0-79, generated for 80+ */
static const round_group_t* get_round_groups(uint16_t round, uint8_t* num_groups) {
    if (round < NUM_ROUNDS) {
        *num_groups = ROUND_DEFS[round].num_groups;
        return ROUND_DEFS[round].groups;
    }
    generate_freeplay_round(round);
    *num_groups = freeplay_num_groups;
    return freeplay_groups;
}

#ifdef __cplusplus
}
#endif

#endif
