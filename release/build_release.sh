#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

# Read version from source or default to 1.0.0
VERSION=$(grep -oP '#define\s+GLOOMBOT_VERSION\s+"\K[^"]+' "$PROJECT_DIR/src/bot/bot.h" 2>/dev/null || echo "1.0.0")
ARCHIVE_NAME="q2gloombot-v${VERSION}-linux-x86.tar.gz"

echo "=== Q2GloomBot Release Build v${VERSION} (Linux x86) ==="

# Clean build
echo "--- Cleaning previous build ---"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure and build
echo "--- Configuring CMake (Release) ---"
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "--- Building ---"
cmake --build "$BUILD_DIR" --config Release -- -j"$(nproc)"

# Run tests
echo "--- Running tests ---"
"$BUILD_DIR/bot_test"
echo "--- Tests passed ---"

# Package release
echo "--- Packaging release ---"
STAGING_DIR=$(mktemp -d)
RELEASE_DIR="$STAGING_DIR/q2gloombot-v${VERSION}"
mkdir -p "$RELEASE_DIR"

cp "$BUILD_DIR/gamei386.so" "$RELEASE_DIR/"
cp -r "$PROJECT_DIR/config" "$RELEASE_DIR/"
cp -r "$PROJECT_DIR/maps" "$RELEASE_DIR/"
cp -r "$PROJECT_DIR/docs" "$RELEASE_DIR/"
cp "$PROJECT_DIR/README.md" "$RELEASE_DIR/"
cp "$PROJECT_DIR/LICENSE" "$RELEASE_DIR/"
cp "$PROJECT_DIR/CHANGELOG.md" "$RELEASE_DIR/"

mkdir -p "$SCRIPT_DIR"
tar -czf "$SCRIPT_DIR/$ARCHIVE_NAME" -C "$STAGING_DIR" "q2gloombot-v${VERSION}"
rm -rf "$STAGING_DIR"

echo "=== Release package created: release/$ARCHIVE_NAME ==="
