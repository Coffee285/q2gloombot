# User Manual — q2gloombot

> Complete reference for server operators running q2gloombot on a Quake 2 Gloom server.

---

## Table of Contents

1. [Console Command Reference](#1-console-command-reference)
2. [Cvar Reference](#2-cvar-reference)
   - [General](#general)
   - [Difficulty Cvars](#difficulty-cvars)
   - [Behaviour Cvars](#behaviour-cvars)
   - [Navigation Cvars](#navigation-cvars)
   - [Debug Cvars](#debug-cvars)
3. [Skill Presets](#3-skill-presets)
4. [Class Guide](#4-class-guide)
   - [Human Classes](#human-classes)
   - [Alien Classes](#alien-classes)
5. [Team Strategy & AI Behaviour](#5-team-strategy--ai-behaviour)
6. [Navigation Data](#6-navigation-data)
7. [Configuration Files](#7-configuration-files)
8. [Bot Names](#8-bot-names)

---

## 1. Console Command Reference

All commands are issued from the **server console** (or an RCON connection) using the `sv` prefix.

| Command | Arguments | Description |
|---------|-----------|-------------|
| `sv addbot` | `[team] [skill]` | Add a bot. `team` is `alien` or `human` (default: random). `skill` is `0.0`–`1.0` (default: `bot_skill`). |
| `sv removebot` | `<name\|all>` | Remove a bot by name, or remove all bots with `all`. |
| `sv listbots` | *(none)* | Print a table of all currently active bots. |
| `sv botstatus` | `[name]` | Detailed status of a named bot, or all bots if no name given. |
| `sv botkick` | `<name>` | Alias for `sv removebot <name>`. |
| `sv botpause` | *(none)* | Toggle bot AI thinking on/off (bots freeze in place when paused). |
| `sv botdebug` | `<flag\|all\|none>` | Toggle a debug output flag. See [Debug Flags](#debug-flags) below. |
| `sv botstrategy` | `[team]` | Print the current team strategy state for `alien`, `human`, or both. |
| `sv navgen` | *(none)* | Auto-generate navigation nodes for the current map (requires `bot_nav_autogen 1`). |

### Examples

```
sv addbot alien 0.5          // add an alien bot at medium skill
sv addbot human              // add a human bot at the default skill level
sv addbot                    // add a bot to a random team
sv removebot Xenomorph       // remove the bot named "Xenomorph"
sv removebot all             // remove every bot from the server
sv listbots                  // list all active bots
sv botstatus Sgt.Rock        // show detailed info for "Sgt.Rock"
sv botpause                  // freeze all bots
sv botdebug nav              // toggle navigation debug output
sv botdebug all              // enable all debug output
sv botdebug none             // disable all debug output
sv botstrategy alien         // show current alien team strategy
```

### Debug Flags

Used with `sv botdebug <flag>`:

| Flag | Description |
|------|-------------|
| `state` | Bot state machine transitions |
| `nav` | Pathfinding and node traversal |
| `combat` | Target selection and weapon firing |
| `build` | Builder/Granger structure placement decisions |
| `strategy` | Team strategy updates |
| `upgrade` | Class upgrade decisions |
| `all` | Enable all flags simultaneously |
| `none` | Disable all flags |

---

## 2. Cvar Reference

Cvars control all bot behaviour at runtime. Set them in `gloombot.cfg` for persistence, or type them directly in the server console for immediate effect.

**Syntax:** `<cvarname> <value>`  
**Example:** `bot_skill 0.7`

### General

| Cvar | Default | Range | Description |
|------|---------|-------|-------------|
| `bot_enable` | `1` | `0`–`1` | Master switch. `0` disables all bot logic. |
| `bot_skill` | `0.5` | `0.0`–`1.0` | Default skill level for newly added bots. |
| `bot_skill_min` | `0.2` | `0.0`–`1.0` | Lower bound when `bot_skill_random` is enabled. |
| `bot_skill_max` | `0.8` | `0.0`–`1.0` | Upper bound when `bot_skill_random` is enabled. |
| `bot_skill_random` | `1` | `0`–`1` | Randomise each new bot's skill between `bot_skill_min` and `bot_skill_max`. |
| `bot_count` | `0` | `0`–`32` | Auto-fill server to this many total players (humans + bots). `0` = disabled. |
| `bot_count_alien` | `0` | `0`–`16` | Override: keep this many alien bots in the server. `0` = use `bot_count`. |
| `bot_count_human` | `0` | `0`–`16` | Override: keep this many human bots in the server. `0` = use `bot_count`. |
| `bot_think_interval` | `100` | `50`–`200` | Milliseconds between bot AI frames. Lower values are more responsive but use more CPU. |
| `bot_chat` | `1` | `0`–`1` | Allow bots to send chat messages (taunts, callouts). |

### Difficulty Cvars

| Cvar | Default | Range | Description |
|------|---------|-------|-------------|
| `bot_aim_skill_scale` | `1.0` | `0.1`–`2.0` | Multiplier on aiming accuracy. `<1` = less accurate, `>1` = more accurate. |
| `bot_reaction_scale` | `1.0` | `0.1`–`3.0` | Multiplier on reaction time. Higher = slower reactions. |
| `bot_awareness_range` | `1000` | `200`–`2000` | Maximum distance (in Quake units) at which bots can spot enemies. |
| `bot_hearing_range` | `800` | `200`–`1500` | Maximum distance at which bots react to sounds. |
| `bot_fov` | `120` | `60`–`360` | Bot field of view for visual detection, in degrees. |
| `bot_strafe_skill` | `0.5` | `0.0`–`1.0` | How well bots dodge during combat. `0` = no strafing, `1` = expert dodging. |

### Behaviour Cvars

| Cvar | Default | Range | Description |
|------|---------|-------|-------------|
| `bot_aggression` | `0.5` | `0.0`–`1.0` | Global aggression modifier. `0` = defensive/passive, `1` = always attacking. |
| `bot_teamwork` | `0.7` | `0.0`–`1.0` | Tendency to follow team strategy vs acting solo. |
| `bot_build_priority` | `0.8` | `0.0`–`1.0` | How much Builder/Granger bots prioritise construction over fighting. |
| `bot_upgrade_enabled` | `1` | `0`–`1` | Allow bots to spend credits/evos to upgrade classes. |
| `bot_upgrade_delay` | `5` | `0`–`30` | Seconds to wait after earning a frag before attempting a class upgrade. |
| `bot_taunt` | `1` | `0`–`1` | Bots send taunt messages after kills. |

### Navigation Cvars

| Cvar | Default | Range | Description |
|------|---------|-------|-------------|
| `bot_nav_autogen` | `1` | `0`–`1` | Automatically generate navigation nodes when no `.nav` file is found for the current map. |
| `bot_nav_show` | `0` | `0`–`1` | Render navigation nodes in-world for debugging (requires a client connection). |
| `bot_nav_density` | `128` | `64`–`256` | Spacing (in Quake units) between auto-generated navigation nodes. Smaller = denser graph, more memory. |

### Debug Cvars

| Cvar | Default | Range | Description |
|------|---------|-------|-------------|
| `bot_debug` | `0` | `0`–`5` | Debug output verbosity. `0` = silent, `5` = extremely verbose. |
| `bot_debug_target` | `""` | string | Restrict debug output to a single named bot. Empty string = all bots. |
| `bot_perf` | `0` | `0`–`1` | Print performance statistics (think-frame timing) each second. |

---

## 3. Skill Presets

Four preset configuration files are included in `config/`. Apply one from the server console or add `exec <file>` to your `gloombot.cfg`.

| File | `bot_skill` | `bot_reaction_scale` | `bot_aim_skill_scale` | `bot_awareness_range` |
|------|-------------|---------------------|-----------------------|-----------------------|
| `skill_easy.cfg` | `0.2` | `2.0` | `0.5` | `600` |
| `skill_medium.cfg` | `0.5` | `1.0` | `1.0` | `1000` |
| `skill_hard.cfg` | `0.8` | `0.6` | `1.5` | `1500` |
| `skill_nightmare.cfg` | `1.0` | `0.3` | `2.0` | `2000` |

**Example — apply medium difficulty at runtime:**
```
exec skill_medium.cfg
```

---

## 4. Class Guide

Bots automatically choose and upgrade classes based on their team role and available resources.

### Human Classes

Human bots earn **credits** from kills and spend them to upgrade through the marine tech tree.

| # | Class | Role | Notes |
|---|-------|------|-------|
| 0 | Marine Light | Basic rifleman | Starting class; pistol/blaster |
| 1 | Marine Assault | Front-line attacker | Pulse rifle; first upgrade target |
| 2 | Marine Heavy | Area-denial | Flamethrower or rockets |
| 3 | Marine Laser | Energy specialist | Long-range energy weapons |
| 4 | Marine Battle | Armoured assault | Powered battlesuit |
| 5 | Marine Elite | Top-tier loadout | End-game class |
| 6 | Builder | Base construction | Builds Reactor, Telenodes, Turrets |
| 7 | Builder Advanced | Enhanced construction | Faster building, more structure types |

**Human combat targeting priority:** Overmind → Grangers → alien structures → alien players

Human bots fall back toward the Reactor when below 20% health.

### Alien Classes

Alien bots earn **evos** from kills and evolve into higher-tier forms.

| # | Class | Tier | Notes |
|---|-------|------|-------|
| 8 | Granger | Builder | Builds Overmind, Eggs, Acid Tubes; cannot wall-walk |
| 9 | Dretch | 1 | Fast, tiny biter; wall-walks |
| 10 | Spiker | 2 | Ranged biological projectile |
| 11 | Kamikaze | 2 | Explosive suicide charge against structures |
| 12 | Marauder | 3 | Agile pounce attacker |
| 13 | Dragoon | 3 | Heavy barb volley |
| 14 | Guardian | 4 | High-HP tank |
| 15 | Tyrant | 5 | Ultimate fortress-breaker |

**Alien combat targeting priority:** Reactor → human Builders → Turrets → Marines

Alien bots prefer wall/ceiling routes to bypass turret fields. Kamikaze bots close to point-blank range and detonate against structures.

---

## 5. Team Strategy & AI Behaviour

### Build Priority (Builder / Granger)

Builder-class bots follow a prioritised state machine:

| Priority | Condition | Action |
|----------|-----------|--------|
| CRITICAL | Primary structure missing (Reactor or Overmind) | Build it immediately |
| SPAWNS | No spawn points alive (Telenodes / Eggs) | Place a spawn point |
| DEFENSE | Perimeter expansion needed | Build Turrets (humans) or Acid Tubes (aliens) at choke points |
| REPAIR | Structures are damaged | Restore health to weakest structure |

### Team Coordination

- Bots monitor the health of their team's primary structure and adjust aggression when it is under attack.
- `bot_teamwork` controls the balance between following team strategy and acting on individual targets.
- `bot_aggression` sets the global aggression level — lower values make bots more defensive and likely to retreat; higher values push them to attack even when outnumbered.
- Use `sv botstrategy` to inspect the current strategy state in real time.

---

## 6. Navigation Data

Bots use pre-computed navigation graphs stored in `maps/<mapname>.nav`.

### Auto-generation

If no `.nav` file is found and `bot_nav_autogen 1` is set, the bot system will generate a basic node graph by flood-filling walkable space from each spawn point. This graph is functional but not hand-tuned.

```
bot_nav_autogen 1       // enable auto-generation (default)
bot_nav_density 128     // node spacing in Quake units
```

To force regeneration on the current map:
```
sv navgen
```

### Manual Navigation Editing

For best results on a specific map, use the nav-editor tool (planned for a future release) to place, connect, and annotate nodes by hand. Hand-tuned `.nav` files can be contributed back to the project.

### Nav File Format

`.nav` files are plain-text files with one node per line:

```
# <mapname>.nav
node <id> <x> <y> <z> <flags>
link <from_id> <to_id> [<flags>]
```

Place `.nav` files in `quake2/gloom/maps/` (create the directory if it does not exist).

---

## 7. Configuration Files

All configuration files live in `quake2/gloom/` after installation.

| File | Purpose |
|------|---------|
| `gloombot.cfg` | Master configuration — all cvars with defaults and comments. Load with `exec gloombot.cfg`. |
| `bot_config.cfg` | Auto-fill settings and per-session overrides. |
| `skill_easy.cfg` | Easy difficulty preset. |
| `skill_medium.cfg` | Medium difficulty preset. |
| `skill_hard.cfg` | Hard difficulty preset. |
| `skill_nightmare.cfg` | Nightmare difficulty preset. |
| `bot_names_human.txt` | One name per line — pool of names for human bots. |
| `bot_names_alien.txt` | One name per line — pool of names for alien bots. |

### Recommended `server.cfg` snippet

```
// q2gloombot — add to your server.cfg
exec gloombot.cfg
exec skill_medium.cfg   // or whichever difficulty preset you prefer
bot_count 10            // keep the server filled to 10 players
```

---

## 8. Bot Names

Bot names are drawn randomly from two plain-text files:

- `bot_names_human.txt` — names for human (Marine) bots
- `bot_names_alien.txt` — names for alien bots

Each file contains one name per line. Edit these files to customise the name pool. Names may contain letters, digits, underscores, and dots. Avoid spaces (use underscores instead) and Quake 2 colour codes.

**Example `bot_names_human.txt` entries:**
```
Sgt.Rock
Pvt.Hudson
Cpl.Hicks
```

**Example `bot_names_alien.txt` entries:**
```
Xenomorph
FaceHugger
Lurker
```
