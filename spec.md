# BTD CE -- Bloons Tower Defense for TI-84 Plus CE

## Specification Document (Single Source of Truth)

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Design Decisions](#design-decisions)
3. [Technical Architecture](#technical-architecture)
4. [Appvar Sprite System](#appvar-sprite-system)
5. [Sprite Pipeline](#sprite-pipeline)
6. [UI Layout](#ui-layout)
7. [Controls](#controls)
8. [Bloon Types and Data](#bloon-types-and-data)
9. [Tower Data](#tower-data)
10. [Round Data](#round-data)
11. [Cash System](#cash-system)
12. [Lives System](#lives-system)
13. [Map System](#map-system)
14. [Save System](#save-system)
15. [Existing Codebase Reference](#existing-codebase-reference)
16. [Next Steps](#next-steps)
17. [Reference Links](#reference-links)

---

## Project Overview

BTD CE is a Bloons Tower Defense game (inspired by BTD5/BTD6) built for the **TI-84 Plus CE** graphing calculator. The project is a fresh rewrite, willing to restructure code significantly from the existing prototype.

- **Platform**: TI-84 Plus CE (320x240 screen, 8-bit indexed color, 256 colors)
- **Language**: C (CE C SDK / CE Toolchain)
- **Build System**: `make` via `cedev-config --makefile`
- **Program Name**: `BTDCE`
- **Program Limit**: ~65KB for .8xp; sprites go into appvars to stay under this limit
- **GitHub Account**: EverydayCodeNet
- **Authors**: Everyday Code & EKB
- **Art Credit**: Derek_and_Harum1 sprite sheet (https://www.deviantart.com/derek200)

---

## Design Decisions

These decisions were established through the design/interview process and serve as binding constraints for all development.

### Towers
- All 17 tower types available in the sprite media: Dart, Tack, Super, Wizard, Engineer, Sniper, Glue, Catapult, Mortar, Ice, Banana Farm, Shredder, Gatling, Ninja, Bomber, Boomerang, and (tack pile as a variant).
- **Classic core first** (8 towers, in order of priority): Dart, Tack, Sniper, Bomb (Bomber), Boomerang, Ninja, Ice, Glue.
- **BTD5-style 2 upgrade paths** per tower (the sprite sheet has variations to support this).
- Remaining towers added after core gameplay is solid.

### Bloons
- Full standard bloon layer chain: Red through Ceramic.
- **MOAB** as the sole boss bloon type initially. No BFB, ZOMG, DDT, or BAD at launch.
- **Camo** and **Regrow** modifiers both included.
- **Skip Purple bloons** entirely (no energy-type towers to justify them).
- Fortified modifier is a future feature.

### Difficulty
- 3 difficulty modes: **Easy** (rounds 1-40), **Medium** (rounds 1-60), **Hard** (rounds 1-80).
- Selectable difficulty/round range on the menu screen (future).
- Single game speed only (no fast-forward).

### Gameplay
- **Priority**: Core gameplay loop first (spawn bloons, tower targeting, projectile firing, collision detection, pop, cash reward).
- **"First" targeting only** initially; plan architecture for all 4 modes (First, Last, Strong, Close).
- Data-driven maps that can be generated on the fly (path defined as waypoint arrays).
- **Sandbox/free play mode**: Unlimited cash for testing and fun.

### UI
- Arrow key cursor for tower placement.
- **gfx_Circle** drawn when no monkey is selected (current behavior of drawing the monkey sprite as cursor is wrong and must be fixed).
- **Right sidebar** (vertical) for 6-8 tower icons, scrollable if more towers than visible slots.
- **Hybrid UI**: Compact tower bar during gameplay; full-screen pause menu overlay for upgrades.
- Range circle shown when placing towers and when selecting/inspecting a placed tower.

### Rendering
- Use **gfx_SwapDraw()** instead of current gfx_BlitBuffer() for proper double-buffering.
- Keep the spatial partition system from the existing code.

### Save/Load
- Save/load game state to an appvar (informed by Clash Royale CE rewrite pattern).
- Save between rounds.

---

## Technical Architecture

### CE C Toolchain Libraries

| Library | Purpose |
|---------|---------|
| `graphx.h` | All rendering: sprites, primitives, text, tilemaps, rotation, clipping |
| `keypadc.h` | Keyboard input via polling (`kb_Scan`, `kb_Data[]`, `kb_IsDown`) |
| `fileioc.h` | AppVar read/write for save data and sprite loading |
| `compression.h` | ZX7/ZX0 decompression for compressed sprites |
| `sys/timers.h` | Hardware timers for frame timing (`TIMER_32K` = 32768 Hz) |
| `debug.h` | `dbg_printf` for emulator debugging (no-op in release builds) |
| `sys/rtc.h` | `rtc_Time()` for random seed generation |

### graphx.h Important Notes

- **Rotation requires square sprites**: width must equal height. Max input size is 210x210. Angle range is 0-255 (256 discrete positions around a full circle).
- **RLE sprites** (`gfx_rletsprite_t`) provide faster transparent rendering and should be used for frequently drawn sprites.
- **Tilemap system** can be used for map backgrounds.
- **Hardware cursor** (32x32 or 64x64 overlay) requires no redraw and could be used for the game cursor.
- `gfx_SwapDraw()` is preferred over `gfx_BlitBuffer()` for double-buffering.

### keypadc.h Usage Pattern

```c
kb_Scan();
if (kb_Data[7] & kb_Up)    { /* up pressed */ }
if (kb_Data[7] & kb_Down)  { /* down pressed */ }
if (kb_Data[7] & kb_Left)  { /* left pressed */ }
if (kb_Data[7] & kb_Right) { /* right pressed */ }
if (kb_Data[6] & kb_Enter) { /* enter pressed */ }
if (kb_Data[6] & kb_Clear) { /* clear pressed */ }
if (kb_Data[1] & kb_Mode)  { /* mode pressed */ }
```

### Performance Guidelines

- **Avoid floating-point math** (`sin`, `cos`, `atan2`, `sqrt`) in hot loops. Use integer math and lookup tables instead.
- Use **RLE sprites** (`gfx_rletsprite_t`) for all frequently drawn transparent sprites (bloons, towers, projectiles).
- Consider the **hardware LCD cursor** for the game cursor (avoids redraw overhead).
- Use `gfx_SwapDraw()` instead of `gfx_BlitBuffer()`.
- Program `.8xp` limit is ~65KB. Use appvars for sprites to stay well under this.
- The spatial partitioning system (20x20 pixel boxes) is critical for collision detection performance. Keep and refine it.

### Core Data Structures

The following are the key data types. The rewrite may modify these, but they represent the current architecture and intent.

```c
typedef struct {
    int16_t x;
    int16_t y;
} position_t;

typedef struct bloon_t {
    position_t position;
    uint8_t type;           // index into bloon_data[] table
    uint8_t modifiers;      // bitmask: CAMO=0x01, REGROW=0x02
    int16_t hp;             // remaining HP for this layer
    uint8_t regrow_timer;   // ticks until next regrow
    uint8_t regrow_max;     // highest type this bloon can regrow to
    uint16_t segment;       // current path segment index
    uint16_t progress;      // sub-pixel progress along segment (fixed-point)
} bloon_t;

typedef struct {
    position_t position;
    uint8_t type;           // index into tower_data[] table
    uint8_t upgrades[2];    // path 1 and path 2 upgrade levels (0-4)
    uint8_t target_mode;    // FIRST=0, LAST=1, STRONG=2, CLOSE=3
    uint16_t cooldown_timer;
    uint16_t total_invested; // for sell-back calculation
} tower_t;

typedef struct {
    position_t position;
    uint8_t type;           // determines sprite, damage type, etc.
    int16_t dx;             // x velocity (fixed-point)
    int16_t dy;             // y velocity (fixed-point)
    uint8_t pierce_remaining;
    uint8_t lifetime;       // frames until despawn
} projectile_t;

typedef struct {
    path_t *path;
    int16_t lives;
    int24_t cash;           // use 24-bit int for cash (CE toolchain supports)
    uint8_t round;
    uint8_t difficulty;     // EASY=0, MEDIUM=1, HARD=2
    tower_t towers[MAX_TOWERS];
    uint8_t num_towers;
    // ... spatial partitions for bloons and projectiles
} game_t;
```

### Game Loop Architecture

```
main()
  gfx_Begin()
  load_palette()
  load_appvar_sprites()   // BTDTW1_init(), BTDTW2_init(), BTDBLN_init(), BTDUI_init()
  gfx_SetDrawScreen()     // set up double buffering

  show_title_menu()       // difficulty select, load game, sandbox

  while (!game_over) {
      kb_Scan()
      handle_input()

      if (round_active) {
          spawn_bloons()        // from round data
          update_bloons()       // move along path
          update_towers()       // find targets, fire projectiles
          update_projectiles()  // move, check collisions
          check_collisions()    // projectile vs bloon (spatial partition)
          check_bloon_escapes() // subtract lives
      }

      draw_map()
      draw_path()
      draw_towers()
      draw_bloons()
      draw_projectiles()
      draw_ui()               // sidebar, stats overlay
      draw_cursor()

      gfx_SwapDraw()
  }

  save_game()               // if between rounds
  // No _fini() needed — appvar data is archive-mapped, OS handles cleanup
  gfx_End()
```

---

## Appvar Sprite System

Based on the Clash Royale CE rewrite branch pattern (https://github.com/EverydayCodeNet/CLASH-ROYALE-CE/tree/rewrite).

### Current State (Embedded Sprites)

The current `convimg.yaml` uses `type: c` output, which embeds all sprite data directly into the .8xp program binary. This works but quickly hits the ~65KB program size limit as more sprites are added.

### Target State (Appvar Sprites)

Change the `convimg.yaml` output from `type: c` to `type: appvar`. The palette remains as `type: c` (compiled into the program). Group sprites into appvars by category, each staying under 64KB:

| Appvar Name | Contents | Max Size |
|-------------|----------|----------|
| `BTDTW1` | Tower sprites group 1 (dart, tack, bomb, boomerang, glue, etc.) | < 64KB |
| `BTDTW2` | Tower sprites group 2 (ninja, ice, sniper, etc.) | < 64KB |
| `BTDBLN` | Bloon sprites (all colors, MOAB, damage states) | < 64KB |
| `BTDUI` | UI elements, projectiles, effects, icons | < 64KB |

### How It Works

Each appvar group generates `XXX_init()` and `XXX_fini()` functions automatically:

```c
// In main(), call all _init() functions BEFORE gfx_Begin()
// Check each for failure (returns 0 if appvar not found on calculator)
if (BTDTW1_init() == 0 || BTDTW2_init() == 0 ||
    BTDBLN_init() == 0 || BTDUI_init() == 0) {
    // Error: missing sprite appvar files
    return 1;
}

srand(rtc_Time());  // seed RNG from real-time clock

gfx_Begin();
// ... game code ...
gfx_End();
// No _fini() needed — appvar data is archive-mapped, OS handles cleanup
```

After calling `_init()`, sprite pointers work identically to embedded sprites. No code changes needed for draw calls.

### Deployment

The `.8xv` appvar files must be shipped alongside the `.8xp` program file. Users must transfer all files to their calculator:
- `BTDCE.8xp` (program)
- `BTDTW1.8xv` (tower sprites group 1)
- `BTDTW2.8xv` (tower sprites group 2)
- `BTDBLN.8xv` (bloon sprites)
- `BTDUI.8xv` (UI sprites)

### convimg.yaml Target Structure

```yaml
palettes:
  - name: global_palette
    fixed-entries:
      - color: {index: 0, r: 0, g: 0, b: 0}       # black
      - color: {index: 1, r: 88, g: 94, b: 181}     # transparent (bg color)
      - color: {index: 158, r: 158, g: 181, b: 86}   # green (grass)
      - color: {index: 159, r: 133, g: 133, b: 133}   # grey (path stone)
      - color: {index: 255, r: 255, g: 255, b: 255}   # white
    images: automatic

converts:
  - name: tower_sprites
    palette: global_palette
    transparent-color-index: 1
    images:
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/dart/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/tack_farm/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/sniper/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/bomber/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/boomerang/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/ninja/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/ice/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/glue/*
      # Additional towers added later:
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/super/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/wizard/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/engineer/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/catapult/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/mortar/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/bannana_farm/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/shredder/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/towers/gattling/*

  - name: bloon_sprites
    palette: global_palette
    transparent-color-index: 1
    images:
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/bloons/non-regrow/non-camo/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/bloons/non-regrow/camo/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/bloons/regrow/non-camo/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/bloons/regrow/camo/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/bloons/MOAB/*

  - name: ui_sprites
    palette: global_palette
    transparent-color-index: 1
    images:
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/misc/projectiles/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/misc/icons/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/misc/effects/**/*
      - ../../media/shapes/Derek_and_Harum1_Sprites_shapes/misc/animations/**/*

outputs:
  - type: c
    include-file: gfx.h
    palettes:
      - global_palette

  - type: appvar
    name: BTDTW1
    include-file: btdtw1_gfx.h
    palettes:
      - global_palette
    converts:
      - tower_sprites

  - type: appvar
    name: BTDBLN
    include-file: btdbln_gfx.h
    palettes:
      - global_palette
    converts:
      - bloon_sprites

  - type: appvar
    name: BTDUI
    include-file: btdui_gfx.h
    palettes:
      - global_palette
    converts:
      - ui_sprites
```

---

## Sprite Pipeline

### Step-by-Step Process

1. **Raw sprite sheet**: `media/raw/Derek_and_Harum1_Sprites.png`
   - Full combined sprite sheet by Derek_and_Harum1.
   - Background color: RGB(88, 94, 181) -- used as the transparency key.
   - Glue color (shape connector): RGB(70, 70, 70).

2. **Extract individual sprites**: `python3 src/image_loader.py`
   - Uses `src/splitShapes.py` (BFS connected-component extraction).
   - Outputs individual PNG files to `media/shapes/Derek_and_Harum1_Sprites_shapes/`.
   - Organized into subdirectories: `towers/`, `bloons/`, `misc/`.
   - Names are mapped by the `names` dictionary in `image_loader.py` using the (row, col) of each shape's first pixel.

3. **Square all sprites**: `python3 src/square_sprites.py`
   - Required because graphx rotation (`gfx_RotatedTransparentSprite`) only works with square sprites.
   - Centers the original sprite on a square canvas padded with the background color RGB(88, 94, 181).
   - Overwrites the original files in place.

4. **Convert to CE format**: `cd src/gfx && convimg`
   - Reads `convimg.yaml` configuration.
   - Generates `.c` and `.h` files for embedded sprites, or `.8xv` files for appvar sprites.
   - Palette is always generated as C source.

5. **Build**: `make`
   - Uses CE C SDK toolchain (`cedev-config --makefile`).
   - Outputs `BTDCE.8xp` to `bin/`.
   - Build flags: `-Wall -Wextra -Oz` (optimized for size).

### Build Scripts

- `build.sh`: `cd src/gfx && convimg && cd - && make clean && make && open bin`
- `build_dbg.sh`: `cd src/gfx && convimg && cd - && make clean && make debug && open bin`

---

## UI Layout

### Screen Layout (320x240)

```
+--------------------------------------------------+--------+
|                                                  | TOWER  |
|                   GAME AREA                      | SIDEBAR|
|                  ~280 x 240                      | ~40px  |
|                                                  |        |
|  [Round: 5]  [Cash: $1250]  [Lives: 150]        | [Dart] |
|  (top overlay, semi-transparent)                 | [Tack] |
|                                                  | [Snip] |
|              Path with bloons                    | [Bomb] |
|              and placed towers                   | [Boom] |
|                                                  | [Ninj] |
|              Cursor with range circle            | [Ice]  |
|                                                  | [Glue] |
|                                                  |  [v]   |
|                                                  | scroll |
+--------------------------------------------------+--------+
```

### Game Area (~280 x 240)
- Fills the left portion of the screen.
- Contains the map background, path, placed towers, bloons, and projectiles.
- Top-left or top-center overlay for: Round counter, Cash display, Lives (hearts).

### Right Sidebar (~40 pixels wide)
- Vertical strip of tower icons.
- 6-8 tower icons visible at a time.
- Scrollable if more towers than visible slots.
- Each icon shows the tower sprite and its cost.
- Highlighted border when selected.

### Cursor
- Arrow key controlled.
- When **no tower is selected**: draw a `gfx_Circle` at cursor position (crosshair/selection indicator).
- When **a tower is selected for placement**: draw the tower sprite centered on the cursor, with a range circle around it.
- **Do NOT use the monkey sprite as the default cursor** (this was identified as a bug in the existing code).

### Tower Placement Flow
1. Player selects a tower from the sidebar (number key or scroll + enter).
2. Cursor changes to show the tower sprite with a range circle.
3. Player moves cursor with arrow keys to desired location.
4. Player presses 2nd/Enter to place (if valid position and sufficient funds).
5. Player presses Alpha to cancel placement.

### Tower Upgrade Overlay (Full-Screen Pause)
- Triggered when player selects an already-placed tower and chooses to upgrade.
- Full-screen overlay (pauses gameplay).
- Shows the 2 upgrade paths side-by-side.
- Each path shows: upgrade name, cost, current level indicator.
- Player selects path and confirms with Enter, or cancels with Alpha/Clear.

### Tower Info/Selection Mode
- Tap on a placed tower (move cursor over it and press Enter).
- Shows: range circle, current stats, upgrade button, sell button, targeting mode.
- Can change targeting mode from this view (future: cycle through First/Last/Strong/Close).

---

## Controls

### Key Mapping

| Key | Action |
|-----|--------|
| Arrow Up | Move cursor up |
| Arrow Down | Move cursor down |
| Arrow Left | Move cursor left |
| Arrow Right | Move cursor right |
| 2nd / Enter | Select / place tower / confirm action |
| Alpha | Cancel current action / deselect |
| Clear | Exit / back / quit game |
| Mode | Toggle stats overlay |
| `+` (Add) | Currently: select tower for placement (to be replaced by sidebar) |
| Number keys (1-8) | Quick-select tower from sidebar by slot number |

### Current Code Key Bindings (from main.c)

```c
kb_Data[7] & kb_Up      // cursor up
kb_Data[7] & kb_Down    // cursor down
kb_Data[7] & kb_Left    // cursor left
kb_Data[7] & kb_Right   // cursor right
kb_Data[6] & kb_Enter   // place tower
kb_Data[6] & kb_Add     // select tower type (placeholder)
kb_Data[1] & kb_Mode    // toggle SHOW_STATS
kb_Data[6] & kb_Clear   // exit game
```

---

## Bloon Types and Data

### Standard Bloons (Layer Order)

| # | Bloon | HP | Speed | Children on Pop | Immunities | RBE | Lives Lost |
|---|-------|-----|-------|----------------|------------|-----|------------|
| 0 | Red | 1 | 1.0 | None | None | 1 | 1 |
| 1 | Blue | 1 | 1.4 | 1 Red | None | 2 | 2 |
| 2 | Green | 1 | 1.8 | 1 Blue | None | 3 | 3 |
| 3 | Yellow | 1 | 3.2 | 1 Green | None | 4 | 4 |
| 4 | Pink | 1 | 3.5 | 1 Yellow | None | 5 | 5 |
| 5 | Black | 1 | 1.8 | 2 Pink | Explosion | 11 | 11 |
| 6 | White | 1 | 2.0 | 2 Pink | Freeze | 11 | 11 |
| 7 | Lead | 1 | 1.0 | 2 Black | Sharp | 23 | 23 |
| 8 | Zebra | 1 | 1.8 | 1 Black + 1 White | Explosion + Freeze | 23 | 23 |
| 9 | Rainbow | 1 | 2.2 | 2 Zebra | None | 47 | 47 |
| 10 | Ceramic | 10 | 2.5 | 2 Rainbow | None | 104 | 104 |
| 11 | MOAB | 200 | 1.0 | 4 Ceramic | None | 616 | 616 |

### Speed Notes

All speeds are relative to Red = 1.0. For implementation, convert to pixels-per-frame using a base speed constant:

```c
#define BASE_BLOON_SPEED 2  // Red bloon moves 2 pixels per frame

// Actual speed = BASE_BLOON_SPEED * speed_multiplier
// Use fixed-point (x256) to avoid floating point:
// Red:     256
// Blue:    358  (1.4 * 256)
// Green:   461  (1.8 * 256)
// Yellow:  819  (3.2 * 256)
// Pink:    896  (3.5 * 256)
// Black:   461  (1.8 * 256)
// White:   512  (2.0 * 256)
// Lead:    256  (1.0 * 256)
// Zebra:   461  (1.8 * 256)
// Rainbow: 563  (2.2 * 256)
// Ceramic: 640  (2.5 * 256)
// MOAB:    256  (1.0 * 256)
```

### C Data Table

```c
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

typedef enum {
    DMG_SHARP     = 0x01,  // darts, tacks, shurikens
    DMG_EXPLOSION = 0x02,  // bombs
    DMG_FREEZE    = 0x04,  // ice
    DMG_NORMAL    = 0x08,  // bypasses all immunities
    DMG_ENERGY    = 0x10,  // unused (no purple bloons)
} damage_type_t;

typedef enum {
    IMMUNE_SHARP     = 0x01,  // Lead blocks sharp
    IMMUNE_EXPLOSION = 0x02,  // Black blocks explosion
    IMMUNE_FREEZE    = 0x04,  // White blocks freeze
} immunity_t;

typedef enum {
    MOD_NONE   = 0x00,
    MOD_CAMO   = 0x01,
    MOD_REGROW = 0x02,
} bloon_modifier_t;

typedef struct {
    uint8_t hp;
    uint16_t speed_fp;      // fixed-point speed (x256)
    uint8_t child_type;     // bloon_type_t of children (or 0xFF = none)
    uint8_t child_count;    // how many children spawn on pop
    uint8_t child_type2;    // second child type (for Zebra: White)
    uint8_t child_count2;   // second child count (for Zebra: 1)
    uint8_t immunities;     // bitmask of immunity_t
    uint16_t rbe;           // Red Bloon Equivalent (total lives lost)
} bloon_data_t;

static const bloon_data_t BLOON_DATA[NUM_BLOON_TYPES] = {
    // HP   Speed   Child1        Cnt1  Child2        Cnt2  Immune    RBE
    {  1,   256,    0xFF,          0,    0xFF,          0,   0x00,      1 }, // Red
    {  1,   358,    BLOON_RED,     1,    0xFF,          0,   0x00,      2 }, // Blue
    {  1,   461,    BLOON_BLUE,    1,    0xFF,          0,   0x00,      3 }, // Green
    {  1,   819,    BLOON_GREEN,   1,    0xFF,          0,   0x00,      4 }, // Yellow
    {  1,   896,    BLOON_YELLOW,  1,    0xFF,          0,   0x00,      5 }, // Pink
    {  1,   461,    BLOON_PINK,    2,    0xFF,          0,   0x02,     11 }, // Black
    {  1,   512,    BLOON_PINK,    2,    0xFF,          0,   0x04,     11 }, // White
    {  1,   256,    BLOON_BLACK,   2,    0xFF,          0,   0x01,     23 }, // Lead
    {  1,   461,    BLOON_BLACK,   1,    BLOON_WHITE,   1,   0x06,     23 }, // Zebra
    {  1,   563,    BLOON_ZEBRA,   2,    0xFF,          0,   0x00,     47 }, // Rainbow
    { 10,   640,    BLOON_RAINBOW, 2,    0xFF,          0,   0x00,    104 }, // Ceramic
    {200,   256,    BLOON_CERAMIC, 4,    0xFF,          0,   0x00,    616 }, // MOAB
};
```

### Modifiers

| Modifier | Effect | Implementation |
|----------|--------|----------------|
| **Camo** | Invisible to towers without camo detection | Bloon has `MOD_CAMO` flag; towers skip this bloon in targeting unless they have `can_see_camo` |
| **Regrow** | Regenerates layers over time | Bloon has `MOD_REGROW` flag and `regrow_max` (highest type it can regrow to). Every ~3 seconds (90 frames at 30fps), it regrows 1 layer toward `regrow_max` |
| **Fortified** | Doubles HP of the bloon layer | Future feature. Would set a flag doubling the `hp` field |

### Immunity Resolution

When a projectile hits a bloon:
1. Get the projectile's `damage_type` (bitmask).
2. Get the bloon's `immunities` (bitmask).
3. If `damage_type` includes `DMG_NORMAL`, the hit always succeeds (bypasses all immunities).
4. Otherwise, if `damage_type & immunities != 0`, the bloon is immune -- the projectile passes through without dealing damage (but still consumes pierce).

---

## Tower Data

### Core 8 Towers

All costs below are base costs for Easy difficulty (1.0x multiplier). Attack speeds are in frames between shots (at ~30fps game speed).

#### 1. Dart Monkey

| Stat | Value |
|------|-------|
| **Cost** | $200 |
| **Attack Speed** | 21 frames (~0.7s) |
| **Range** | 40 pixels |
| **Damage** | 1 |
| **Pierce** | 2 |
| **Damage Type** | Sharp |
| **Projectile** | Dart (travels in straight line) |
| **Special** | None (basic starter tower) |

**Upgrade Path 1 -- Sharp Shots**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Sharp Shots | $140 | +1 pierce (total 3) |
| 2 | Razor Sharp Shots | $220 | +2 pierce (total 5) |
| 3 | Triple Shot | $400 | Fires 3 darts per attack |
| 4 | Super Fan Club | $8000 | Transforms nearby dart monkeys temporarily |

**Upgrade Path 2 -- Quick Shots**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Quick Shots | $100 | +15% attack speed |
| 2 | Very Quick Shots | $190 | +15% more attack speed |
| 3 | Long Range Darts | $300 | +15% range |
| 4 | Enhanced Eyesight | $2500 | Camo detection + more range |

---

#### 2. Tack Shooter

| Stat | Value |
|------|-------|
| **Cost** | $280 |
| **Attack Speed** | 20 frames (~0.67s) |
| **Range** | 28 pixels (short range) |
| **Damage** | 1 |
| **Pierce** | 1 per tack |
| **Damage Type** | Sharp |
| **Projectile** | 8 tacks fired in all compass directions simultaneously |
| **Special** | Area coverage tower; all 8 tacks fire at once |

**Upgrade Path 1 -- Faster Shooting**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Faster Shooting | $200 | +25% attack speed |
| 2 | Even Faster Shooting | $300 | +25% more attack speed |
| 3 | Tack Sprayer | $550 | Fires 16 tacks instead of 8 |
| 4 | Ring of Fire | $3500 | Continuous fire ring, pops bloons in range |

**Upgrade Path 2 -- Extra Range**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Extra Range Tacks | $100 | +15% range |
| 2 | Super Range Tacks | $225 | +15% more range |
| 3 | Blade Shooter | $500 | +2 pierce, +1 damage |
| 4 | Blade Maelstrom | $2800 | Ability: shoots blades everywhere for a short time |

---

#### 3. Sniper Monkey

| Stat | Value |
|------|-------|
| **Cost** | $350 |
| **Attack Speed** | 48 frames (~1.6s) |
| **Range** | Infinite (full map) |
| **Damage** | 2 |
| **Pierce** | 1 (hitscan, single target) |
| **Damage Type** | Sharp |
| **Projectile** | Hitscan (instant hit, no visible projectile travel) |
| **Special** | Targets anywhere on map. Hitscan means no projectile entity is created. |

**Upgrade Path 1 -- Firepower**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Full Metal Jacket | $350 | Damage type becomes Normal (pops Lead) |
| 2 | Large Calibre | $1500 | +3 damage (total 5) |
| 3 | Deadly Precision | $3000 | +13 damage (total 18) |
| 4 | Cripple MOAB | $12000 | Slows and weakens MOAB-class bloons |

**Upgrade Path 2 -- Utility**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Night Vision Goggles | $200 | Camo detection |
| 2 | Shrapnel Shot | $400 | Hit bloon releases shrapnel hitting nearby bloons |
| 3 | Bouncing Bullet | $3200 | Shot bounces to nearby bloons |
| 4 | Supply Drop | $7000 | Ability: drops a crate of cash |

---

#### 4. Bomb Shooter

| Stat | Value |
|------|-------|
| **Cost** | $525 |
| **Attack Speed** | 24 frames (~0.8s) |
| **Range** | 40 pixels |
| **Damage** | 1 |
| **Pierce** | 18 (explosion splash radius) |
| **Damage Type** | Explosion |
| **Projectile** | Bomb (arcing projectile, explodes on contact) |
| **Special** | Cannot pop Black or Zebra bloons (Explosion immunity). Splash damage hits many bloons. |

**Upgrade Path 1 -- Bigger Bombs**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Bigger Bombs | $350 | +6 pierce (total 24), larger blast |
| 2 | Heavy Bombs | $650 | +1 damage (total 2) |
| 3 | Really Big Bombs | $1200 | +10 pierce (total 34), even larger blast |
| 4 | Bloon Impact | $3400 | Stuns bloons on hit |

**Upgrade Path 2 -- Missile**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Extra Range | $200 | +8 range |
| 2 | Frag Bombs | $300 | Explosion releases 8 sharp fragments |
| 3 | Cluster Bombs | $800 | Explosion releases 8 smaller bombs |
| 4 | MOAB Mauler | $3200 | +10 damage to MOAB-class bloons |

---

#### 5. Boomerang Monkey

| Stat | Value |
|------|-------|
| **Cost** | $325 |
| **Attack Speed** | 21 frames (~0.7s) |
| **Range** | 40 pixels |
| **Damage** | 1 |
| **Pierce** | 4 |
| **Damage Type** | Sharp |
| **Projectile** | Boomerang (follows a curved return path, can hit on both outgoing and return arc) |
| **Special** | Boomerang follows an arc and returns, hitting bloons twice potentially. |

**Upgrade Path 1 -- Better Rangs**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Multi-Target | $250 | +3 pierce (total 7) |
| 2 | Glaive Thrower | $300 | Faster projectile, +4 pierce (total 11) |
| 3 | Glaive Ricochet | $1400 | Glaive bounces between bloons |
| 4 | Glaive Lord | $5500 | Orbiting glaives that shred bloons |

**Upgrade Path 2 -- Power**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Sonic Boom | $200 | Damage type becomes Normal (pops Lead + Frozen) |
| 2 | Red Hot Rangs | $300 | +1 damage |
| 3 | Bionic Boomer | $1600 | +75% attack speed |
| 4 | Turbo Charge | $3200 | Ability: massively increased attack speed |

---

#### 6. Ninja Monkey

| Stat | Value |
|------|-------|
| **Cost** | $500 |
| **Attack Speed** | 17 frames (~0.57s) |
| **Range** | 40 pixels |
| **Damage** | 1 |
| **Pierce** | 2 |
| **Damage Type** | Sharp |
| **Projectile** | Shuriken (fast, straight line) |
| **Special** | **Built-in camo detection**. Fast attack speed. |

**Upgrade Path 1 -- Ninja Discipline**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Ninja Discipline | $200 | +10% attack speed, +10% range |
| 2 | Sharp Shurikens | $350 | +2 pierce (total 4) |
| 3 | Double Shot | $700 | Fires 2 shurikens per attack |
| 4 | Bloonjitsu | $2750 | Fires 5 shurikens per attack |

**Upgrade Path 2 -- Distraction**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Distraction | $350 | Chance to send bloons back on the track |
| 2 | Counter-Espionage | $500 | De-camos bloons on hit |
| 3 | Sabotage Supply Lines | $4500 | Ability: slows all bloons on screen |
| 4 | Grand Saboteur | $12000 | Ability also reduces MOAB health |

---

#### 7. Ice Monkey

| Stat | Value |
|------|-------|
| **Cost** | $500 |
| **Attack Speed** | 39 frames (~1.3s) |
| **Range** | 30 pixels |
| **Damage** | 0 (does not damage by default) |
| **Pierce** | 40 (all bloons in radius) |
| **Damage Type** | Freeze |
| **Projectile** | None (area effect around tower) |
| **Special** | Freezes all bloons in range for ~1.5 seconds. Frozen bloons cannot be moved. Cannot affect White or Zebra bloons (Freeze immunity). Does not deal damage by default. |

**Upgrade Path 1 -- Colder Ice**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Enhanced Freeze | $225 | Freeze duration +0.5s |
| 2 | Deep Freeze | $350 | +1 freeze layer (freezes bloons popped from frozen parents) |
| 3 | Arctic Wind | $2500 | +50% range, slows bloons even outside freeze radius |
| 4 | Viral Frost | $3000 | Frozen bloons freeze nearby bloons |

**Upgrade Path 2 -- Snap Freeze**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Permafrost | $100 | Frozen bloons move slower even after thawing |
| 2 | Metal Freeze | $300 | Can freeze Lead bloons |
| 3 | Ice Shards | $1500 | Frozen bloons release shards when popped |
| 4 | Absolute Zero | $2500 | Ability: freezes all bloons on screen |

---

#### 8. Glue Gunner

| Stat | Value |
|------|-------|
| **Cost** | $275 |
| **Attack Speed** | 24 frames (~0.8s) |
| **Range** | 38 pixels |
| **Damage** | 0 (does not damage by default) |
| **Pierce** | 1 (single target) |
| **Damage Type** | None |
| **Projectile** | Glue blob (travels to target, applies slow) |
| **Special** | Slows hit bloon by ~50% for several seconds. Does not deal damage by default. |

**Upgrade Path 1 -- Stronger Glue**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Glue Soak | $200 | Glue soaks through layers (stays when bloon is popped) |
| 2 | Corrosive Glue | $300 | Glue deals 1 damage per 2 seconds |
| 3 | Bloon Dissolver | $2500 | Greatly increased corrosive damage |
| 4 | Bloon Liquifier | $5000 | Extreme corrosive damage |

**Upgrade Path 2 -- Coverage**:
| Level | Name | Cost | Effect |
|-------|------|------|--------|
| 1 | Stickier Glue | $100 | Slow effect lasts longer |
| 2 | Glue Splatter | $1800 | Glue splashes to 6 nearby bloons |
| 3 | MOAB Glue | $3400 | Can target and slow MOAB-class bloons |
| 4 | Glue Striker | $3500 | Ability: glues all bloons on screen |

### Tower Data C Table (Base Stats)

```c
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

typedef struct {
    uint16_t cost;
    uint8_t attack_frames;   // frames between attacks
    uint8_t range;           // in pixels
    uint8_t damage;
    uint8_t pierce;
    uint8_t damage_type;     // bitmask of damage_type_t
    uint8_t can_see_camo;    // 1 if tower can target camo bloons
    uint8_t projectile_count;// how many projectiles per attack
    uint8_t projectile_speed;// pixels per frame
    uint8_t is_hitscan;      // 1 for sniper (no projectile travel)
    uint8_t is_area_effect;  // 1 for ice (no projectile, area freeze)
} tower_data_t;

static const tower_data_t TOWER_DATA[NUM_TOWER_TYPES] = {
    //  Cost  AtkF  Rng  Dmg  Prc  DmgT  Camo  #Proj PSpd Scan Area
    {   200,  21,   40,   1,   2,  0x01,   0,    1,    5,   0,   0 }, // Dart
    {   280,  20,   28,   1,   1,  0x01,   0,    8,    4,   0,   0 }, // Tack
    {   350,  48,  255,   2,   1,  0x01,   0,    1,    0,   1,   0 }, // Sniper
    {   525,  24,   40,   1,  18,  0x02,   0,    1,    3,   0,   0 }, // Bomb
    {   325,  21,   40,   1,   4,  0x01,   0,    1,    4,   0,   0 }, // Boomerang
    {   500,  17,   40,   1,   2,  0x01,   1,    1,    6,   0,   0 }, // Ninja
    {   500,  39,   30,   0,  40,  0x04,   0,    0,    0,   0,   1 }, // Ice
    {   275,  24,   38,   0,   1,  0x00,   0,    1,    4,   0,   0 }, // Glue
};
```

---

## Round Data

Round data defines which bloons spawn in each round, in what quantity, and with what spacing. The implementation uses compact C arrays.

### Round Data Structure

```c
typedef struct {
    uint8_t bloon_type;    // bloon_type_t
    uint8_t modifiers;     // MOD_CAMO | MOD_REGROW bitmask
    uint16_t count;        // how many of this bloon type
    uint8_t spacing;       // frames between spawns of this group
} round_group_t;

typedef struct {
    const round_group_t *groups;
    uint8_t num_groups;
} round_def_t;
```

### Rounds 1-80 (Simplified BTD6 Data)

Note: Spacing values are in game frames (~30fps). Groups within a round spawn sequentially (group 2 starts after group 1 finishes).

```c
// Round 1
static const round_group_t R1[] = {
    { BLOON_RED, 0, 20, 25 },
};

// Round 2
static const round_group_t R2[] = {
    { BLOON_RED, 0, 30, 20 },
};

// Round 3
static const round_group_t R3[] = {
    { BLOON_RED, 0, 25, 15 },
    { BLOON_BLUE, 0, 5, 25 },
};

// Round 4
static const round_group_t R4[] = {
    { BLOON_RED, 0, 30, 15 },
    { BLOON_BLUE, 0, 15, 20 },
};

// Round 5
static const round_group_t R5[] = {
    { BLOON_RED, 0, 5, 15 },
    { BLOON_BLUE, 0, 27, 15 },
};

// Round 6
static const round_group_t R6[] = {
    { BLOON_RED, 0, 15, 10 },
    { BLOON_BLUE, 0, 15, 15 },
    { BLOON_GREEN, 0, 4, 25 },
};

// Round 7
static const round_group_t R7[] = {
    { BLOON_RED, 0, 20, 10 },
    { BLOON_BLUE, 0, 20, 12 },
    { BLOON_GREEN, 0, 5, 20 },
};

// Round 8
static const round_group_t R8[] = {
    { BLOON_RED, 0, 10, 10 },
    { BLOON_BLUE, 0, 20, 12 },
    { BLOON_GREEN, 0, 14, 15 },
};

// Round 9
static const round_group_t R9[] = {
    { BLOON_GREEN, 0, 30, 10 },
};

// Round 10
static const round_group_t R10[] = {
    { BLOON_BLUE, 0, 20, 8 },
    { BLOON_GREEN, 0, 10, 12 },
    { BLOON_YELLOW, 0, 2, 30 },
};

// Round 11
static const round_group_t R11[] = {
    { BLOON_BLUE, 0, 10, 10 },
    { BLOON_GREEN, 0, 12, 12 },
    { BLOON_YELLOW, 0, 8, 18 },
};

// Round 12
static const round_group_t R12[] = {
    { BLOON_BLUE, 0, 15, 8 },
    { BLOON_GREEN, 0, 15, 10 },
    { BLOON_YELLOW, 0, 5, 15 },
    { BLOON_PINK, 0, 2, 20 },
};

// Round 13
static const round_group_t R13[] = {
    { BLOON_BLUE, 0, 30, 5 },
    { BLOON_GREEN, 0, 10, 10 },
    { BLOON_YELLOW, 0, 8, 12 },
    { BLOON_PINK, 0, 5, 18 },
};

// Round 14
static const round_group_t R14[] = {
    { BLOON_RED, 0, 30, 5 },
    { BLOON_BLUE, 0, 20, 5 },
    { BLOON_GREEN, 0, 15, 8 },
    { BLOON_YELLOW, 0, 10, 10 },
    { BLOON_PINK, 0, 5, 12 },
};

// Round 15
static const round_group_t R15[] = {
    { BLOON_RED, 0, 20, 5 },
    { BLOON_BLUE, 0, 15, 5 },
    { BLOON_GREEN, 0, 12, 8 },
    { BLOON_YELLOW, 0, 10, 10 },
    { BLOON_PINK, 0, 10, 12 },
};

// Round 16
static const round_group_t R16[] = {
    { BLOON_GREEN, 0, 20, 5 },
    { BLOON_YELLOW, 0, 15, 8 },
    { BLOON_PINK, 0, 12, 10 },
};

// Round 17
static const round_group_t R17[] = {
    { BLOON_YELLOW, 0, 25, 6 },
    { BLOON_PINK, 0, 8, 10 },
};

// Round 18
static const round_group_t R18[] = {
    { BLOON_GREEN, 0, 30, 5 },
    { BLOON_YELLOW, 0, 10, 8 },
    { BLOON_PINK, 0, 8, 10 },
};

// Round 19
static const round_group_t R19[] = {
    { BLOON_GREEN, 0, 20, 5 },
    { BLOON_YELLOW, 0, 15, 6 },
    { BLOON_PINK, 0, 12, 8 },
};

// Round 20
static const round_group_t R20[] = {
    { BLOON_BLACK, 0, 6, 15 },
};

// Round 21
static const round_group_t R21[] = {
    { BLOON_YELLOW, 0, 20, 5 },
    { BLOON_PINK, 0, 15, 8 },
    { BLOON_BLACK, 0, 8, 12 },
};

// Round 22
static const round_group_t R22[] = {
    { BLOON_WHITE, 0, 8, 12 },
    { BLOON_BLACK, 0, 8, 12 },
};

// Round 23
static const round_group_t R23[] = {
    { BLOON_YELLOW, 0, 15, 5 },
    { BLOON_WHITE, 0, 10, 10 },
    { BLOON_BLACK, 0, 10, 10 },
};

// Round 24
static const round_group_t R24[] = {
    { BLOON_GREEN, MOD_CAMO, 20, 8 },
    { BLOON_PINK, 0, 15, 8 },
    { BLOON_BLACK, 0, 5, 15 },
    { BLOON_WHITE, 0, 5, 15 },
};

// Round 25
static const round_group_t R25[] = {
    { BLOON_YELLOW, MOD_REGROW, 25, 5 },
    { BLOON_BLACK, 0, 10, 10 },
    { BLOON_WHITE, 0, 10, 10 },
};

// Round 26
static const round_group_t R26[] = {
    { BLOON_PINK, 0, 30, 4 },
    { BLOON_BLACK, 0, 10, 8 },
    { BLOON_WHITE, 0, 6, 10 },
    { BLOON_ZEBRA, 0, 4, 18 },
};

// Round 27
static const round_group_t R27[] = {
    { BLOON_YELLOW, 0, 25, 4 },
    { BLOON_BLACK, 0, 12, 8 },
    { BLOON_WHITE, 0, 12, 8 },
    { BLOON_LEAD, 0, 3, 30 },
};

// Round 28
static const round_group_t R28[] = {
    { BLOON_LEAD, 0, 6, 20 },
    { BLOON_BLACK, 0, 15, 8 },
    { BLOON_ZEBRA, 0, 8, 12 },
};

// Round 29
static const round_group_t R29[] = {
    { BLOON_PINK, 0, 30, 3 },
    { BLOON_BLACK, 0, 12, 6 },
    { BLOON_WHITE, 0, 12, 6 },
    { BLOON_ZEBRA, 0, 6, 10 },
    { BLOON_RAINBOW, 0, 2, 25 },
};

// Round 30
static const round_group_t R30[] = {
    { BLOON_LEAD, 0, 8, 15 },
    { BLOON_ZEBRA, 0, 10, 8 },
    { BLOON_RAINBOW, 0, 4, 15 },
};

// Round 31
static const round_group_t R31[] = {
    { BLOON_PINK, MOD_CAMO, 20, 5 },
    { BLOON_BLACK, 0, 15, 6 },
    { BLOON_WHITE, 0, 15, 6 },
    { BLOON_ZEBRA, 0, 8, 8 },
    { BLOON_RAINBOW, 0, 5, 12 },
};

// Round 32
static const round_group_t R32[] = {
    { BLOON_YELLOW, MOD_REGROW, 30, 3 },
    { BLOON_ZEBRA, 0, 10, 8 },
    { BLOON_RAINBOW, 0, 8, 10 },
};

// Round 33
static const round_group_t R33[] = {
    { BLOON_BLACK, MOD_REGROW, 15, 6 },
    { BLOON_WHITE, MOD_REGROW, 15, 6 },
    { BLOON_RAINBOW, 0, 10, 8 },
};

// Round 34
static const round_group_t R34[] = {
    { BLOON_ZEBRA, 0, 20, 5 },
    { BLOON_RAINBOW, 0, 12, 8 },
    { BLOON_LEAD, 0, 5, 15 },
};

// Round 35
static const round_group_t R35[] = {
    { BLOON_BLACK, MOD_CAMO | MOD_REGROW, 10, 8 },
    { BLOON_PINK, 0, 30, 3 },
    { BLOON_RAINBOW, 0, 15, 6 },
};

// Round 36
static const round_group_t R36[] = {
    { BLOON_PINK, 0, 50, 2 },
    { BLOON_BLACK, 0, 20, 5 },
    { BLOON_RAINBOW, 0, 12, 6 },
    { BLOON_LEAD, MOD_CAMO, 3, 25 },
};

// Round 37
static const round_group_t R37[] = {
    { BLOON_ZEBRA, MOD_REGROW, 20, 5 },
    { BLOON_RAINBOW, 0, 15, 6 },
    { BLOON_CERAMIC, 0, 2, 40 },
};

// Round 38
static const round_group_t R38[] = {
    { BLOON_RAINBOW, 0, 20, 5 },
    { BLOON_CERAMIC, 0, 4, 30 },
    { BLOON_WHITE, MOD_REGROW, 20, 5 },
};

// Round 39
static const round_group_t R39[] = {
    { BLOON_BLACK, MOD_REGROW, 30, 3 },
    { BLOON_RAINBOW, 0, 20, 4 },
    { BLOON_CERAMIC, 0, 6, 20 },
};

// Round 40 (EASY MODE FINAL)
static const round_group_t R40[] = {
    { BLOON_CERAMIC, 0, 10, 12 },
    { BLOON_RAINBOW, MOD_REGROW, 15, 5 },
    { BLOON_LEAD, MOD_CAMO, 5, 15 },
};

// Round 41
static const round_group_t R41[] = {
    { BLOON_CERAMIC, MOD_REGROW, 8, 12 },
    { BLOON_RAINBOW, 0, 25, 4 },
    { BLOON_ZEBRA, MOD_CAMO, 15, 5 },
};

// Round 42
static const round_group_t R42[] = {
    { BLOON_RAINBOW, MOD_REGROW, 20, 4 },
    { BLOON_CERAMIC, 0, 10, 10 },
    { BLOON_BLACK, MOD_CAMO, 20, 5 },
};

// Round 43
static const round_group_t R43[] = {
    { BLOON_CERAMIC, 0, 15, 8 },
    { BLOON_LEAD, 0, 10, 10 },
    { BLOON_RAINBOW, MOD_CAMO, 10, 6 },
};

// Round 44
static const round_group_t R44[] = {
    { BLOON_CERAMIC, MOD_REGROW, 12, 8 },
    { BLOON_RAINBOW, 0, 30, 3 },
};

// Round 45
static const round_group_t R45[] = {
    { BLOON_CERAMIC, MOD_CAMO, 10, 10 },
    { BLOON_CERAMIC, 0, 15, 8 },
    { BLOON_PINK, MOD_CAMO | MOD_REGROW, 30, 3 },
};

// Round 46
static const round_group_t R46[] = {
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_CERAMIC, 0, 10, 10 },
};

// Round 47
static const round_group_t R47[] = {
    { BLOON_CERAMIC, MOD_REGROW, 20, 6 },
    { BLOON_RAINBOW, MOD_CAMO, 20, 5 },
    { BLOON_LEAD, MOD_CAMO, 10, 10 },
};

// Round 48
static const round_group_t R48[] = {
    { BLOON_CERAMIC, 0, 20, 6 },
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_RAINBOW, MOD_REGROW, 20, 4 },
};

// Round 49
static const round_group_t R49[] = {
    { BLOON_CERAMIC, MOD_REGROW, 25, 5 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 8, 12 },
    { BLOON_RAINBOW, 0, 30, 3 },
};

// Round 50
static const round_group_t R50[] = {
    { BLOON_MOAB, 0, 2, 60 },
    { BLOON_CERAMIC, MOD_CAMO, 15, 6 },
};

// Round 51
static const round_group_t R51[] = {
    { BLOON_CERAMIC, 0, 30, 4 },
    { BLOON_RAINBOW, MOD_REGROW, 25, 4 },
    { BLOON_LEAD, MOD_CAMO, 10, 10 },
};

// Round 52
static const round_group_t R52[] = {
    { BLOON_CERAMIC, MOD_CAMO, 15, 6 },
    { BLOON_MOAB, 0, 1, 60 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 15, 5 },
};

// Round 53
static const round_group_t R53[] = {
    { BLOON_CERAMIC, MOD_REGROW, 20, 5 },
    { BLOON_CERAMIC, MOD_CAMO, 15, 6 },
    { BLOON_LEAD, MOD_CAMO, 12, 8 },
};

// Round 54
static const round_group_t R54[] = {
    { BLOON_MOAB, 0, 2, 50 },
    { BLOON_CERAMIC, 0, 20, 5 },
};

// Round 55
static const round_group_t R55[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 20, 5 },
    { BLOON_RAINBOW, 0, 40, 3 },
    { BLOON_MOAB, 0, 1, 60 },
};

// Round 56
static const round_group_t R56[] = {
    { BLOON_MOAB, 0, 2, 45 },
    { BLOON_CERAMIC, MOD_REGROW, 20, 5 },
    { BLOON_LEAD, MOD_CAMO, 8, 12 },
};

// Round 57
static const round_group_t R57[] = {
    { BLOON_CERAMIC, MOD_CAMO, 25, 4 },
    { BLOON_RAINBOW, MOD_REGROW, 30, 3 },
    { BLOON_MOAB, 0, 1, 60 },
};

// Round 58
static const round_group_t R58[] = {
    { BLOON_MOAB, 0, 2, 40 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 15, 5 },
    { BLOON_LEAD, MOD_CAMO, 10, 10 },
};

// Round 59
static const round_group_t R59[] = {
    { BLOON_CERAMIC, 0, 30, 3 },
    { BLOON_CERAMIC, MOD_REGROW, 20, 4 },
    { BLOON_MOAB, 0, 2, 40 },
};

// Round 60 (MEDIUM MODE FINAL)
static const round_group_t R60[] = {
    { BLOON_MOAB, 0, 3, 35 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 20, 5 },
};

// Round 61
static const round_group_t R61[] = {
    { BLOON_MOAB, 0, 2, 35 },
    { BLOON_CERAMIC, MOD_CAMO, 30, 3 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 20, 4 },
};

// Round 62
static const round_group_t R62[] = {
    { BLOON_MOAB, 0, 3, 30 },
    { BLOON_CERAMIC, MOD_REGROW, 25, 4 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 10, 10 },
};

// Round 63
static const round_group_t R63[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 40, 3 },
    { BLOON_LEAD, MOD_CAMO, 15, 6 },
    { BLOON_MOAB, 0, 2, 35 },
};

// Round 64
static const round_group_t R64[] = {
    { BLOON_MOAB, 0, 3, 30 },
    { BLOON_CERAMIC, 0, 35, 3 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 25, 4 },
};

// Round 65
static const round_group_t R65[] = {
    { BLOON_MOAB, 0, 4, 25 },
    { BLOON_CERAMIC, MOD_CAMO, 20, 4 },
};

// Round 66
static const round_group_t R66[] = {
    { BLOON_CERAMIC, MOD_REGROW, 50, 2 },
    { BLOON_MOAB, 0, 2, 30 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 15, 8 },
};

// Round 67
static const round_group_t R67[] = {
    { BLOON_MOAB, 0, 3, 25 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 30, 3 },
};

// Round 68
static const round_group_t R68[] = {
    { BLOON_MOAB, 0, 4, 22 },
    { BLOON_CERAMIC, MOD_CAMO, 25, 3 },
    { BLOON_LEAD, MOD_CAMO, 12, 8 },
};

// Round 69
static const round_group_t R69[] = {
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 40, 2 },
    { BLOON_MOAB, 0, 3, 25 },
};

// Round 70
static const round_group_t R70[] = {
    { BLOON_MOAB, 0, 4, 20 },
    { BLOON_CERAMIC, MOD_REGROW, 30, 3 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 10, 10 },
};

// Round 71
static const round_group_t R71[] = {
    { BLOON_MOAB, 0, 3, 20 },
    { BLOON_CERAMIC, MOD_CAMO, 35, 3 },
    { BLOON_RAINBOW, MOD_CAMO | MOD_REGROW, 30, 3 },
};

// Round 72
static const round_group_t R72[] = {
    { BLOON_MOAB, 0, 4, 18 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 35, 3 },
};

// Round 73
static const round_group_t R73[] = {
    { BLOON_MOAB, 0, 5, 18 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 20, 6 },
    { BLOON_CERAMIC, 0, 40, 2 },
};

// Round 74
static const round_group_t R74[] = {
    { BLOON_MOAB, 0, 4, 18 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 40, 2 },
    { BLOON_RAINBOW, MOD_REGROW, 40, 2 },
};

// Round 75
static const round_group_t R75[] = {
    { BLOON_MOAB, 0, 5, 16 },
    { BLOON_CERAMIC, MOD_CAMO, 30, 3 },
};

// Round 76
static const round_group_t R76[] = {
    { BLOON_MOAB, 0, 5, 15 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 40, 2 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 15, 8 },
};

// Round 77
static const round_group_t R77[] = {
    { BLOON_MOAB, 0, 6, 15 },
    { BLOON_CERAMIC, MOD_REGROW, 50, 2 },
};

// Round 78
static const round_group_t R78[] = {
    { BLOON_MOAB, 0, 6, 14 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 40, 2 },
    { BLOON_LEAD, MOD_CAMO, 20, 6 },
};

// Round 79
static const round_group_t R79[] = {
    { BLOON_MOAB, 0, 7, 12 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 50, 2 },
};

// Round 80 (HARD MODE FINAL)
static const round_group_t R80[] = {
    { BLOON_MOAB, 0, 8, 10 },
    { BLOON_CERAMIC, MOD_CAMO | MOD_REGROW, 30, 3 },
    { BLOON_LEAD, MOD_CAMO | MOD_REGROW, 15, 6 },
};

// Round definitions table (index 0 = round 1)
static const round_def_t ROUND_DEFS[80] = {
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
```

### Difficulty Round Ranges

| Difficulty | Start Round | End Round | Notes |
|------------|-------------|-----------|-------|
| Easy | 1 | 40 | No MOAB. Ceramics appear around round 37-40. |
| Medium | 1 | 60 | First MOAB around round 46. Multiple MOABs by round 60. |
| Hard | 1 | 80 | Many MOABs. Heavy camo + regrow combos. |

---

## Cash System

| Parameter | Value |
|-----------|-------|
| **Starting Cash** | $650 |
| **Cash per Pop** | $1 per bloon layer popped |
| **End-of-Round Bonus** | $100 + (round_number - 1) |
| **Tower Sell Return** | 70% of total invested amount (base cost + all upgrades purchased) |

### Difficulty Cash Multipliers

| Difficulty | Tower Cost Multiplier | Income Multiplier |
|------------|----------------------|-------------------|
| Easy | 0.85x, rounded to nearest $5 | 1.0x |
| Medium | 1.0x | 1.0x |
| Hard | 1.08x, rounded to nearest $5 | 0.8x (reduced income) |

### Implementation Notes

```c
// End-of-round bonus calculation
int24_t end_of_round_bonus(uint8_t round, uint8_t difficulty) {
    int24_t bonus = 100 + (round - 1);
    if (difficulty == HARD) {
        bonus = (bonus * 4) / 5;  // 0.8x multiplier
    }
    return bonus;
}

// Tower cost calculation (wiki-standard rounding to nearest $5)
uint16_t tower_cost(uint8_t tower_type, uint8_t difficulty) {
    uint16_t base = TOWER_DATA[tower_type].cost;
    uint16_t cost;
    switch (difficulty) {
        case EASY:   cost = (base * 85 + 50) / 100; break;
        case HARD:   cost = (base * 108 + 50) / 100; break;
        default:     return base;
    }
    return ((cost + 2) / 5) * 5;  // Round to nearest $5
}

// Tower sell value
uint16_t sell_value(tower_t *tower) {
    return (tower->total_invested * 7) / 10;  // 70% return
}
```

---

## Lives System

| Difficulty | Starting Lives |
|------------|---------------|
| Easy | 200 |
| Medium | 150 |
| Hard | 100 |

**Lives lost per bloon escape** = **RBE** of that bloon type (including all children that would have spawned).

For example, if a Rainbow bloon (RBE = 47) escapes past the end of the path, the player loses 47 lives.

### Game Over

The game ends when lives reach 0. On game over, display a results screen showing:
- Round reached
- Bloons popped
- Cash earned
- Option to return to main menu

---

## Map System

Maps are defined as arrays of waypoints (path points). The bloon track follows these waypoints as a piecewise linear path. The path is rendered as filled rectangles connecting the waypoints, with circles at the joints.

### Current Default Map

```c
position_t default_path[] = {
    {  0, 113}, { 64, 113}, { 64,  54}, {140,  54},
    {140, 174}, { 36, 174}, { 36, 216}, {288, 216},
    {288, 149}, {206, 149}, {206,  94}, {290,  94},
    {290,  28}, {180,  28}, {180,   0}
};
```

### Map Data Structure

```c
typedef struct {
    const position_t *points;
    uint8_t num_points;
    uint8_t path_width;      // visual width of the path in pixels
} map_def_t;

static const map_def_t MAPS[] = {
    { default_path, 15, 20 },
    // Additional maps added here
};
```

### Map Design Constraints

- Path must start at a screen edge (bloons enter from off-screen).
- Path should end at a screen edge (bloons exit off-screen).
- Path segments should be predominantly horizontal or vertical (diagonal segments use more CPU due to sqrt calculations).
- Path should stay within the game area (x < 280 to leave room for the sidebar).
- The `DEFAULT_PATH_WIDTH` is currently 20 pixels.

### Tower Placement Validation

Towers cannot be placed:
- On the path (check against path rectangles).
- On the sidebar UI area.
- Overlapping another tower.
- Off-screen or out of bounds.

---

## Save System

Based on the Clash Royale CE save/load pattern using TI-OS AppVars.

### AppVar Name

`BTDSAVE`

### Save Trigger

Save occurs between rounds only (not mid-round). Saving mid-round would require serializing all active bloons and projectiles, which is too complex for the initial implementation.

### Save Data Format

```c
#define SAVE_VERSION 1

typedef struct {
    uint8_t version;        // SAVE_VERSION, for forward compatibility
    uint8_t difficulty;     // 0=Easy, 1=Medium, 2=Hard
    uint8_t round;          // current round number (1-80)
    int16_t lives;
    int24_t cash;
    uint8_t map_index;      // which map is being played
    uint8_t num_towers;
    // followed by num_towers tower_save_t entries
} save_header_t;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t type;           // tower_type_t
    uint8_t upgrades[2];    // path 1 and path 2 levels
    uint8_t target_mode;    // targeting enum
    uint16_t total_invested;
} tower_save_t;
```

### Save/Load Implementation

```c
#include <fileioc.h>

bool save_game(game_t *game) {
    ti_var_t slot = ti_Open("BTDSAVE", "w");
    if (!slot) return false;

    save_header_t header = {
        .version = SAVE_VERSION,
        .difficulty = game->difficulty,
        .round = game->round,
        .lives = game->lives,
        .cash = game->cash,
        .map_index = game->map_index,
        .num_towers = game->num_towers,
    };

    ti_Write(&header, sizeof(header), 1, slot);

    for (uint8_t i = 0; i < game->num_towers; i++) {
        tower_save_t ts = {
            .x = game->towers[i].position.x,
            .y = game->towers[i].position.y,
            .type = game->towers[i].type,
            .upgrades = { game->towers[i].upgrades[0], game->towers[i].upgrades[1] },
            .target_mode = game->towers[i].target_mode,
            .total_invested = game->towers[i].total_invested,
        };
        ti_Write(&ts, sizeof(ts), 1, slot);
    }

    ti_SetArchiveStatus(true, slot);  // archive for persistence
    ti_Close(slot);
    return true;
}

bool load_game(game_t *game) {
    ti_var_t slot = ti_Open("BTDSAVE", "r");
    if (!slot) return false;

    save_header_t header;
    if (ti_Read(&header, sizeof(header), 1, slot) != 1) {
        ti_Close(slot);
        return false;
    }

    if (header.version != SAVE_VERSION) {
        ti_Close(slot);
        return false;  // incompatible save version
    }

    game->difficulty = header.difficulty;
    game->round = header.round;
    game->lives = header.lives;
    game->cash = header.cash;
    game->map_index = header.map_index;
    game->num_towers = header.num_towers;

    for (uint8_t i = 0; i < header.num_towers; i++) {
        tower_save_t ts;
        ti_Read(&ts, sizeof(ts), 1, slot);
        // Reconstruct tower from saved data
        game->towers[i].position.x = ts.x;
        game->towers[i].position.y = ts.y;
        game->towers[i].type = ts.type;
        game->towers[i].upgrades[0] = ts.upgrades[0];
        game->towers[i].upgrades[1] = ts.upgrades[1];
        game->towers[i].target_mode = ts.target_mode;
        game->towers[i].total_invested = ts.total_invested;
        game->towers[i].cooldown_timer = 0;
    }

    ti_Close(slot);
    return true;
}
```

---

## Existing Codebase Reference

The following files exist in the current codebase and will be restructured in the rewrite.

### Source Files

| File | Purpose |
|------|---------|
| `src/main.c` | Main game loop, input handling, rendering, bloon/tower/projectile init and update, collision detection |
| `src/structs.h` | All data structures: `position_t`, `bloon_t`, `tower_t`, `projectile_t`, `round_t`, `game_t`, `multi_list_t` |
| `src/list.h` / `src/list.c` | Doubly-linked list (queue) implementation used everywhere |
| `src/spacial_partition.h` / `src/spacial_partition.c` | Spatial partitioning grid for O(1)-ish collision lookups. Breaks screen into 20x20 pixel boxes. |
| `src/path.h` / `src/path.c` | Path definition, rendering, and rectangle generation from line segments. Contains the default map waypoints. |
| `src/utils.h` / `src/utils.c` | `safe_malloc` wrapper, `distance` function |

### Graphics Files

| File | Purpose |
|------|---------|
| `src/gfx/convimg.yaml` | ConvImg configuration (currently `type: c` output, single "sprites" group) |
| `src/gfx/gfx.h` | Auto-generated include file for all sprites |
| `src/gfx/*.c` / `src/gfx/*.h` | Auto-generated sprite data (dart1-5, acid, base, big_dart, global_palette) |

### Python Pipeline Scripts

| File | Purpose |
|------|---------|
| `src/image_loader.py` | Main sprite extraction script. Opens raw sprite sheet, uses `splitShapes.py` BFS to find connected components, saves each as PNG using a name mapping dictionary. |
| `src/splitShapes.py` | Core image splitting logic. `SplitImages` class with BFS-based connected component extraction. Handles background color detection and glue pixels. |
| `src/square_sprites.py` | Makes all sprites square (required for graphx rotation). Centers sprites on background-colored canvas matching the larger dimension. |

### Build Files

| File | Purpose |
|------|---------|
| `makefile` | CE SDK makefile. `NAME=BTDCE`, `COMPRESSED=YES`, `ARCHIVED=YES`, flags `-Wall -Wextra -Oz` |
| `build.sh` | Quick build: convimg + make clean + make + open bin |
| `build_dbg.sh` | Debug build: convimg + make clean + make debug + open bin |

### Known Issues in Current Code

1. **Cursor draws dart monkey sprite** (`drawCursor` uses `gfx_TransparentSprite(dart1, ...)` always). Should draw `gfx_Circle` when no tower is selected.
2. **Uses `gfx_BlitBuffer()`** instead of recommended `gfx_SwapDraw()`.
3. **Heavy use of floating-point math** in hot loops: `sqrt`, `atan2`, `cos`, `sin` in `moveTowards`, `calculate_angle`, `updateProjectiles`. Must be replaced with integer math / lookup tables.
4. **All bloons are identical** (same sprite, speed, no type differentiation). No bloon data table.
5. **All towers are identical** (same sprite, cost, stats). No tower data table.
6. **Round system is placeholder** (fixed 50 rounds, each spawning identical bloons with hardcoded count).
7. **No UI sidebar** (tower selection uses `kb_Add` key as placeholder).
8. **No upgrade system**.
9. **No save/load**.
10. **No camo/regrow/immunity system**.
11. **Sprites embedded in program** (will hit .8xp size limit as more sprites are added).
12. **Transparent color index mismatch**: `convimg.yaml` uses index 0 (black) as transparent, but `main.c` calls `gfx_SetTransparentColor(1)` (the background color). These need to be consistent.

---

## Next Steps (Priority Order)

1. **Square all sprites** (`python3 src/square_sprites.py`) -- DONE/IN PROGRESS
2. **Set up appvar system** in `convimg.yaml` (switch from embedded `type: c` to `type: appvar` output)
3. **Restructure main.c** with new game architecture (data-driven, proper game loop)
4. **Implement bloon data tables** and round system (const arrays with proper bloon types, speeds, children, immunities)
5. **Implement tower data tables** with costs, stats, damage types
6. **Build core gameplay loop**: spawn bloons from round data -> tower targeting (First mode) -> projectile firing -> collision detection -> pop (spawn children) -> cash reward
7. **Implement tower placement UI** (right sidebar with 6-8 tower icons + arrow key cursor with range circle)
8. **Fix cursor rendering** (gfx_Circle when no monkey selected; tower sprite + range when placing)
9. **Implement tower upgrade UI** (full-screen pause overlay showing 2 upgrade paths)
10. **Replace floating-point math** with integer math and lookup tables in hot loops
11. **Switch to gfx_SwapDraw()** for proper double-buffering
12. **Add difficulty selection menu** (Easy/Medium/Hard with corresponding lives and round ranges)
13. **Implement save/load system** (BTDSAVE appvar, save between rounds)
14. **Add Camo and Regrow modifiers** (modifier bitmask on bloons, tower camo detection flag, regrow timer)
15. **Add MOAB bloon type** (high HP, spawns 4 Ceramics, unique sprite and animations)
16. **Add remaining towers** beyond core 8 (Super, Wizard, Engineer, etc.)
17. **Add sandbox/free play mode** (unlimited cash, all rounds)
18. **Polish and optimize** (RLE sprites, hardware cursor, frame timing, memory optimization)

---

## Reference Links

| Resource | URL |
|----------|-----|
| CE C Toolchain Documentation | https://ce-programming.github.io/toolchain/libraries/index.html |
| Clash Royale CE (appvar pattern reference) | https://github.com/EverydayCodeNet/CLASH-ROYALE-CE/tree/rewrite |
| Sprite Art Credit (Derek200) | https://www.deviantart.com/derek200 |
| lwip-ce Library | https://github.com/cagstech/lwip-ce |
| CE C Toolchain GitHub | https://github.com/CE-Programming/toolchain |
| CEmu Emulator | https://ce-programming.github.io/CEmu/ |
