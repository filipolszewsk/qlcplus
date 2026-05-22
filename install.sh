#!/usr/bin/env bash
# ============================================================================
# GRIDqlc macOS install script (Apple Silicon, Qt 6 from Homebrew)
#
# Usage:  ./install.sh           — build + install + verify
#         ./install.sh verify    — only verify existing install
#
# Always installs to ~/QLC+.app. NEVER touches /Applications.
# ============================================================================
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP="$HOME/QLC+.app"
QT_PLUGINS_DIR="/opt/homebrew/share/qt/plugins"
QT_BASE_LIB="/opt/homebrew/opt/qtbase/lib"

RED=$'\033[0;31m'; GREEN=$'\033[0;32m'; YELLOW=$'\033[0;33m'; NC=$'\033[0m'
ok()    { echo "${GREEN}[ OK ]${NC} $*"; }
warn()  { echo "${YELLOW}[WARN]${NC} $*"; }
fail()  { echo "${RED}[FAIL]${NC} $*"; exit 1; }
info()  { echo "${YELLOW}[....]${NC} $*"; }

# ---------------------------------------------------------------------------
# Verification (used at end of install and standalone `./install.sh verify`)
# ---------------------------------------------------------------------------
verify() {
    echo ""
    echo "================ VERIFICATION ================"

    [ -d "$APP" ] || fail "$APP does not exist. Run install first."

    # 1. Qt version of main library — MUST be 6.x from /opt/homebrew/opt/qt*/lib
    local qt_line
    qt_line=$(otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
              | grep "QtCore " | head -1)

    if echo "$qt_line" | grep -q "/opt/homebrew/opt/qt"; then
        if echo "$qt_line" | grep -qE "current version 6\.(10|9|11|12|13)\."; then
            ok "Qt version: $(echo "$qt_line" | grep -oE 'current version 6\.[0-9.]+')"
        else
            warn "Qt version unexpected: $qt_line"
        fi
    else
        fail "Qt path WRONG (not /opt/homebrew/opt/qt*/lib): $qt_line"
    fi

    # 2. Qt 5 must NOT be referenced
    if otool -L "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null \
       | grep -q "qt@5\|Qt5"; then
        fail "Qt 5 still referenced — this is the old broken build!"
    fi
    ok "No Qt 5 references"

    # 3. qt.conf must point to Homebrew plugins (so styles/imageformats load)
    if [ -f "$APP/Contents/Resources/qt.conf" ] \
       && grep -q "Plugins = /opt/homebrew/share/qt/plugins" "$APP/Contents/Resources/qt.conf"; then
        ok "qt.conf points to Homebrew Qt plugins (UI will have native style)"
    else
        fail "qt.conf MISSING or wrong (UI will look 'old/ugly'). Reinstall."
    fi

    # 4. Hot-reload code present (sentinel string from new build)
    local sentinel
    sentinel=$(strings "$APP/Contents/Frameworks/libqlcplusui.dylib" 2>/dev/null | grep -c "_qlcpluginId" || true)
    if [ "$sentinel" -ge 1 ]; then
        ok "New hot-reload code present (_qlcpluginId)"
    else
        fail "Old build — missing Plugin Hub code. Rebuild needed."
    fi

    # 5. I/O plugins exist
    local n_plugins
    n_plugins=$(find "$APP/Contents/PlugIns" -maxdepth 1 -name "*.dylib" 2>/dev/null | wc -l | tr -d ' ')
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
        warn "Code signature invalid (run codesign again)"
    fi

    # 8. Build date
    local build_date
    build_date=$(stat -f "%Sm" "$APP/Contents/Frameworks/libqlcplusui.dylib")
    ok "Build date: $build_date"

    echo "==============================================="
    echo "${GREEN}OK — open with:  open ~/QLC+.app${NC}"
}

# ---------------------------------------------------------------------------
# `verify` mode
# ---------------------------------------------------------------------------
if [ "${1:-}" = "verify" ]; then
    verify
    exit 0
fi

# ---------------------------------------------------------------------------
# Install mode
# ---------------------------------------------------------------------------
cd "$PROJECT_DIR"

# Refuse to run if Qt 6 from Homebrew is missing
[ -d "$QT_BASE_LIB/QtCore.framework" ] \
    || fail "Qt 6 not found at $QT_BASE_LIB. Run: brew install qt"
[ -d "$QT_PLUGINS_DIR" ] \
    || fail "Qt 6 plugins not found at $QT_PLUGINS_DIR. Reinstall Qt: brew reinstall qt"

# Refuse if build dir missing
[ -d build ] || fail "No build/ dir. Run cmake first: cmake -B build -DCMAKE_BUILD_TYPE=Release"

info "Building qlcplus..."
cmake --build build --target qlcplus --parallel "$(sysctl -n hw.ncpu)"

info "Installing to $APP..."
cmake --install build >/dev/null

info "Installing QLC+ I/O plugins..."
mkdir -p "$APP/Contents/PlugIns"
find plugins -name "*.dylib" -type f -exec cp {} "$APP/Contents/PlugIns/" \;

info "Writing qt.conf (Homebrew Qt plugin path — critical for UI style)..."
cat > "$APP/Contents/Resources/qt.conf" << 'EOF'
[Paths]
Plugins = /opt/homebrew/share/qt/plugins

[Qt]
QT_AUTO_SCREEN_SCALE_FACTOR = 1
QT_ENABLE_HIGHDPI_SCALING = 1
EOF

# Remove old platforms subfolder (Qt loads platform from Homebrew via qt.conf)
rm -rf "$APP/Contents/PlugIns/platforms" 2>/dev/null || true

info "Bundling TLS plugins (required for HTTPS)..."
mkdir -p "$APP/Contents/PlugIns/tls"
for tls_plugin in "$QT_PLUGINS_DIR/tls/"*.dylib; do
    cp "$tls_plugin" "$APP/Contents/PlugIns/tls/"
    install_name_tool \
        -delete_rpath "@loader_path/../../../../lib" \
        -add_rpath "/opt/homebrew/lib" \
        "$APP/Contents/PlugIns/tls/$(basename "$tls_plugin")" 2>/dev/null || true
done

info "Codesigning..."
codesign --force --deep --sign - "$APP" 2>&1 | tail -1

verify
