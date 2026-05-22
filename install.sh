#!/usr/bin/env bash
# ============================================================================
# GRIDqlc macOS install script (Apple Silicon, Qt 6.8.1 LTS via aqtinstall)
#
# Usage:  ./install.sh           — build + install + verify
#         ./install.sh verify    — only verify existing install
#         ./install.sh setup     — install Qt 6.8.1 via aqtinstall (one-time)
#
# Always installs to ~/QLC+.app. NEVER touches /Applications.
#
# Qt is pinned to 6.8.1 LTS — the same version as GitHub Actions CI.
# This guarantees that VC Widget plugins built locally are binary-compatible
# with the official release DMG and vice versa.
#
# QTDIR env var overrides the default Qt path (for CI or custom installs).
# ============================================================================
set -euo pipefail

QT_VERSION="6.8.1"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP="$HOME/QLC+.app"
QTDIR="${QTDIR:-$HOME/Qt/$QT_VERSION/macos}"
QMAKE="$QTDIR/bin/qmake"
MACDEPLOYQT="$QTDIR/bin/macdeployqt"

RED=$'\033[0;31m'; GREEN=$'\033[0;32m'; YELLOW=$'\033[0;33m'; NC=$'\033[0m'
ok()    { echo "${GREEN}[ OK ]${NC} $*"; }
warn()  { echo "${YELLOW}[WARN]${NC} $*"; }
fail()  { echo "${RED}[FAIL]${NC} $*"; exit 1; }
info()  { echo "${YELLOW}[....]${NC} $*"; }

# ---------------------------------------------------------------------------
# `setup` — install Qt 6.8.1 via aqtinstall (run once)
# ---------------------------------------------------------------------------
setup_qt() {
    echo ""
    echo "================ SETUP: Qt $QT_VERSION ================"
    echo "Installing Qt $QT_VERSION LTS to $HOME/Qt/$QT_VERSION/macos"
    echo "This is a one-time operation (~3 GB download)."
    echo ""

    if [ -x "$QMAKE" ]; then
        ok "Qt $QT_VERSION already installed at $QTDIR"
        "$QMAKE" --version
        return 0
    fi

    if ! command -v aqt &>/dev/null; then
        info "Installing aqtinstall..."
        pip3 install aqtinstall || pip install aqtinstall || \
            fail "pip3 not found. Install Python 3 first: brew install python"
    fi

    info "Downloading Qt $QT_VERSION (modules: qt5compat qt3d qtimageformats qtmultimedia qtserialport qtwebsockets)..."
    aqt install-qt mac desktop "$QT_VERSION" clang_64 \
        --outputdir "$HOME/Qt" \
        -m qt5compat qt3d qtimageformats qtmultimedia qtserialport qtwebsockets \
        || fail "aqtinstall failed. Check https://aqtinstall.readthedocs.io/"

    ok "Qt $QT_VERSION installed at $QTDIR"
    "$QMAKE" --version
    echo "======================================================="
}

# ---------------------------------------------------------------------------
# Verification (standalone and at end of install)
# ---------------------------------------------------------------------------
verify() {
    echo ""
    echo "================ VERIFICATION ================"

    [ -d "$APP" ] || fail "$APP does not exist. Run install first."

    # 1. QtCore must be bundled inside the app (macdeployqt copies it in)
    local qtcore_fw="$APP/Contents/Frameworks/QtCore.framework"
    if [ -d "$qtcore_fw" ]; then
        # Read version from bundled framework Info.plist
        local bundled_ver
        bundled_ver=$(defaults read "$qtcore_fw/Resources/Info.plist" CFBundleShortVersionString 2>/dev/null || echo "?")
        if echo "$bundled_ver" | grep -qE "^6\.8"; then
            ok "Qt bundled: $bundled_ver (Qt 6.8.x LTS)"
        elif [ "$bundled_ver" = "?" ]; then
            # Fallback: check via otool
            local qt_link
            qt_link=$(otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
                      | grep "QtCore " | head -1)
            if echo "$qt_link" | grep -qE "current version 6\.8\."; then
                ok "Qt version: $(echo "$qt_link" | grep -oE 'current version 6\.[0-9.]+')"
            else
                warn "Could not verify Qt version from Info.plist or otool: $qt_link"
            fi
        else
            warn "Unexpected Qt version bundled: $bundled_ver (expected 6.8.x)"
        fi
    else
        # macdeployqt not run or failed — check rpath-linked Qt
        local qt_line
        qt_line=$(otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
                  | grep "QtCore " | head -1)
        if echo "$qt_line" | grep -qE "current version 6\.8\."; then
            ok "Qt version: $(echo "$qt_line" | grep -oE 'current version 6\.[0-9.]+')"
        elif echo "$qt_line" | grep -qE "current version 6\."; then
            warn "Qt not bundled (no QtCore.framework in app) — Qt version: $qt_line"
            warn "Run install.sh again to re-bundle Qt properly."
        else
            fail "QtCore not found in bundle and not linked properly: $qt_line"
        fi
    fi

    # 2. Qt 5 must NOT be referenced
    if otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
       | grep -q "qt@5\|Qt5"; then
        fail "Qt 5 still referenced — rebuild with: rm -rf build && ./install.sh"
    fi
    ok "No Qt 5 references"

    # 3. Homebrew Qt must NOT be referenced (would break on machines without Homebrew Qt 6.x)
    if otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
       | grep -q "/opt/homebrew/opt/qt"; then
        warn "Linked to Homebrew Qt — run ./install.sh to rebuild with aqtinstall Qt 6.8.1"
    else
        ok "No Homebrew Qt references (bundle is self-contained)"
    fi

    # 4. Hot-reload code present (_qlcpluginId)
    local sentinel
    sentinel=$(strings "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
               | grep -c "_qlcpluginId" || true)
    if [ "$sentinel" -ge 1 ]; then
        ok "Plugin Hub code present (_qlcpluginId)"
    else
        fail "Old build — missing Plugin Hub code. Run: rm -rf build && ./install.sh"
    fi

    # 5. I/O plugins exist
    local n_plugins
    n_plugins=$(find "$APP/Contents/PlugIns" -maxdepth 1 -name "*.dylib" 2>/dev/null \
                | wc -l | tr -d ' ')
    if [ "$n_plugins" -ge 10 ]; then
        ok "I/O plugins installed ($n_plugins dylibs)"
    else
        warn "Only $n_plugins I/O plugins found (expected ~13)"
    fi

    # 6. Architecture
    if file "$APP/Contents/MacOS/qlcplus" 2>/dev/null | grep -q "arm64"; then
        ok "ARM64 native"
    else
        warn "Not arm64 native?"
    fi

    # 7. Code signature
    if codesign --verify "$APP" 2>/dev/null; then
        ok "Code signature valid"
    else
        warn "Code signature invalid"
    fi

    # 8. VC Widget user plugins — check Qt compatibility
    local vcw_dir="$HOME/Library/Application Support/QLC+/VCWidgets"
    if [ -d "$vcw_dir" ]; then
        local bad_plugins=0
        while IFS= read -r -d '' plugin_file; do
            local plugin_qt
            plugin_qt=$(otool -L "$plugin_file" 2>/dev/null \
                        | grep "QtCore " | grep -oE "current version [0-9.]+" | head -1 || true)
            if [ -n "$plugin_qt" ] && ! echo "$plugin_qt" | grep -q "6\.8\."; then
                warn "Plugin Qt mismatch: $(basename "$plugin_file") — $plugin_qt (need 6.8.x)"
                warn "  Rebuild plugin against Qt $QT_VERSION SDK"
                bad_plugins=$((bad_plugins + 1))
            fi
        done < <(find "$vcw_dir" -name "*.dylib" -print0 2>/dev/null)
        if [ "$bad_plugins" -eq 0 ]; then
            ok "VC Widget user plugins: all Qt-compatible (or none installed)"
        fi
    fi

    # 9. Build date
    local build_date
    build_date=$(stat -f "%Sm" "$APP/Contents/Frameworks/libqlcplusui.dylib")
    ok "Build date: $build_date"

    echo "==============================================="
    echo "${GREEN}OK — open with:  open ~/QLC+.app${NC}"
}

# ---------------------------------------------------------------------------
# Detect Qt 6.8.1 — require aqtinstall, refuse Homebrew rolling Qt
# ---------------------------------------------------------------------------
check_qt() {
    if [ ! -x "$QMAKE" ]; then
        echo ""
        fail "Qt $QT_VERSION not found at $QTDIR.

Run the one-time setup:
    ./install.sh setup

Or set QTDIR to your Qt $QT_VERSION installation:
    QTDIR=~/Qt/$QT_VERSION/macos ./install.sh"
    fi

    local found_ver
    found_ver=$("$QMAKE" -query QT_VERSION 2>/dev/null || echo "?")
    if ! echo "$found_ver" | grep -qE "^6\.8\."; then
        fail "Qt at $QTDIR is version $found_ver, need 6.8.x.
Expected path: $HOME/Qt/$QT_VERSION/macos
Run: ./install.sh setup"
    fi
    ok "Qt $found_ver at $QTDIR"
}

# ---------------------------------------------------------------------------
# Detect if CMakeCache has wrong Qt paths and reconfigure if needed
# ---------------------------------------------------------------------------
maybe_reconfigure() {
    local cache="$PROJECT_DIR/build/CMakeCache.txt"
    local need_reconfigure=0

    if [ ! -d "$PROJECT_DIR/build" ]; then
        need_reconfigure=1
    elif [ -f "$cache" ]; then
        # Check if CMakeCache was configured with a different Qt
        local cached_qt_dir
        cached_qt_dir=$(grep "^QT_DIR:PATH=" "$cache" 2>/dev/null | cut -d= -f2 || true)
        local expected_qt_dir="$QTDIR/lib/cmake/Qt6"
        if [ -n "$cached_qt_dir" ] && [ "$cached_qt_dir" != "$expected_qt_dir" ]; then
            warn "CMakeCache has Qt at: $cached_qt_dir"
            warn "Expected Qt at:       $expected_qt_dir"
            info "Deleting stale build dir and reconfiguring for Qt $QT_VERSION..."
            rm -rf "$PROJECT_DIR/build"
            need_reconfigure=1
        fi
    fi

    if [ "$need_reconfigure" -eq 1 ]; then
        info "Configuring cmake (Qt $QT_VERSION, arm64, macOS 12+)..."
        cmake -S "$PROJECT_DIR" -B "$PROJECT_DIR/build" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_PREFIX_PATH="$QTDIR/lib/cmake" \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
            -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -Dqmlui=OFF \
            || fail "cmake configure failed"
        ok "cmake configured"
    fi
}

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
case "${1:-}" in
    setup)  setup_qt; exit 0 ;;
    verify) verify;   exit 0 ;;
esac

# ---------------------------------------------------------------------------
# Install mode
# ---------------------------------------------------------------------------
cd "$PROJECT_DIR"
check_qt
maybe_reconfigure

info "Building QLC+ (full build, all targets)..."
cmake --build "$PROJECT_DIR/build" --parallel "$(sysctl -n hw.ncpu)"

info "Installing to $APP..."
cmake --install "$PROJECT_DIR/build" >/dev/null

info "Running macdeployqt (bundles Qt $QT_VERSION frameworks into app)..."
# chmod -R u+w needed because cmake --install may copy Homebrew-style read-only files
chmod -R u+w "$APP"
"$MACDEPLOYQT" "$APP" -codesign=-

# Remove backup files left by BSD sed (from platform cmake scripts)
find "$APP" \( -name '*.plist-e' -o -name '*.bak' \) -delete 2>/dev/null || true

info "Final codesign..."
codesign --force --sign - --timestamp=none "$APP" 2>&1 | tail -1

verify
