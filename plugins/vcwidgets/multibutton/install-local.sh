#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
FIX="${ROOT}/install-vcwidget-dylib.sh"
BUILD_DIR="${SCRIPT_DIR}/build"
DYLIB="${BUILD_DIR}/libmultibutton_vcwidget.dylib"
QLCPLUS_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
QLCPLUS_BUILD="${QLCPLUS_ROOT}/build"
QT_CMAKE="${HOME}/Qt/6.8.1/macos/lib/cmake"

if [[ ! -x "$FIX" ]]; then
  echo "Missing $FIX" >&2
  exit 1
fi

if [[ ! -d "${HOME}/QLC+.app/Contents/Frameworks" ]]; then
  echo "Missing ~/QLC+.app — run ./install.sh in qlcplus repo first." >&2
  exit 1
fi

if [[ ! -d "$QT_CMAKE" ]]; then
  echo "Qt 6.8.1 not found. Run: ./install.sh setup" >&2
  exit 1
fi

if [[ ! -f "${BUILD_DIR}/Makefile" ]]; then
  cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DQLCPLUS_SRC_DIR="${QLCPLUS_ROOT}" \
    -DQLCPLUS_BUILD_DIR="${QLCPLUS_BUILD}" \
    -DCMAKE_PREFIX_PATH="${QT_CMAKE}"
fi

cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
"$FIX" "$DYLIB"
echo "Restart ~/QLC+.app — VC → Add → Multi Button"
