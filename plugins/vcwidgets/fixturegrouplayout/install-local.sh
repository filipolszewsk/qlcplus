#!/usr/bin/env bash
# Install Fixture Group Layout VC widget to user folder (macOS).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DYLIB="${BUILD_DIR}/libfixturegrouplayout_vcwidget.dylib"
BUNDLE="${HOME}/QLC+.app"
DEST="${HOME}/Library/Application Support/QLC+/VCWidgets"

if [[ ! -f "${DYLIB}" ]]; then
  echo "Build first from ${BUILD_DIR}: cmake .. && cmake --build ." >&2
  exit 1
fi

if [[ ! -d "${BUNDLE}/Contents/Frameworks" ]]; then
  echo "Missing ${BUNDLE} — run ./install.sh in qlcplus repo first." >&2
  exit 1
fi

mkdir -p "${DEST}"

install_name_tool -add_rpath "${BUNDLE}/Contents/Frameworks" "${DYLIB}" 2>/dev/null || true

# Warn if plugin still links Qt from aqtinstall path instead of the app bundle
if otool -L "${DYLIB}" 2>/dev/null | grep -q "QtCore.framework"; then
  if otool -L "${DYLIB}" | grep "QtCore.framework" | grep -qv "@rpath"; then
    echo "WARNING: Plugin links Qt outside ~/QLC+.app bundle — modal dialogs may crash." >&2
    echo "  Rebuild with Qt 6.8.1 and ensure @rpath includes Contents/Frameworks." >&2
    otool -L "${DYLIB}" | grep "Qt" || true
  fi
fi

codesign --force --sign - "${DYLIB}"

cp "${DYLIB}" "${DEST}/"
DEST_DYLIB="${DEST}/$(basename "${DYLIB}")"
codesign --force --sign - "${DEST_DYLIB}"
codesign --verify --verbose=2 "${DEST_DYLIB}"
echo "Installed and signed: ${DEST_DYLIB}"
