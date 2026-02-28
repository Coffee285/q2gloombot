# Developer Guide — q2gloombot

> Internal reference for contributors working on the q2gloombot codebase — the AI bot DLL for Quake 2 Gloom.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
   - [System Diagram](#system-diagram)
   - [Frame Data Flow](#frame-data-flow)
   - [Per-Bot Think Pipeline](#per-bot-think-pipeline)
2. [Module Reference](#2-module-reference)
   - [Game Interface (`src/game/`)](#game-interface-srcgame)
   - [Gloom Definitions (`src/gloom/`)](#gloom-definitions-srcgloom)
   - [Bot Core (`src/bot/`)](#bot-core-srcbot)
   - [Navigation (`src/bot/nav/`)](#navigation-srcbotnav)
   - [Combat (`src/bot/combat/`)](#combat-srcbotcombat)
   - [Team & Strategy (`src/bot/team/`)](#team--strategy-srcbotteam)
   - [Building AI (`src/bot/build/`)](#building-ai-srcbotbuild)
   - [Class Behaviours (`src/bot/classes/`)](#class-behaviours-srcbotclasses)
3. [Key Types](#3-key-types)
4. [Adding a New Class](#4-adding-a-new-class)
5. [Adding Map Support](#5-adding-map-support)
6. [Build System](#6-build-system)
7. [Testing](#7-testing)
8. [Code Conventions](#8-code-conventions)
9. [Contributing Guidelines](#9-contributing-guidelines)

---

## 1. Architecture Overview

### System Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Quake 2 Engine                               │
│                                                                     │
│   G_RunFrame()  ──►  Bot_Frame()   (once per server frame)          │
└────────────────────────┬────────────────────────────────────────────┘
                         │
          ┌──────────────▼──────────────┐
          │        Bot Frame Loop       │
          │                             │
          │  1. BotAutofill_Frame()     │  auto-add/remove bots
          │  2. BotUpgrade_UpdateGame   │  refresh game-state snapshot
          │        State()              │
          │  3. BotTeam_Frame()         │  team coordination pass
          │  4. BotStrategy_Frame()     │  strategy & role assignment
          │  5. BotBuild_Update         │  scan & cache structures
          │        Structures()         │
          │  6. Bot_Think()  × N bots   │  per-bot AI (see below)
          └─────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  Bot_Think()  — per-bot pipeline (runs for each active bot)         │
│                                                                     │
│  ┌──────────────┐  ┌───────────────────┐  ┌────────────────────┐   │
│  │ Awareness    │─►│ Class Upgrade     │─►│ Class Behaviour    │   │
│  │ (sense env)  │  │ (spend credits/   │  │ (dispatch to       │   │
│  │              │  │  evos)            │  │  bot_class_*.c)    │   │
│  └──────────────┘  └───────────────────┘  └────────┬───────────┘   │
│                                                     │               │
│  ┌──────────────┐  ┌───────────────────┐  ┌────────▼───────────┐   │
│  │ Update       │◄─│ Finalise          │◄─│ Humanise           │   │
│  │ Timers       │  │ Movement          │  │ (aim smooth,       │   │
│  │              │  │ (send pmove)      │  │  overshoot, drift) │   │
│  └──────────────┘  └───────────────────┘  └────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### Frame Data Flow

```
G_RunFrame()
  └─► Bot_Frame()
        ├─► BotAutofill_Frame()         — add/remove bots to hit bot_count target
        ├─► BotUpgrade_UpdateGameState() — snapshot frag counts, resource totals, structure HP
        ├─► BotTeam_Frame()             — team-level awareness (Reactor/Overmind alive, spawn count)
        ├─► BotStrategy_Frame()         — pick strategy (DEFEND/PUSH/HARASS/ALLIN/REBUILD)
        ├─► BotBuild_UpdateStructures() — build structure entity cache for builder AI
        └─► for each bot:
              └─► Bot_Think(bot)
                    ├─► Bot_UpdateAwareness()    — scan for enemies, update memory
                    ├─► Bot_EvaluateClassUpgrade()— decide whether to evolve/purchase
                    ├─► Bot_RunClassBehavior()   — dispatch to class-specific AI
                    ├─► Bot_Humanize_Think()     — apply aim smoothing, drift, hesitation
                    ├─► Bot_FinalizeMovement()   — convert decisions into pmove commands
                    └─► Bot_UpdateTimers()       — tick cooldowns, memory expiry
```

### Per-Bot Think Pipeline

Each bot maintains a `bot_state_t` struct that carries all persistent state across frames. The AI state machine transitions between these states:

| State | Description |
|-------|-------------|
| `IDLE` | No objective — wait for strategy assignment |
| `PATROL` | Move between navigation nodes on a patrol route |
| `HUNT` | Seek a known or last-seen enemy position |
| `COMBAT` | Actively engaging an enemy target |
| `FLEE` | Retreating toward a safe area (low HP, outnumbered) |
| `DEFEND` | Hold a position (primary structure, choke point) |
| `ESCORT` | Follow and protect a Builder/Granger teammate |
| `BUILD` | Builder/Granger placing or repairing a structure |
| `EVOLVE` | Alien spending evos to evolve to a higher class |
| `UPGRADE` | Human spending credits at an Armory |

---

## 2. Module Reference

### Game Interface (`src/game/`)

Quake 2 game DLL interface layer. These files are derived from the id Software game SDK and provide the bridge between the engine and the bot code.

| File | Purpose |
|------|---------|
| `q_shared.h` | Shared type definitions (`vec3_t`, `qboolean`, etc.) |
| `game.h` | Game import/export function tables (`game_import_t`, `game_export_t`) |
| `g_local.h` | Internal game types (`edict_t`, `gclient_t`, `gitem_t`) |
| `g_main.c` | DLL entry point (`GetGameAPI()`), `G_RunFrame()` — calls `Bot_Frame()` |

> **Do not modify** these files unless adapting to a new Gloom version.

### Gloom Definitions (`src/gloom/`)

Gloom-specific data definitions shared across the bot code.

| File | Purpose |
|------|---------|
| `gloom_classes.h` | `gloom_class_t` enum (16 classes, 0–7 human, 8–15 alien), class metadata |
| `gloom_classes.c` | Class property tables (name, tier, cost, HP, speed, abilities) |
| `gloom_structs.h` | Structure types (`gloom_struct_type_t`), structure property data |

**Key enum — `gloom_class_t`:**

| Index | Human | Index | Alien |
|-------|-------|-------|-------|
| 0 | Marine Light | 8 | Granger |
| 1 | Marine Assault | 9 | Dretch |
| 2 | Marine Heavy | 10 | Spiker |
| 3 | Marine Laser | 11 | Kamikaze |
| 4 | Marine Battle | 12 | Marauder |
| 5 | Marine Elite | 13 | Dragoon |
| 6 | Builder | 14 | Guardian |
| 7 | Builder Advanced | 15 | Tyrant |

### Bot Core (`src/bot/`)

Central bot management, lifecycle, and shared subsystems.

| File | Purpose | Key Functions |
|------|---------|---------------|
| `bot.h` | Master header — `bot_state_t`, all enums, constants (`MAX_BOTS`, `BOT_THINK_RATE`) | — |
| `bot_main.c` | Lifecycle and frame loop | `Bot_Init()`, `Bot_Shutdown()`, `Bot_Connect()`, `Bot_Disconnect()`, `Bot_Frame()`, `Bot_Think()` |
| `bot_commands.c` | Console command handlers (`sv addbot`, `sv removebot`, etc.) | `Bot_ServerCommand()` |
| `bot_config.c` | Configuration file loading | `Bot_LoadConfig()` |
| `bot_autofill.c` | Auto-fill system — keeps server at `bot_count` players | `BotAutofill_Frame()` |
| `bot_cvars.c` / `.h` | Cvar declarations and registration (27 cvars) | `Bot_RegisterCvars()` |
| `bot_upgrade.c` / `.h` | Class upgrade decision engine | `BotUpgrade_UpdateGameState()`, `Bot_ChooseClass()`, `BotUpgrade_ShouldUpgrade()` |
| `bot_personality.c` / `.h` | Per-bot personality traits (aggression, caution, teamwork, patience, build_focus) | `Bot_Personality_Init()` |
| `bot_humanize.c` / `.h` | Aim smoothing, overshoot, drift, hesitation, speed reduction | `Bot_Humanize_Init()`, `Bot_Humanize_Think()` |
| `bot_chat.c` / `.h` | Contextual chat messages | `Bot_Chat_OnKill()`, `Bot_Chat_OnDeath()`, `Bot_Chat_OnTeamWin()`, `Bot_Chat_OnSpawn()` |
| `bot_debug.c` / `.h` | Debug flags, state logging, performance stats | Flags: `BOT_DEBUG_STATE`, `BOT_DEBUG_NAV`, `BOT_DEBUG_COMBAT`, `BOT_DEBUG_BUILD`, `BOT_DEBUG_STRATEGY`, `BOT_DEBUG_UPGRADE` |
| `bot_safety.h` | Safe memory and bounds-checking macros | — |

### Navigation (`src/bot/nav/`)

A\* pathfinding on pre-generated nav graphs. Alien bots support 3-D wall/ceiling node traversal.

| File | Purpose | Key Functions |
|------|---------|---------------|
| `bot_nav.c` / `.h` | Path planning and movement | `BotNav_Init()`, `BotNav_LoadMap()`, `BotNav_FindPath()`, `BotNav_MoveTowardGoal()`, `BotNav_UpdateWallWalk()` |
| `bot_nodes.c` / `.h` | Node graph storage, loading/saving `.nav` files | `BotNodes_Load()`, `BotNodes_Save()`, `BotNodes_AutoGenerate()` |

**Constants:** `BOT_MAX_PATH_NODES` (256), `BOT_INVALID_NODE` (-1)

**Configuration cvars:** `bot_nav_autogen`, `bot_nav_show`, `bot_nav_density`

### Combat (`src/bot/combat/`)

Target selection and weapon firing with asymmetric priority rules per team.

| File | Purpose | Key Functions |
|------|---------|---------------|
| `bot_combat.c` / `.h` | Combat think loop, target picking, aiming, firing | `BotCombat_Think()`, `BotCombat_PickTarget()`, `BotCombat_AimAtTarget()`, `BotCombat_Fire()`, `BotCombat_AimError()` |

**Targeting priorities:**
- **Human:** Overmind → Grangers → alien structures → alien players
- **Alien:** Reactor → human Builders → Turrets → Marines

### Team & Strategy (`src/bot/team/`)

Team-level coordination, strategy selection, and role assignment.

| File | Purpose | Key Functions |
|------|---------|---------------|
| `bot_team.c` / `.h` | Team state tracking, role assignment | `BotTeam_Init()`, `BotTeam_Frame()`, `BotTeam_AssignRoles()`, `BotTeam_ReactorAlive()`, `BotTeam_OvmindAlive()` |
| `bot_strategy.c` / `.h` | Strategy state machine (5 strategies), role distribution | `BotStrategy_Frame()`, `BotStrategy_Evaluate()` |
| `bot_teamwork.c` | Cooperative behaviours (escort, group attacks) | — |
| `bot_mapcontrol.c` | Map-area control tracking | — |

**Strategies (`bot_strategy_t`):**

| Strategy | When Selected | Typical Role Mix |
|----------|---------------|------------------|
| `DEFEND` | Primary structure under threat | Defenders + Builder |
| `PUSH` | Team has resource/number advantage | Attackers + Escort |
| `HARASS` | Stalemate — probe enemy defenses | Scouts + Attackers |
| `ALLIN` | Enemy primary structure is weak | All Attackers |
| `REBUILD` | Own primary structure or spawns destroyed | Builders + Defenders |

**Roles (`bot_role_t`):** `BUILDER`, `ATTACKER`, `DEFENDER`, `SCOUT`, `ESCORT`, `HEALER`

### Building AI (`src/bot/build/`)

Builder/Granger construction and repair logic.

| File | Purpose | Key Functions |
|------|---------|---------------|
| `bot_build.c` / `.h` | Build state machine, structure cache, repair logic | `BotBuild_Think()`, `BotBuild_UpdateStructures()` |
| `bot_build_placement.c` | Placement position selection (choke points, coverage) | `BotBuild_FindPlacement()` |

**Build priority order:**

1. **CRITICAL** — Primary structure missing (Reactor / Overmind) → build immediately
2. **SPAWNS** — No spawn points alive → place Telenode / Egg
3. **DEFENSE** — Expand perimeter (Turrets / Acid Tubes at choke points)
4. **REPAIR** — Restore health to structures below `BOT_BUILD_REPAIR_THRESHOLD` (50%)

**Constants:** `BOT_BUILD_REPAIR_THRESHOLD` (0.50), `BOT_BUILD_MAX_STRUCTS` (64)

### Class Behaviours (`src/bot/classes/`)

One source file per Gloom class, plus shared team logic.

| File | Purpose |
|------|---------|
| `bot_human_common.c` | Shared human behaviours (retreat, ammo management, range engagement) |
| `bot_alien_common.c` | Shared alien behaviours (wall-walk routing, melee patterns, swarm logic) |
| `bot_class_marine_light.c` | Marine Light (class 0) — basic combat AI |
| `bot_class_marine_assault.c` | Marine Assault (class 1) — pulse rifle engagement |
| `bot_class_marine_heavy.c` | Marine Heavy (class 2) — flamethrower/rocket behaviour |
| `bot_class_marine_laser.c` | Marine Laser (class 3) — energy weapon AI |
| `bot_class_marine_battle.c` | Marine Battle (class 4) — battlesuit tactics |
| `bot_class_marine_elite.c` | Marine Elite (class 5) — end-game loadout |
| `bot_class_builder.c` | Builder (class 6) — delegates to `bot_build.c` |
| `bot_class_builder_adv.c` | Builder Advanced (class 7) — extended build orders |
| `bot_class_granger.c` | Granger (class 8) — alien builder |
| `bot_class_dretch.c` | Dretch (class 9) — wall-walk biter |
| `bot_class_spiker.c` | Spiker (class 10) — ranged projectile |
| `bot_class_kamikaze.c` | Kamikaze (class 11) — suicide charge on structures |
| `bot_class_marauder.c` | Marauder (class 12) — pounce attacker |
| `bot_class_dragoon.c` | Dragoon (class 13) — barb volley |
| `bot_class_guardian.c` | Guardian (class 14) — tank |
| `bot_class_tyrant.c` | Tyrant (class 15) — fortress-breaker |

> **Legacy class files:** The directory also contains stub files for legacy or alternate class names (e.g. `bot_class_grunt.c`, `bot_class_biotech.c`, `bot_class_engineer.c`, `bot_class_hatchling.c`, `bot_class_breeder.c`). These are kept for backward compatibility with older Gloom versions and map scripts. They are not part of the core 16-class enum but are still compiled into the DLL.

---

## 3. Key Types

| Type | Defined In | Description |
|------|------------|-------------|
| `bot_state_t` | `bot.h` | Master per-bot state struct — embeds nav state, combat state, build state, `bot_personality_t`, `bot_humanize_state_t`, AI state, role, class, timers, and enemy memory |
| `gloom_class_t` | `gloom_classes.h` | Enum for all 16 Gloom classes (0–7 human, 8–15 alien) |
| `bot_ai_state_t` | `bot.h` | AI state machine enum: `IDLE`, `PATROL`, `HUNT`, `COMBAT`, `FLEE`, `DEFEND`, `ESCORT`, `BUILD`, `EVOLVE`, `UPGRADE` |
| `bot_strategy_t` | `bot_strategy.h` | Team strategy enum: `DEFEND`, `PUSH`, `HARASS`, `ALLIN`, `REBUILD` |
| `bot_role_t` | `bot_strategy.h` | Per-bot role enum: `BUILDER`, `ATTACKER`, `DEFENDER`, `SCOUT`, `ESCORT`, `HEALER` |
| `bot_personality_t` | `bot.h` | Personality traits: `aggression`, `caution`, `teamwork`, `patience`, `build_focus` (all `float`, 0.0–1.0) |
| `bot_humanize_state_t` | `bot.h` | Humanisation state: aim angles, overshoot, drift, hesitation, mistake counter |
| `gloom_struct_type_t` | `gloom_structs.h` | Structure type enum (Reactor, Overmind, Telenode, Egg, Turret, Acid Tube, etc.) |

**Limits:**
- `MAX_BOTS` — 16 concurrent bots
- `BOT_MAX_PATH_NODES` — 256 nodes per path
- `BOT_MAX_REMEMBERED_ENEMIES` — 8 enemies in memory
- `BOT_ENEMY_MEMORY_TIME` — 10 seconds until an enemy is forgotten

---

## 4. Adding a New Class

Follow these steps to add AI support for a new Gloom class (e.g. after a Gloom mod update adds one).

### Step 1 — Update the class enum

Edit `src/gloom/gloom_classes.h` and add the new class to the `gloom_class_t` enum. Place it at the end of the appropriate team block (human 0–7, alien 8–15). Update the count constant if present.

### Step 2 — Add class properties

Edit `src/gloom/gloom_classes.c` and add an entry to the class property table with the new class's name, tier, cost, HP, speed, and ability flags.

### Step 3 — Create the class behaviour file

Create `src/bot/classes/bot_class_<name>.c`. Use an existing class file as a template. At minimum, implement:

```c
#include "bot.h"
#include "bot_combat.h"
#include "bot_nav.h"

/*
 * BotClass_<Name>_Think -- per-frame AI for the <Name> class
 */
void BotClass_<Name>_Think(bot_state_t *bs)
{
    /* Class-specific AI logic here.
     * Call into bot_human_common.c or bot_alien_common.c
     * for shared team behaviour. */
}
```

### Step 4 — Register the class dispatch

Edit `bot_main.c` (inside `Bot_RunClassBehavior()`) and add a `case` for the new `gloom_class_t` value that calls `BotClass_<Name>_Think()`.

### Step 5 — Add to the build system

Edit `CMakeLists.txt` and add the new `.c` file to the `BOT_CLASS_SOURCES` list.

### Step 6 — Update the upgrade engine

Edit `src/bot/bot_upgrade.c` — add the new class to the upgrade evaluation logic in `Bot_ChooseClass()` so bots know when to upgrade/evolve into it.

### Step 7 — Test

```bash
cmake --build build
# In-game: sv addbot <team> 0.5
# Verify with: sv botstatus <bot_name>
# Enable debug: sv botdebug state
```

### Step 8 — Update documentation

Add the new class to the tables in `docs/README.md`, `docs/MANUAL.md`, and this file.

---

## 5. Adding Map Support

Bots use pre-computed navigation graphs stored as `.nav` files in `maps/`.

### Auto-generation

Set `bot_nav_autogen 1` (the default). When a map loads without a `.nav` file, the bot system flood-fills walkable space from spawn points to generate a basic nav graph.

### Manual nav file creation

For higher quality navigation, create a hand-tuned `.nav` file:

1. **Generate a base graph** — load the map with `bot_nav_autogen 1` and run `sv navgen`.
2. **Enable visualisation** — set `bot_nav_show 1` to render nodes in-world.
3. **Edit the file** — open `maps/<mapname>.nav` in a text editor. The format is:

```
# <mapname>.nav — navigation graph
# Lines starting with # are comments
node <id> <x> <y> <z> <flags>
link <from_id> <to_id> [<flags>]
```

**Node flags:**
- `0` — normal ground node
- `1` — wall node (alien wall-walk only)
- `2` — ceiling node (alien wall-walk only)
- `4` — choke point (strategic importance)

**Link flags:**
- `0` — bidirectional walk
- `1` — jump required
- `2` — drop (one-way)

4. **Place the file** — save to `maps/<mapname>.nav` in the project, or install to `quake2/gloom/maps/` on the server.

### Tuning nav density

Adjust `bot_nav_density` (default 128 Quake units). Lower values produce a denser graph with better coverage but higher memory use. Range: 64–256.

---

## 6. Build System

The project uses CMake 3.10+ targeting C99. The output is a shared library that Quake 2 loads as the game DLL.

### Quick build

```bash
# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Output: build/gamei386.so

# Windows (Visual Studio 2022, 32-bit)
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
# Output: build/Release/gamex86.dll
```

### Build targets

| Target | Output | Description |
|--------|--------|-------------|
| `gamei386` / `gamex86` | `gamei386.so` / `gamex86.dll` | Main game DLL |
| `bot_test` | `bot_test` executable | Test harness (standalone, no engine) |

### Debug build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Debug builds enable assertions and disable optimisations. Combine with `bot_debug 5` in-game for maximum diagnostic output.

### Compiler requirements

| Compiler | Minimum Version |
|----------|----------------|
| GCC | 7.0 |
| Clang | 6.0 |
| MSVC | 2017 (15.x) |

> **Note:** Quake 2 is a 32-bit application. On 64-bit Linux, install `gcc-multilib` and the CMakeLists.txt will pass `-m32` automatically.

---

## 7. Testing

### Test harness

The project includes a standalone test executable that exercises bot subsystems without the Quake 2 engine.

```bash
cmake --build build --target bot_test
./build/bot_test
```

The test harness (`test/bot_test.c`) provides:
- A minimal assertion framework (`ASSERT_TRUE`, `ASSERT_EQ`, etc.)
- Mock implementations of Quake 2 engine functions
- Unit tests for nav, combat, upgrade, and personality subsystems

Tests are compiled with the `BOT_TEST_MODE` preprocessor define, which stubs out engine dependencies.

### In-game debugging

| Tool | Usage |
|------|-------|
| `sv botdebug <flag>` | Toggle per-subsystem debug output (`state`, `nav`, `combat`, `build`, `strategy`, `upgrade`, `all`, `none`) |
| `sv botstatus [name]` | Dump full state for one or all bots |
| `sv botstrategy [team]` | Show current strategy and role assignments |
| `bot_debug <0–5>` | Set global debug verbosity |
| `bot_debug_target "<name>"` | Restrict debug output to a single bot |
| `bot_perf 1` | Print per-frame timing statistics |
| `bot_nav_show 1` | Render nav nodes in-world |

---

## 8. Code Conventions

### Language and standard

- **C99** throughout. No C++ or compiler-specific extensions.
- All source files use **LF line endings** (enforced by `.gitattributes`).

### Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Functions (public) | `Module_VerbNoun()` | `BotNav_FindPath()`, `BotCombat_PickTarget()` |
| Functions (static) | `module_verb_noun()` | `nav_score_path()` |
| Types (structs) | `snake_case_t` | `bot_state_t`, `bot_personality_t` |
| Types (enums) | `snake_case_t` | `bot_ai_state_t`, `bot_strategy_t` |
| Enum values | `UPPER_SNAKE_CASE` | `STRATEGY_DEFEND`, `BOT_DEBUG_NAV` |
| Constants / macros | `UPPER_SNAKE_CASE` | `MAX_BOTS`, `BOT_THINK_RATE` |
| Cvars | `bot_snake_case` | `bot_skill`, `bot_nav_density` |
| Local variables | `snake_case` | `best_target`, `path_length` |

### File layout

Each `.c` file follows this order:

1. File header comment (purpose, public API summary)
2. `#include` directives (own header first, then project headers, then system headers)
3. Static (file-scope) variables
4. Static helper functions
5. Public functions

### Header guards

Use `#ifndef` / `#define` / `#endif` guards. The guard name matches the file name in upper-case:

```c
#ifndef BOT_NAV_H
#define BOT_NAV_H
/* ... */
#endif /* BOT_NAV_H */
```

### Comments

- File-level block comments for purpose and public API overview.
- Function-level comments for non-trivial public functions.
- Inline comments only where the code is not self-explanatory.
- Use `/* C89-style block comments */` — not `//` line comments.

### Memory and safety

- Use the macros in `bot_safety.h` for bounds checking.
- Never use `malloc`/`free` directly — all bot memory is statically allocated within `bot_state_t` arrays.
- Validate all entity pointers (`edict_t *`) before dereferencing — entities can be freed between frames.

---

## 9. Contributing Guidelines

### Getting started

1. Fork the repository and clone your fork.
2. Create a feature branch from `main`:
   ```bash
   git checkout -b feature/my-change
   ```
3. Build and run the test harness to verify the baseline:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --target bot_test
   ./build/bot_test
   ```

### Making changes

- Keep commits focused — one logical change per commit.
- Follow the [code conventions](#8-code-conventions) above.
- Add or update tests in `test/bot_test.c` for new logic.
- Update documentation if your change affects the public API, cvars, or commands.

### Commit messages

Use imperative mood, capitalise the first word, and keep the summary line under 72 characters:

```
Add wall-walk support for Marauder class

Marauder bots now use wall/ceiling nodes when routing to avoid
turret line-of-sight, matching the Dretch wall-walk behaviour.
```

### Pull requests

- Target the `main` branch.
- Include a clear description of what changed and why.
- Ensure the test harness passes.
- Verify in-game with at least one bot per affected class.

### Bug reports

Open an issue with:
- Quake 2 engine version and Gloom mod version.
- Operating system and compiler.
- Steps to reproduce.
- Relevant console output (use `bot_debug 3` or higher).

### Code of conduct

Be respectful and constructive. This is a hobby project — contributors donate their time.
