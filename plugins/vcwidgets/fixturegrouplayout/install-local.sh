#!/usr/bin/env bash
# Build and install Fixture Group Layout VC widget to user folder (macOS).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DYLIB="${BUILD_DIR}/libfixturegrouplayout_vcwidget.dylib"
BUNDLE="${HOME}/QLC+.app"
DEST="${HOME}/Library/Application Support/QLC+/VCWidgets"
QLCPLUS_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
QLCPLUS_BUILD="${QLCPLUS_ROOT}/build"

if [[ ! -d "${BUNDLE}/Contents/Frameworks" ]]; then
  echo "Missing ${BUNDLE} — run ./install.sh in qlcplus repo first." >&2
  exit 1
fi

if [[ ! -d "${BUILD_DIR}" ]] || [[ ! -f "${BUILD_DIR}/Makefile" ]]; then
  echo "Configuring plugin build..."
  cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DQLCPLUS_SRC_DIR="${QLCPLUS_ROOT}" \
    -DQLCPLUS_BUILD_DIR="${QLCPLUS_BUILD}" \
    -DCMAKE_PREFIX_PATH="${HOME}/Qt/6.8.1/macos/lib/cmake"
fi

echo "Building plugin..."
cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

if [[ ! -f "${DYLIB}" ]]; then
  echo "Build failed: ${DYLIB} not found" >&2
  exit 1
fi

# Verify EFX conflict UI is present (constructor includes MaskEfxConflictPolicy).
CTOR_CHECK="$(nm -gU "${DYLIB}" 2>/dev/null | grep 'ConfigDialogC1' | c++filt 2>/dev/null || true)"
if ! echo "${CTOR_CHECK}" | grep -q 'MaskEfxConflictPolicy'; then
  echo "ERROR: Plugin binary is missing EFX conflict policy UI (stale build?)." >&2
  echo "  ConfigDialog constructors found:" >&2
  echo "${CTOR_CHECK}" | sed 's/^/    /' >&2
  echo "  Try: rm -rf \"${BUILD_DIR}\" && re-run ./install-local.sh" >&2
  exit 1
fi

mkdir -p "${DEST}"

install_name_tool -add_rpath "${BUNDLE}/Contents/Frameworks" "${DYLIB}" 2>/dev/null || true

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
echo "Restart ~/QLC+.app to load the updated plugin."
