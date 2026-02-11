# BTD CE

Bloons Tower Defense for the TI-84 Plus CE calculator. Inspired by BTD5/BTD6 by Ninja Kiwi, rebuilt from scratch in C.

## Features

### Towers
8 fully upgradeable towers, each with 2 upgrade paths (4 levels per path):

| Tower | Base Cost | Description |
|-------|-----------|-------------|
| Dart Monkey | $200 | Fast, cheap all-rounder with sharp darts |
| Tack Shooter | $280 | Fires tacks in all directions |
| Sniper Monkey | $350 | Infinite-range hitscan, hits one bloon instantly |
| Bomb Shooter | $525 | Explosive splash damage |
| Boomerang Monkey | $325 | Throws boomerangs with high pierce |
| Ninja Monkey | $500 | Fast attack, detects camo, homing shurikens |
| Ice Monkey | $300 | Area freeze, slows and damages nearby bloons |
| Glue Gunner | $250 | Slows bloons, corrosive glue deals damage over time |

### Upgrade Abilities
- Splash damage (Bomb Shooter, Juggernaut)
- Homing projectiles (Seeking Shuriken, Missile Launcher)
- Damage over time (Corrosive Glue, Bloon Dissolver, Bloon Liquefier)
- Stun on hit (Bloon Impact, Flash Bomb)
- Aura slow (Arctic Wind)
- MOAB bonus damage (MOAB Mauler, Assassin, Eliminator)
- Permafrost, Distraction, Glue Soak, Camo stripping

### Bloons
12 bloon types from Red to MOAB with full pop hierarchy:

Red > Blue > Green > Yellow > Pink > Black/White > Lead > Zebra > Rainbow > Ceramic > MOAB

- **Camo bloons** - invisible to most towers (Ninja detects by default, others need upgrades)
- **Regrow bloons** - regenerate to their original type over time
- Unique sprites for all camo, regrow, and camo+regrow variants
- Bloon immunities: Lead (sharp-immune), Black (explosion-immune), White (freeze-immune)

### Game Modes
- **3 difficulties** - Easy (40 rounds), Medium (60 rounds), Hard (80 rounds)
- **Freeplay** - endless scaling rounds after victory
- **Sandbox** - unlimited money for testing
- **Spectate** - watch the action after game over or victory
- **2x speed** - fast-forward toggle

### Targeting
4 targeting modes for each tower, cycled with the Mode key:
- **First** - targets the bloon furthest along the path
- **Last** - targets the bloon closest to the start
- **Strong** - targets the highest RBE bloon in range
- **Close** - targets the nearest bloon to the tower

### Other Features
- Auto-save on round completion and menu exit
- Resume from save on launch
- Tower selling at 70% of invested value
- Difficulty-adjusted pricing (Easy 0.85x, Hard 1.08x, rounded to nearest $5)
- Hard mode income multiplier (0.8x)
- Spatial partitioning for efficient collision detection
- Deferred bloon spawn queue to prevent memory overflow from pop cascades

## Controls

| Key | Action |
|-----|--------|
| Arrow Keys | Move cursor |
| Enter | Place tower / Select tower / Confirm |
| + | Open tower buy menu |
| - | Sell tower (in upgrade screen) |
| 2nd | Start round / Toggle 2x speed |
| Mode | Cycle target mode from game screen or upgrade menu |
| Del | Back / Cancel |
| Clear | Save and return to title screen |

## Installation

1. Connect your TI-84 Plus CE to your computer
2. Open TI Connect CE (or another transfer tool)
3. Transfer all 5 files to your calculator:
   - `BTDCE.8xp`
   - `BTDTW1.8xv`
   - `BTDTW2.8xv`
   - `BTDBLN.8xv`
   - `BTDUI.8xv`
4. Run `BTDCE` from the programs menu (`[prgm]` key)

> **OS 5.5.1+ users:** You will need [arTIfiCE](https://yvantt.github.io/arTIfiCE/) to run assembly programs.

## Building from Source

### Prerequisites
- [CE C SDK](https://ce-programming.github.io/toolchain/) installed (set `CEDEV` environment variable)
- Python 3 with Pillow (`pip install -r src/requirements.txt`)

### Build Steps

```bash
# Extract and process sprite images
cd src && python3 image_loader.py && cd -

# Generate sprite appvars
cd src/gfx && convimg && cd -

# Compile
make
```

Output files are in `bin/`.

## Credits

- Original game by [Ninja Kiwi](https://ninjakiwi.com/)
- Adapted for TI-84 Plus CE by Everyday Code, Elijah Baraw (2026)
- Art by [Derek200](https://www.deviantart.com/derek200) on DeviantArt
