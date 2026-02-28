# Q2GloomBot Release vX.Y.Z

## What's New

<!-- Paste relevant CHANGELOG.md entries here -->

## Downloads

| Platform | File | SHA256 |
|----------|------|--------|
| Linux x86 | [q2gloombot-vX.Y.Z-linux-x86.tar.gz](https://github.com/Coffee285/q2gloombot/releases/download/vX.Y.Z/q2gloombot-vX.Y.Z-linux-x86.tar.gz) | `<hash>` |
| Windows x86 | [q2gloombot-vX.Y.Z-windows-x86.zip](https://github.com/Coffee285/q2gloombot/releases/download/vX.Y.Z/q2gloombot-vX.Y.Z-windows-x86.zip) | `<hash>` |

## Quick Install

1. Download the archive for your platform from the table above.
2. Back up your existing Gloom game DLL (`gamex86.dll` or `gamei386.so`).
3. Extract the archive contents into your Quake 2 `gloom/` directory.
4. Start the server: `quake2 +set game gloom +exec gloombot.cfg`
5. Add bots from the console: `sv addbot alien 0.5`

## Compatibility

- **Quake II engine**: Compatible with Q2PRO, R1Q2, and vanilla Quake II v3.20+.
- **Mod**: Requires the Gloom mod.
- **Architecture**: 32-bit (x86) builds only; matches Quake II's native architecture.
- **OS**: Linux (glibc 2.17+), Windows 7 and later.

## Known Issues

<!-- List any known issues or limitations for this release -->

- None at this time.
