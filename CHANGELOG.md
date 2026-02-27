# Changelog

All notable changes to q2gloombot are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] — 2026-02-27

First stable release of GloomBot — AI bot players for the Quake 2 Gloom mod.

### Added

- **All 16 Gloom classes** implemented with distinct AI behaviour:
  - Human classes: Marine Light, Marine Assault, Marine Heavy, Marine Laser,
    Marine Battle, Marine Elite, Builder, Builder Advanced.
  - Alien classes: Granger, Dretch, Spiker, Kamikaze, Marauder, Dragoon,
    Guardian, Tyrant.
- **A\* pathfinding** on pre-generated nav graphs; alien bots use full 3-D
  wall/ceiling routing to bypass turret fields.
- **Building AI** — Builder and Granger bots construct and repair base
  structures following a prioritised state machine (CRITICAL → SPAWNS →
  DEFENSE → REPAIR).
- **Class upgrade system** — bots spend credits (humans) or evos (aliens) to
  advance through the tech tree at runtime.
- **Team strategy system** — five strategy modes (DEFEND, PUSH, HARASS,
  ALLIN, REBUILD) with automatic role assignment (BUILDER, ATTACKER,
  DEFENDER, SCOUT, ESCORT, HEALER).
- **Team coordination** — Reactor/Overmind awareness, map-control tracking,
  and teamwork cvars.
- **Auto-fill system** — keeps the server population at a configured minimum
  using `bot_count`/`bot_count_alien`/`bot_count_human` cvars.
- **Personality system** — each bot has a randomised personality trait set
  (aggression, caution, teamwork, patience, build focus).
- **Humanise system** — smooth aim interpolation, overshoot/drift, hesitation,
  and deliberate mistakes to give bots a human feel.
- **Chat system** — contextual taunt and callout messages.
- **Debug system** — per-flag debug output (`sv botdebug`), performance stats
  (`bot_perf`), and a full bot status dump (`sv botstatus`).
- **Console commands:** `sv addbot`, `sv removebot`, `sv listbots`,
  `sv botstatus`, `sv botkick`, `sv botpause`, `sv botdebug`,
  `sv botstrategy`, `sv navgen`, `sv botversion`.
- **Cvars:** 27 runtime-tunable cvars covering general settings, difficulty
  scaling, behaviour, navigation, and debug output.
- **Skill presets:** `skill_easy.cfg`, `skill_medium.cfg`, `skill_hard.cfg`,
  `skill_nightmare.cfg`.
- **Configuration files:** `gloombot.cfg`, `bot_config.cfg`,
  `bot_names_human.txt`, `bot_names_alien.txt`.
- **Version stamping:** `GLOOMBOT_VERSION "1.0.0"` in `bot.h`; printed on
  DLL initialisation and via `sv botversion`.
- **Documentation:** `docs/INSTALL.md`, `docs/MANUAL.md`, `README.md`.
- **CMake build system** targeting Windows (`gamex86.dll`) and Linux
  (`gamei386.so`); C99; standalone test harness (`bot_test`).
- **GPL v2 `LICENSE` file**.
- **`.gitattributes`** enforcing LF line endings.
- **`RELEASE_NOTES_v1.0.0.md`** with full feature highlights and quick start.

[1.0.0]: https://github.com/Coffee285/q2gloombot/releases/tag/v1.0.0
