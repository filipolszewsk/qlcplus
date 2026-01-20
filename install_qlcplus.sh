#!/bin/bash
set -e

echo "QLC+ Installation Script for macOS Apple Silicon"
echo "================================================"

# Przejdź do katalogu QLC+
cd "$(dirname "$0")"

# Build
# Clean up potential leftovers from qmake/cmake in root
rm -f Makefile
rm -f CMakeCache.txt
rm -rf CMakeFiles
rm -f cmake_install.cmake
rm -rf build_cmake

mkdir build_cmake
cd build_cmake
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(sysctl -n hw.ncpu)
make install/fast

# Usuń duplikaty Qt
rm -rf ~/QLC+.app/Contents/Frameworks/Qt*.framework
rm -rf ~/QLC+.app/Contents/PlugIns

# Konfiguracja qt.conf
cat > ~/QLC+.app/Contents/Resources/qt.conf << 'QTCONF'
[Paths]
Plugins = /opt/homebrew/share/qt/plugins
Libraries = /opt/homebrew/lib

[Qt]
QT_MAC_WANTS_LAYER = 1
QT_STYLE_OVERRIDE = macos
QT_AUTO_SCREEN_SCALE_FACTOR = 1
QT_ENABLE_HIGHDPI_SCALING = 1
QT_SCALE_FACTOR = 1.0
QTCONF

# Napraw ścieżki Qt
for lib in libqlcplusengine.dylib libqlcplusui.dylib libqlcpluswebaccess.dylib; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null || true
  done
done

for exe in qlcplus qlcplus-fixtureeditor qlcplus-launcher; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null || true
  done
done

# Instalacja pluginów
mkdir -p ~/QLC+.app/Contents/PlugIns
find build_cmake/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \;

# Podpisanie
codesign --force --deep --sign - ~/QLC+.app

echo ""
echo "✅ Instalacja zakończona!"
echo "📍 Aplikacja: ~/QLC+.app"
echo "🚀 Uruchom: open ~/QLC+.app"
