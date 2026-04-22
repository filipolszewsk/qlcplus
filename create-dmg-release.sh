#!/bin/bash
#
# GRIDqlc v1 - macOS DMG Release Build Script
#
# Usage:
#   ./create-dmg-release.sh            # ad-hoc signing (default)
#   SIGNATURE="Developer ID Application: Foo (TEAMID)" ./create-dmg-release.sh  # real signing
#   QTDIR=/path/to/Qt/6.x/macos ./create-dmg-release.sh
#
# After building, users must do ONE of:
#   a) Double-click Install.command inside the DMG  (recommended)
#   b) System Settings > Privacy & Security > "Open Anyway"
#   c) Terminal: xattr -dr com.apple.quarantine /Applications/QLC+.app
#
set -e

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

VERSION=$(grep -m 1 APPVERSION variables.pri | cut -d '=' -f 2 | sed -e 's/^[[:space:]]*//' | tr ' ' _ | tr -d '\r\n')
APP_NAME="GRIDqlc"
APP_DIR=~/QLC+.app
BIN_DIR="$APP_DIR/Contents/MacOS"
ENTITLEMENTS="platforms/macos/qlcplus.entitlements"
OUTDIR="$PWD"

# Use ad-hoc identity by default; export SIGNATURE to override with a real cert
SIGNATURE="${SIGNATURE:--}"

if [ "$SIGNATURE" = "-" ]; then
    echo ">>> Using ad-hoc signature. Export SIGNATURE='Developer ID Application: ...' for notarized builds."
else
    echo ">>> Signing with: $SIGNATURE"
fi

# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------

if [ ! -n "$QTDIR" ]; then
    # Try to auto-detect Qt installed via homebrew or ~/Qt
    for candidate in \
        "$(brew --prefix qt 2>/dev/null)/bin" \
        "$HOME/Qt/6.9.1/macos/bin" \
        "$HOME/Qt/6.8.0/macos/bin" \
        "$HOME/Qt/6.7.0/macos/bin"; do
        if [ -x "$candidate/macdeployqt" ]; then
            QTDIR="$(dirname "$candidate")"
            echo ">>> Auto-detected QTDIR: $QTDIR"
            break
        fi
    done
fi

if [ ! -n "$QTDIR" ] || [ ! -x "$QTDIR/bin/macdeployqt" ]; then
    echo "ERROR: macdeployqt not found. Set QTDIR to your Qt installation root."
    echo "  Example: QTDIR=~/Qt/6.9.1/macos ./create-dmg-release.sh"
    exit 1
fi

# ---------------------------------------------------------------------------
# Clean previous build
# ---------------------------------------------------------------------------

echo ">>> Cleaning previous build..."
rm -rf "$APP_DIR"
find "$OUTDIR" -maxdepth 1 -name "GRIDqlc_*.dmg" -delete
find "$OUTDIR" -maxdepth 1 -name "rw.*.dmg" -delete
rm -rf "$OUTDIR/build_release"

# ---------------------------------------------------------------------------
# CMake configure + build
# Use explicit -S/-B to avoid conflicts with any CMakeCache.txt in project root
# ---------------------------------------------------------------------------

echo ">>> Configuring with CMake..."
cmake \
    -S "$OUTDIR" \
    -B "$OUTDIR/build_release" \
    -DCMAKE_PREFIX_PATH="$QTDIR/lib/cmake" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_BUILD_TYPE=Release

NUM_CPUS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo ">>> Building with $NUM_CPUS cores..."
cmake --build "$OUTDIR/build_release" --parallel "$NUM_CPUS"
cmake --install "$OUTDIR/build_release"

# ---------------------------------------------------------------------------
# Fix non-Qt Homebrew dependencies (libsndfile, fftw, libftdi, libusb)
# These are bundled by CMakeLists; fix_dylib_deps.sh rewrites their rpaths.
# ---------------------------------------------------------------------------

echo ">>> Fixing non-Qt dylib dependencies..."
platforms/macos/fix_dylib_deps.sh "$APP_DIR/Contents/Frameworks/libsndfile.1.dylib" || true
if [ -f "$BIN_DIR/qlcplus" ]; then
    platforms/macos/fix_dylib_deps.sh "$BIN_DIR/qlcplus"
    platforms/macos/fix_dylib_deps.sh "$BIN_DIR/qlcplus-fixtureeditor"
    platforms/macos/fix_dylib_deps.sh "$BIN_DIR/qlcplus-launcher"
else
    platforms/macos/fix_dylib_deps.sh "$BIN_DIR/qlcplus-qml"
fi

# ---------------------------------------------------------------------------
# macdeployqt - bundles Qt frameworks + plugins + fixes rpaths
# ---------------------------------------------------------------------------

echo ">>> Running macdeployqt..."
"$QTDIR/bin/macdeployqt" "$APP_DIR" -verbose=1

# ---------------------------------------------------------------------------
# Purge all Homebrew references from the app bundle
#
# macdeployqt (especially Homebrew-installed) does not always:
#   a) Remove LC_RPATH entries that point to /opt/homebrew inside binaries
#   b) Fix the install_name (-id) of frameworks it copies (QtDBus, QtQmlMeta, etc.)
#   c) Rewrite all dependency paths that still reference /opt/homebrew
#
# Any surviving Homebrew path causes dyld to load that library from Homebrew
# IN ADDITION TO the bundled copy → "implemented in both" crash at startup.
# ---------------------------------------------------------------------------

echo ">>> Purging Homebrew references from bundle..."

# A. Remove LC_RPATH entries pointing to /opt/homebrew or /usr/local from every Mach-O
find "$APP_DIR/Contents" -type f | while read -r f; do
    otool -l "$f" 2>/dev/null | awk '/LC_RPATH/{found=1;next} found && /path/{print $2; found=0}' | \
        grep -E "^(/opt/homebrew|/usr/local)" | while read -r rp; do
        install_name_tool -delete_rpath "$rp" "$f" 2>/dev/null || true
    done
done

# B. Fix install_name (-id) of Qt frameworks that macdeployqt forgot to update
find "$APP_DIR/Contents/Frameworks" -maxdepth 1 -type d -name "*.framework" | while read -r fw; do
    name=$(basename "$fw" .framework)
    bin="$fw/Versions/A/$name"
    if [ -f "$bin" ]; then
        current_id=$(otool -D "$bin" 2>/dev/null | tail -1)
        if echo "$current_id" | grep -q "/opt/homebrew\|/usr/local"; then
            install_name_tool -id \
                "@executable_path/../Frameworks/$name.framework/Versions/A/$name" \
                "$bin" 2>/dev/null || true
            echo "    Fixed install_name: $name"
        fi
    fi
done

# C. Rewrite any remaining /opt/homebrew or /usr/local dependency references
find "$APP_DIR/Contents" -type f | while read -r f; do
    otool -L "$f" 2>/dev/null | awk 'NR>1{print $1}' | \
        grep -E "^(/opt/homebrew|/usr/local)" | while read -r dep; do
        base=$(basename "$dep")
        if [[ "$dep" == *.framework/* ]]; then
            # e.g. /opt/homebrew/.../QtFoo.framework/Versions/A/QtFoo
            fw_name=$(echo "$dep" | sed -E 's|.*/([^/]+\.framework)/.*|\1|')
            bin_name="${fw_name%.framework}"
            new="@executable_path/../Frameworks/$fw_name/Versions/A/$bin_name"
        else
            new="@executable_path/../Frameworks/$base"
        fi
        install_name_tool -change "$dep" "$new" "$f" 2>/dev/null || true
    done
done

echo ">>> Homebrew references purged."

# ---------------------------------------------------------------------------
# Code signing
# ad-hoc (-): checksum only, no identity, ARM64 requires this minimum
# real cert: full Gatekeeper-accepted signature
#
# IMPORTANT ORDER:
# 1. Strip old Qt Company signatures from .framework bundles
# 2. Sign loose .dylibs (non-framework: libsndfile, libfftw3, libqlcplus*, etc.)
# 3. Sign each .framework as a whole BUNDLE DIRECTORY (not individual files inside)
# 4. Sign PlugIns .dylibs
# 5. Sign executables in MacOS/
# 6. Sign the .app bundle as a whole
#
# Signing only the binary FILE inside a framework (e.g. QtWidgets.framework/Versions/A/QtWidgets)
# while leaving the framework's _CodeSignature/ intact from Qt Company causes a Team ID
# mismatch at runtime (DYLD refuses to load: "mapping process and mapped file have different
# Team IDs"). The fix is: --remove-signature on the framework dir first, then re-sign the
# whole framework dir.
# ---------------------------------------------------------------------------

echo ">>> Stripping all existing signatures from .framework bundles..."
find "$APP_DIR" -type d -name "*.framework" | while read -r fw; do
    codesign --remove-signature "$fw" 2>/dev/null || true
done

echo ">>> Signing loose .dylibs in Frameworks (non-framework libraries)..."
find "$APP_DIR/Contents/Frameworks" -name "*.dylib" -type f | while read -r file; do
    codesign --force --sign "$SIGNATURE" \
        --timestamp=none \
        "$file"
done

echo ">>> Signing PlugIns..."
find "$APP_DIR/Contents/PlugIns" -type f -name "*.dylib" | while read -r file; do
    codesign --force --sign "$SIGNATURE" \
        --timestamp=none \
        "$file"
done

echo ">>> Signing .framework bundles..."
find "$APP_DIR/Contents/Frameworks" -type d -name "*.framework" | while read -r fw; do
    codesign --force --sign "$SIGNATURE" \
        --timestamp=none \
        "$fw"
done

echo ">>> Signing executables in MacOS/..."
find "$BIN_DIR" -type f | while read -r file; do
    codesign --force --sign "$SIGNATURE" \
        --options runtime \
        --entitlements "$ENTITLEMENTS" \
        --timestamp=none \
        "$file"
done

# Remove any Info.plist-e temp files that codesign --entitlements may have created;
# if left in place they cause the bundle-level --deep signing to fail.
find "$APP_DIR" -name "*-e" -delete 2>/dev/null || true

echo ">>> Signing the whole .app bundle (--deep ensures correct nested sealing)..."
codesign --force --sign "$SIGNATURE" \
    --deep \
    --options runtime \
    --entitlements "$ENTITLEMENTS" \
    --timestamp=none \
    "$APP_DIR"

# Clear quarantine from our own build (we created it, no quarantine needed)
find "$APP_DIR" -name "com.apple.quarantine" -exec xattr -d com.apple.quarantine {} \; 2>/dev/null || true

echo ">>> Verifying bundle signature..."
codesign --verify --deep --strict --verbose=2 "$APP_DIR"
echo ">>> Signature OK."

# ---------------------------------------------------------------------------
# Create DMG
# ---------------------------------------------------------------------------

DMG_NAME="${APP_NAME}_${VERSION}.dmg"
echo ">>> Creating DMG: $DMG_NAME"

# Stage app + Install.command together in a temp directory so both appear in the DMG volume
STAGING_DIR=$(mktemp -d)
cp -R "$APP_DIR" "$STAGING_DIR/QLC+.app"
cp "$OUTDIR/platforms/macos/dmg/Install.command" "$STAGING_DIR/Install.command"
chmod +x "$STAGING_DIR/Install.command"

cd platforms/macos/dmg
./create-dmg \
    --volname "GRIDqlc $VERSION" \
    --volicon "$OUTDIR/resources/icons/qlcplus.icns" \
    --background background.png \
    --window-size 500 320 \
    --window-pos 200 100 \
    --icon-size 80 \
    --icon "QLC+.app" 120 160 \
    --app-drop-link 360 160 \
    --icon "Install.command" 240 260 \
    "$OUTDIR/$DMG_NAME" \
    "$STAGING_DIR"
cd -

rm -rf "$STAGING_DIR"

# Sign the DMG itself
echo ">>> Signing DMG..."
codesign --force --sign "$SIGNATURE" --timestamp=none "$OUTDIR/$DMG_NAME"

echo ""
echo "=========================================="
echo " BUILD COMPLETE"
echo " Output: $OUTDIR/$DMG_NAME"
echo "=========================================="
echo ""
if [ "$SIGNATURE" = "-" ]; then
    echo " NOTE: Ad-hoc signed. Users will see a Gatekeeper warning on first launch."
    echo " Solution options for end users:"
    echo "   1. Double-click Install.command inside the DMG (automatic bypass)"
    echo "   2. System Settings > Privacy & Security > 'Open Anyway'"
    echo "   3. Terminal: xattr -dr com.apple.quarantine /Applications/QLC+.app"
    echo ""
    echo " To get fully notarized DMG (no warnings ever):"
    echo "   SIGNATURE='Developer ID Application: Name (TEAMID)' ./create-dmg-release.sh"
    echo "   Then: xcrun notarytool submit $DMG_NAME --apple-id ... --team-id ... --wait"
    echo "   Then: xcrun stapler staple $DMG_NAME"
fi

# ---------------------------------------------------------------------------
# Verify DMG
# ---------------------------------------------------------------------------
echo ""
echo ">>> Verifying DMG integrity..."
hdiutil verify "$OUTDIR/$DMG_NAME" && echo ">>> DMG verification: PASSED"
