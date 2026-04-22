#!/bin/bash
#
# GRIDqlc - Install.command
# Double-click this file to install GRIDqlc to /Applications
# and remove macOS quarantine so Gatekeeper won't block it.
#
set -e

DMG_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_SRC="$DMG_DIR/QLC+.app"
DEST="/Applications/QLC+.app"

echo "==============================="
echo "  GRIDqlc Installer"
echo "==============================="
echo ""

if [ ! -d "$APP_SRC" ]; then
    echo "ERROR: QLC+.app not found next to this script."
    echo "Make sure you run this from inside the GRIDqlc DMG."
    exit 1
fi

echo "Installing QLC+.app to /Applications..."
cp -Rf "$APP_SRC" "$DEST"

echo "Removing macOS quarantine attribute..."
xattr -dr com.apple.quarantine "$DEST" 2>/dev/null || true

echo ""
echo "Done! Launching GRIDqlc..."
open "$DEST"
