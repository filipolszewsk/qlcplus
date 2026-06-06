#!/usr/bin/env bash
# Fix rpaths and Qt install names on a VC widget dylib, then copy to the user VCWidgets dir.
# Usage: install-vcwidget-dylib.sh /path/to/libfoo_vcwidget.dylib

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 /path/to/lib*_vcwidget.dylib" >&2
  exit 1
fi

SRC="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
BUNDLE="${HOME}/QLC+.app"
FRAMEWORKS="${BUNDLE}/Contents/Frameworks"
USER_DEST="${HOME}/Library/Application Support/QLC+/VCWidgets"
QT_LIB="${HOME}/Qt/6.8.1/macos/lib"
SIGN_IDENTITY="${SIGN_IDENTITY:--}"
KEYCHAIN="${KEYCHAIN:-$HOME/Library/Keychains/login.keychain-db}"

if [[ ! -f "$SRC" ]]; then
  echo "Not found: $SRC" >&2
  exit 1
fi
if [[ ! -d "$FRAMEWORKS" ]]; then
  echo "Missing $BUNDLE — run ./install.sh first." >&2
  exit 1
fi
if [[ "$SIGN_IDENTITY" != "-" ]] \
    && ! security find-identity -v -p codesigning "$KEYCHAIN" 2>/dev/null | grep -Fq "\"$SIGN_IDENTITY\""; then
  echo "Missing code signing identity: $SIGN_IDENTITY" >&2
  echo "Set SIGN_IDENTITY=- to use ad-hoc signing for local VCWidget installs." >&2
  exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp "$SRC" "$WORK/$(basename "$SRC")"
DYLIB="$WORK/$(basename "$SRC")"

# Point Qt frameworks at the app bundle (not external ~/Qt).
for fw in QtCore QtGui QtWidgets; do
  OLD="${QT_LIB}/${fw}.framework/Versions/A/${fw}"
  NEW="@rpath/${fw}.framework/Versions/A/${fw}"
  while IFS= read -r line; do
    dep=$(echo "$line" | awk '{print $1}')
    if [[ "$dep" == "$OLD" ]]; then
      install_name_tool -change "$OLD" "$NEW" "$DYLIB" 2>/dev/null || true
    fi
  done < <(otool -L "$DYLIB" | tail -n +2)
done

# Remove build-time / external rpaths; keep only app Frameworks.
while IFS= read -r rpath; do
  [[ -z "$rpath" ]] && continue
  if [[ "$rpath" != *"QLC+.app/Contents/Frameworks"* ]]; then
    install_name_tool -delete_rpath "$rpath" "$DYLIB" 2>/dev/null || true
  fi
done < <(otool -l "$DYLIB" | awk '/path / {print $2}')

install_name_tool -add_rpath "@executable_path/../Frameworks" "$DYLIB" 2>/dev/null || true
install_name_tool -add_rpath "${FRAMEWORKS}" "$DYLIB" 2>/dev/null || true

if otool -L "$DYLIB" | grep -q homebrew; then
  echo "ERROR: still links Homebrew Qt: $SRC" >&2
  otool -L "$DYLIB" | grep -i qt || true
  exit 1
fi

codesign --force --sign "$SIGN_IDENTITY" --timestamp=none "$DYLIB"

mkdir -p "$USER_DEST"
install -m 755 "$DYLIB" "$USER_DEST/"
codesign --force --sign "$SIGN_IDENTITY" --timestamp=none "$USER_DEST/$(basename "$DYLIB")"
codesign --verify --verbose=2 "$USER_DEST/$(basename "$DYLIB")" >/dev/null

echo "Installed $(basename "$DYLIB") to:"
echo "  $USER_DEST"
