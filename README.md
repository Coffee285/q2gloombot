# q2gloombot

> AI bot players for **Gloom** — the Aliens-vs-Humans class-based Quake 2 mod.
> **Current release: v1.0.0**

![Build Status](https://img.shields.io/github/actions/workflow/status/Coffee285/q2gloombot/ci.yml?branch=main&label=build)
![License](https://img.shields.io/github/license/Coffee285/q2gloombot)
![Version](https://img.shields.io/github/v/release/Coffee285/q2gloombot?label=version)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)

---

## Description

**q2gloombot** adds fully autonomous AI players to [Gloom](https://www.planetgloom.com/), the asymmetric team-based Quake 2 mod where Space Marines battle Alien Spiders. The bot code ships as a drop-in replacement for the Gloom game DLL (`gamex86.dll` on Windows, `gamei386.so` on Linux). Server operators can populate empty servers, fill uneven teams, or run entirely bot-vs-bot matches — all without modifying a single line of code. Bots support all 16 Gloom classes, build base structures, upgrade through the tech tree, and coordinate team strategy automatically.

---

## Screenshots

> *Screenshots coming soon — contributions welcome!*

---

## Features

- **All 16 Gloom classes** fully implemented with distinct AI behaviour:
  - **Humans:** Marine Light · Marine Assault · Marine Heavy · Marine Laser · Marine Battle · Marine Elite · Builder · Builder Advanced
  - **Aliens:** Granger · Dretch · Spiker · Kamikaze · Marauder · Dragoon · Guardian · Tyrant
- **Asymmetric team strategy** — bots understand each team's unique win condition (destroy Reactor vs destroy Overmind)
- **Building AI** — Builder and Granger bots construct and repair base structures following a prioritised state machine
- **Navigation system** — A\* pathfinding on pre-generated nav graphs; alien bots use 3-D wall/ceiling routing to bypass turret fields
- **Class upgrades** — bots spend credits/evos to advance through the upgrade tree
- **Skill profiles** — four preset difficulty levels (easy / medium / hard / nightmare) plus per-cvar fine-tuning
- **Team auto-fill** — automatically add bots to keep server population at a configured minimum
- **In-game console control** — add, remove, list, and inspect bots without restarting the server
- **Chat taunts** — bots send contextual chat messages
- **Zero recompile** — all behaviour is tunable at runtime through Quake 2 cvars

---

## Quick Start

1. **Download** the [latest release](https://github.com/Coffee285/q2gloombot/releases/latest) for your platform.
2. **Back up** your existing Gloom game DLL — rename `quake2/gloom/gamex86.dll` to `gamex86_original.dll`.
3. **Copy** the release files into your Gloom mod directory:
   ```
   quake2/gloom/gamex86.dll   (or gamei386.so on Linux)
   quake2/gloom/gloombot.cfg
   quake2/gloom/bot_config.cfg
   quake2/gloom/skill_*.cfg
   ```
4. **Start** the server:
   ```
   quake2 +set game gloom +exec gloombot.cfg
   ```
5. **Add a bot** from the server console:
   ```
   sv addbot alien 0.5
   ```

Bots are now playing! See the [Installation Guide](docs/INSTALL.md) for full details and the [User Manual](docs/MANUAL.md) for all commands and cvars.

---

## Requirements

| Requirement | Details |
|-------------|---------|
| Quake 2 | Any standard installation (3.20 or later recommended) |
| Gloom mod | [Gloom 1.x](https://www.planetgloom.com/) installed in `quake2/gloom/` |
| Server access | Console access to the Quake 2 dedicated server |
| OS | Windows (x86) or Linux (x86 / x86-64 with 32-bit libc) |

> To **build from source** you additionally need CMake ≥ 3.10 and a C99 compiler (GCC 7+, Clang 6+, or MSVC 2017+).

---

## Table of Contents

| Document | Description |
|----------|-------------|
| [README.md](README.md) | This file — project overview and quick start |
| [docs/INSTALL.md](docs/INSTALL.md) | Full installation guide (binary and source), platform notes, troubleshooting |
| [docs/MANUAL.md](docs/MANUAL.md) | User manual — console commands, cvar reference, class guide, nav data |

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
  bot/classes/  Per-class AI (16 classes × 1 file)
  bot/build/    Building AI (prioritised build state machine)
docs/           Documentation
maps/           Navigation node data (.nav files, one per map)
config/         Bot configuration files
```

---

## License

See [LICENSE](LICENSE) for details.
