# Release Notes — GloomBot v1.0.0

## Summary

First release of **GloomBot** — AI bot players for the Quake 2 Gloom mod.

GloomBot adds fully autonomous AI players to [Gloom](https://www.planetgloom.com/),
the asymmetric team-based Quake 2 mod where Space Marines battle Alien Spiders.
The bot code ships as a drop-in replacement for the Gloom game DLL
(`gamex86.dll` on Windows, `gamei386.so` on Linux).

---

## Feature Highlights

1. **All 16 Gloom classes** — every human and alien class has distinct AI
   behaviour, weapons handling, and upgrade logic.
2. **Asymmetric team strategy** — bots understand each team's unique win
   condition (destroy Reactor vs. destroy Overmind) and adapt their tactics.
3. **A\* pathfinding** — pre-computed nav graphs with full 3-D wall/ceiling
   routing for alien classes; auto-generation when no `.nav` file is present.
4. **Building AI** — Builder and Granger bots construct and repair base
   structures following a four-level priority state machine.
5. **Class upgrade system** — bots earn and spend credits/evos to advance
   through the tech tree at runtime without server restarts.
6. **Personality & humanise systems** — each bot has a randomised personality
   and smooth, imperfect aim that makes them feel like real players.
7. **Team coordination** — strategy modes, role assignment, Reactor/Overmind
   awareness, and map-control tracking.
8. **Auto-fill** — automatically adds bots to keep the server at a configured
   population minimum.
9. **Full console control** — add, remove, inspect, and tune bots from the
   server console or RCON without restarting.
10. **Zero recompile** — all behaviour is tunable at runtime through 27
    Quake 2 cvars.

---

## System Requirements

| Requirement | Details |
|-------------|---------|
| Quake 2 | Any standard installation (3.20 or later recommended) |
| Gloom mod | Gloom 1.x installed in `quake2/gloom/` |
| Server access | Console or RCON access to the Quake 2 dedicated server |
| OS | Windows (x86) or Linux (x86 / x86-64 with 32-bit libc) |

> To **build from source**: CMake ≥ 3.10 and a C99 compiler (GCC 7+, Clang 6+,
> or MSVC 2017+).

---

## Quick Start

1. **Download** the release archive for your platform from the
   [releases page](https://github.com/Coffee285/q2gloombot/releases/latest).
2. **Back up** your existing Gloom game DLL:
   ```
   rename quake2/gloom/gamex86.dll gamex86_original.dll
   ```
3. **Copy** the release files into your Gloom mod directory:
   ```
   quake2/gloom/gamex86.dll      (Windows)
   quake2/gloom/gamei386.so      (Linux)
   quake2/gloom/gloombot.cfg
   quake2/gloom/bot_config.cfg
   quake2/gloom/skill_*.cfg
   quake2/gloom/bot_names_*.txt
   ```
4. **Start** the server with the bot configuration:
   ```
   quake2 +set game gloom +exec gloombot.cfg
   ```
5. **Add a bot** from the server console:
   ```
   sv addbot alien 0.5
   ```

Bots are now playing! See `docs/INSTALL.md` for the full installation guide
and `docs/MANUAL.md` for all commands and cvars.

---

## Known Limitations & Planned Improvements

- **Nav data** — auto-generated nav graphs are functional but not hand-tuned;
  a dedicated nav-editor tool is planned for a future release.
- **Alien wall-walk pathing** — wall/ceiling traversal relies on basic surface
  normals; complex geometry may cause occasional stuck behaviour.
- **No bot-vs-bot spectating** — bots do not react to spectator chat.
- **Single-server only** — there is no cross-server bot coordination.
- **No team-voice commands** — bots respond to console `sv` commands only;
  in-game voice-menu responses are planned.
- **Windows 64-bit** — the Quake 2 game DLL interface requires 32-bit builds
  on Windows; native 64-bit support depends on the server executable.

---

## Credits & Acknowledgments

- **id Software** — Quake 2 engine and game DLL interface
  ([GPL v2](https://github.com/id-Software/Quake-2)).
- **Gloom mod team** — for creating the asymmetric Gloom game mode that this
  bot system is designed for.
- **Quake 2 community** — documentation, reverse-engineering references, and
  inspiration from earlier Q2 bot projects (Eraser Bot, ACEBOT, Gladiator Bot).

---

## License

GloomBot is distributed under the **GNU General Public License v2**.
See [LICENSE](LICENSE) for the full text.

Copyright (C) 2024 q2gloombot contributors.
