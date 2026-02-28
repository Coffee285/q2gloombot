# Installation Guide — q2gloombot

> Complete instructions for installing q2gloombot on a Quake 2 Gloom server.

---

## Table of Contents

1. [Pre-built Binary Installation](#1-pre-built-binary-installation)
2. [Building from Source](#2-building-from-source)
3. [Platform-Specific Notes](#3-platform-specific-notes)
4. [Verifying the Installation](#4-verifying-the-installation)
5. [Troubleshooting](#5-troubleshooting)

---

## 1. Pre-built Binary Installation

This is the recommended path for server operators who do not want to compile code.

### Step 1 — Download the latest release

Go to the [Releases page](https://github.com/Coffee285/q2gloombot/releases/latest) and download the archive for your platform:

| Platform | File |
|----------|------|
| Windows (x86) | `q2gloombot-<version>-windows-x86.zip` |
| Linux (x86) | `q2gloombot-<version>-linux-x86.tar.gz` |

### Step 2 — Extract the archive

**Windows:**
```
Unzip the archive anywhere convenient, e.g. C:\Downloads\q2gloombot\
```

**Linux:**
```bash
tar xzf q2gloombot-<version>-linux-x86.tar.gz
cd q2gloombot-<version>-linux-x86/
```

The extracted layout is:
```
gamex86.dll        (Windows) or gamei386.so (Linux)
config/
  gloombot.cfg
  bot_config.cfg
  bot_names_human.txt
  bot_names_alien.txt
  skill_easy.cfg
  skill_medium.cfg
  skill_hard.cfg
  skill_nightmare.cfg
maps/
  README.txt
```

### Step 3 — Back up your original Gloom DLL

**Before copying anything**, rename the existing Gloom game DLL so you can restore it later:

**Windows:**
```
Rename  quake2\gloom\gamex86.dll  →  quake2\gloom\gamex86_original.dll
```

**Linux:**
```bash
mv ~/quake2/gloom/gamei386.so ~/quake2/gloom/gamei386_original.so
```

### Step 4 — Copy the game DLL

**Windows:**
```
Copy  gamex86.dll  →  quake2\gloom\gamex86.dll
```

**Linux:**
```bash
cp gamei386.so ~/quake2/gloom/gamei386.so
```

### Step 5 — Copy configuration files

Copy the entire `config/` directory contents into your Gloom mod directory:

**Windows:**
```
xcopy config\* quake2\gloom\ /Y
```

**Linux:**
```bash
cp config/* ~/quake2/gloom/
```

### Step 6 — Copy navigation data (optional)

If the release includes pre-generated `.nav` files for standard Gloom maps, copy them:

**Windows:**
```
xcopy maps\*.nav quake2\gloom\maps\ /Y
```

**Linux:**
```bash
mkdir -p ~/quake2/gloom/maps/
cp maps/*.nav ~/quake2/gloom/maps/
```

> Without `.nav` files bots will navigate randomly. See [docs/MANUAL.md — Navigation Data](MANUAL.md#6-navigation-data) for how to generate them.

### Step 7 — Start the server

```
quake2 +set game gloom +exec gloombot.cfg
```

Or for a dedicated server:

```
quake2ded +set game gloom +exec gloombot.cfg +map gloom1
```

### Step 8 — Add bots

From the server console:

```
sv addbot alien 0.5
sv addbot human 0.5
```

### Step 9 — Bots are now playing!

Use `sv listbots` to confirm they connected. See [docs/MANUAL.md](MANUAL.md) for the full command reference.

---

## 2. Building from Source

### Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.10 |
| GCC | 7.0 |
| Clang | 6.0 |
| MSVC | 2017 (15.x) |
| Git | any recent version |

Only one C compiler is needed — GCC, Clang, **or** MSVC.

### Step 1 — Clone the repository

```bash
git clone https://github.com/Coffee285/q2gloombot.git
cd q2gloombot
```

### Step 2 — Configure

**Linux / macOS:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

**Windows (Visual Studio 2022, 32-bit):**
```bat
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
```

**Windows (Visual Studio 2019, 32-bit):**
```bat
cmake .. -G "Visual Studio 16 2019" -A Win32
```

### Step 3 — Build

```bash
cmake --build . --config Release
```

### Step 4 — Locate the output DLL

| Platform | Output path |
|----------|-------------|
| Linux | `build/gamei386.so` |
| Windows (Release) | `build/Release/gamex86.dll` |

### Step 5 — Install

Follow the [Pre-built Binary Installation](#1-pre-built-binary-installation) steps above, substituting the DLL you just compiled in place of the downloaded one.

---

## 3. Platform-Specific Notes

### Windows

- Quake 2 on Windows is a **32-bit** application. The DLL must be compiled for x86 (32-bit), not x64.
- If you receive a "This application failed to start because gamex86.dll was not found" error, ensure the DLL is in `quake2\gloom\`, not in the Quake 2 root directory.
- The Microsoft Visual C++ Redistributable for Visual Studio 2017 or later must be installed on the server machine if you compiled with MSVC.

### Linux

- Quake 2's dedicated server is typically a 32-bit binary. Build with `-m32` if your system is 64-bit. The CMakeLists.txt handles this automatically when it detects an x86 processor.
- Ensure 32-bit glibc is installed: `sudo apt-get install gcc-multilib` on Debian/Ubuntu.
- The `.so` file must **not** have the `lib` prefix — it must be named exactly `gamei386.so`.

### macOS

- Quake 2 is not officially supported on macOS in this project. Community ports (e.g. Yamagi Quake II) may work but are untested.

---

## 4. Verifying the Installation

After starting the server, check the console output for:

```
q2gloombot v<version> initialised
Bot cvars registered
```

If you see the Gloom game initialise without these lines, the wrong DLL is being loaded — double-check the file was copied to the correct `gloom/` directory.

Run `sv listbots` at any point; if bots are running, the installation is complete.

---

## 5. Troubleshooting

### "Bots don't move"

Navigation data is missing. Either:
- Copy pre-built `.nav` files from the release into `quake2/gloom/maps/`, **or**
- Run `sv navgen` from the server console to auto-generate nodes for the current map (requires `bot_nav_autogen 1` in `gloombot.cfg`).

### "Server crashes on map load"

Version mismatch between the bot DLL and the Gloom mod. Verify:
1. The Gloom mod version matches what q2gloombot was built against (check the release notes).
2. You are not mixing 32-bit and 64-bit binaries.
3. Restore `gamex86_original.dll` and confirm the server runs without bots to isolate the issue.

### "Bots don't upgrade classes"

Check the `bot_upgrade_enabled` cvar:
```
bot_upgrade_enabled 1
```
If it is already `1`, verify that `bot_upgrade_delay` is not set to an excessively large value.

### "Bots are too easy / too hard"

Adjust `bot_skill` in `gloombot.cfg` or apply a preset:
```
exec skill_easy.cfg       // 0.2 skill
exec skill_medium.cfg     // 0.5 skill
exec skill_hard.cfg       // 0.8 skill
exec skill_nightmare.cfg  // 1.0 skill
```
See [docs/MANUAL.md — Difficulty Cvars](MANUAL.md#difficulty-cvars) for per-parameter tuning.

### "Bots don't build"

Check the `bot_build_priority` cvar — it must be greater than 0:
```
bot_build_priority 0.8
```
Engineer/Breeder bots will also not build if the primary structure (Reactor / Overmind) is intact and all spawn points are alive and `bot_build_priority` is low. Increase the value to make them more aggressive about constructing.

### "Connection refused / server not reachable"

This is a Quake 2 networking issue unrelated to q2gloombot. Verify your server's `port` cvar and firewall settings.
