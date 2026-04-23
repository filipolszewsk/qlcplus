#!/bin/bash
#
# QLC+ Install.command
# Double-click this file to install QLC+ to /Applications
# and remove macOS quarantine so Gatekeeper won't block it.
#
set -e

DMG_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_SRC="$DMG_DIR/QLC+.app"
DEST="/Applications/QLC+.app"

echo "==============================="
echo "  QLC+ Installer"
echo "==============================="
echo ""

if [ ! -d "$APP_SRC" ]; then
    echo "ERROR: QLC+.app not found next to this script."
    echo "Make sure you run this from inside the QLC+ DMG."
    exit 1
fi

echo "Installing QLC+.app to /Applications..."
cp -Rf "$APP_SRC" "$DEST"

echo "Removing macOS quarantine attribute..."
xattr -dr com.apple.quarantine "$DEST" 2>/dev/null || true

echo ""
echo "Done! Launching QLC+..."
open "$DEST"
