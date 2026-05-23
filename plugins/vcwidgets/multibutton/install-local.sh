#!/usr/bin/env bash
# Install multibutton VC widget to user folder (macOS).
# Always codesigns the DESTINATION — required or QLC+ crashes on startup.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DYLIB="${BUILD_DIR}/libmultibutton_vcwidget.dylib"
BUNDLE="${HOME}/QLC+.app"
DEST="${HOME}/Library/Application Support/QLC+/VCWidgets"

if [[ ! -f "${DYLIB}" ]]; then
  echo "Build first: cmake --build \"${BUILD_DIR}\"" >&2
  exit 1
fi

if [[ ! -d "${BUNDLE}/Contents/Frameworks" ]]; then
  echo "Missing ${BUNDLE} — run ./install.sh in qlcplus repo first." >&2
  exit 1
fi

mkdir -p "${DEST}"

install_name_tool -add_rpath "${BUNDLE}/Contents/Frameworks" "${DYLIB}" 2>/dev/null || true
codesign --force --sign - "${DYLIB}"

cp "${DYLIB}" "${DEST}/"
# Re-sign destination (cp preserves signature, but re-sign after any install_name_tool on dest is safest)
DEST_DYLIB="${DEST}/$(basename "${DYLIB}")"
codesign --force --sign - "${DEST_DYLIB}"

codesign --verify --verbose=2 "${DEST_DYLIB}"
echo "Installed and signed: ${DEST_DYLIB}"
