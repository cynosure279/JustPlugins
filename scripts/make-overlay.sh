#!/bin/bash
# make-overlay.sh - Generate the JustPlugins overlay tarball
#
# Packages all JustPlugins source files (new + modified) into a tarball
# that the APKBUILD downloads and applies on top of the phosh source tree.
#
# Usage: ./scripts/make-overlay.sh [version]
# Output: justplugins-overlay-VERSION.tar.gz

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION="${1:-0.1.0}"
OUTPUT_DIR="$PROJECT_DIR/dist"

mkdir -p "$OUTPUT_DIR"

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT

mkdir -p "$STAGE/justplugins-overlay/src/ui"
cp "$PROJECT_DIR"/src/*.c "$STAGE/justplugins-overlay/src/"
cp "$PROJECT_DIR"/src/*.h "$STAGE/justplugins-overlay/src/"
cp "$PROJECT_DIR"/src/*.ui "$STAGE/justplugins-overlay/src/ui/" 2>/dev/null || true
cp "$PROJECT_DIR"/src/meson.build "$STAGE/justplugins-overlay/src/"
cp "$PROJECT_DIR"/src/phosh.gresources.xml "$STAGE/justplugins-overlay/src/"

echo "$VERSION" > "$STAGE/justplugins-overlay/VERSION"

TARBALL="$OUTPUT_DIR/justplugins-overlay-$VERSION.tar.gz"
tar -czf "$TARBALL" -C "$STAGE" justplugins-overlay

echo "Created: $TARBALL"
echo "SHA512: $(sha512sum "$TARBALL" | cut -d' ' -f1)"
echo "Size: $(du -h "$TARBALL" | cut -f1)"
