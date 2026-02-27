# q2gloombot — Quake 2 Gloom Bot

AI bot players for **Gloom**, the Aliens-vs-Humans class-based Quake 2 mod.

The bot code compiles as a Quake 2 game DLL (`gamex86.dll` on Windows, `gamei386.so` on Linux) that replaces the standard game logic.

---

## Game Overview

Gloom is an asymmetric, team-based mod where **Space Marines** fight **Alien Spiders**.

| | Humans (Marines) | Aliens |
|---|---|---|
| **Resource** | Credits (kills → buy weapons/armour) | Evos (kills → evolve to higher class) |
| **Builder** | Technician/Builder | Granger |
| **Primary structure** | Reactor | Overmind |
| **Spawn point** | Telenode | Egg |
| **Win condition** | Destroy the Overmind | Destroy the Reactor |

---

## Project Structure

```
src/
  game/         Quake 2 game DLL interface (q_shared.h, game.h, g_local.h, g_main.c)
  gloom/        Gloom mod definitions (classes, structures)
  bot/          Bot AI core
  bot/nav/      Pathfinding (A* on nav graph; alien 3-D wall-walk aware)
  bot/combat/   Target selection and firing (asymmetric priority rules)
  bot/team/     Team coordination (Reactor/Overmind awareness, role assignment)
  bot/classes/  Per-class AI tweaks (16 classes × 1 file)
  bot/build/    Building AI (prioritised build state machine)
docs/           Documentation
maps/           Navigation node data (.nav files, one per map)
config/         Bot configuration
```

---

## Classes

### Human (upgrade via credits)

| # | Class | Role |
|---|-------|------|
| 0 | Marine_Light | Basic marine; pistol/blaster |
| 1 | Marine_Assault | Pulse rifle |
| 2 | Marine_Heavy | Flamethrower / rockets |
| 3 | Marine_Laser | Energy weapons |
| 4 | Marine_Battle | Powered battlesuit |
| 5 | Marine_Elite | Top-tier loadout |
| 6 | Builder | Constructs base structures |
| 7 | Builder_Adv | Advanced construction |

### Alien (evolution via evos)

| # | Class | Tier | Notes |
|---|-------|------|-------|
| 8 | Granger | Builder | Cannot wall-walk |
| 9 | Dretch | 1 | Fast, tiny biter; wall-walks |
| 10 | Spiker | 2 | Ranged biological projectile |
| 11 | Kamikaze | 2 | Explosive suicide charge |
| 12 | Marauder | 3 | Agile pounce attacker |
| 13 | Dragoon | 3 | Heavy barb volley |
| 14 | Guardian | 4 | High-HP tank |
| 15 | Tyrant | 5 | Ultimate fortress-breaker |

---

## AI Behaviour

### Build priority (both builder classes)

1. **CRITICAL** — Primary structure missing (Reactor / Overmind) → build immediately.
2. **SPAWNS** — No spawn points alive → place Telenode / Egg.
3. **DEFENSE** — Expand perimeter (Turrets at choke points / Acid Tubes at corridors).
4. **REPAIR** — Restore health to damaged structures.

### Alien combat targeting

`Reactor → human Builders → Turrets → Marines`

Alien bots prefer wall/ceiling routes to bypass turret fields.  
Kamikaze class closes to point-blank and detonates on structures.

### Human combat targeting

`Overmind → Grangers → alien structures → alien players`

Human bots engage at maximum effective range and fall back toward the Reactor when below 20 % health.

---

## Building

### Prerequisites

- CMake ≥ 3.10
- GCC / Clang (Linux) or MSVC (Windows)

### Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Output: build/gamei386.so
```

### Windows

```bat
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
:: Output: build/Release/gamex86.dll
```

Copy the resulting DLL into your Quake 2 `gloom/` directory.

---

## Navigation

Bot navigation data is stored in `maps/<mapname>.nav`.  
These files are not shipped; they must be generated with the node-editor tool (TBD).  
Without a `.nav` file bots will roam randomly.

---

## Adding Bots In-Game

```
sv addbot human 0.5     # add a human bot at medium skill
sv addbot alien 0.8     # add an alien bot at high skill
sv listbots             # show active bots
sv removebot Bot_Dretch_00
```
