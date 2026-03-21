# Szybka instalacja GRIDqlc na macOS (Apple Silicon)

> Tylko kroki po zakończeniu `make` — gdy kod jest już skompilowany.  
> Cały proces trwa ~30 sekund.

## Wymagania

Build musi być już zrobiony (`make` skończył się sukcesem w katalogu `build_cmake`).

---

## Jeden skrypt — wszystko naraz

Uruchom z katalogu projektu (`qlcplus/`):

```bash
cd build_cmake && LIBRARY_PATH=/opt/homebrew/lib:$LIBRARY_PATH make install/fast && cd .. && \
rm -rf ~/QLC+.app/Contents/Frameworks/Qt*.framework ~/QLC+.app/Contents/PlugIns && \
cat > ~/QLC+.app/Contents/Resources/qt.conf << 'EOF'
[Paths]
Plugins = /opt/homebrew/share/qt/plugins
Libraries = /opt/homebrew/lib

[Qt]
QT_MAC_WANTS_LAYER = 1
QT_STYLE_OVERRIDE = macos
QT_AUTO_SCREEN_SCALE_FACTOR = 1
QT_ENABLE_HIGHDPI_SCALING = 1
QT_SCALE_FACTOR = 1.0
EOF
for lib in libqlcplusengine.dylib libqlcplusui.dylib libqlcpluswebaccess.dylib; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null || true
  done
done
for exe in qlcplus qlcplus-fixtureeditor qlcplus-launcher; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null || true
  done
done
mkdir -p ~/QLC+.app/Contents/PlugIns && \
find build_cmake/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \; && \
mkdir -p ~/QLC+.app/Contents/PlugIns/tls && \
cp /opt/homebrew/share/qt/plugins/tls/libqsecuretransportbackend.dylib ~/QLC+.app/Contents/PlugIns/tls/ && \
for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
  install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
    "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
    ~/QLC+.app/Contents/PlugIns/tls/libqsecuretransportbackend.dylib 2>/dev/null || true
done && \
codesign --force --deep --sign - ~/QLC+.app && \
echo "✅ Gotowe (pluginy: $(ls ~/QLC+.app/Contents/PlugIns/*.dylib | wc -l))" && \
open ~/QLC+.app
```

---

## Co robi ten skrypt (krok po kroku)

| Krok | Co się dzieje |
|------|--------------|
| `LIBRARY_PATH=...` | Wymagane przed `make` — linker musi widzieć `/opt/homebrew/lib` (m.in. `libltc`) |
| `make install/fast` | Kopiuje skompilowane pliki do `~/QLC+.app` |
| `rm Qt*.framework` | Usuwa zduplikowane frameworki Qt z bundle |
| `rm PlugIns` | Czyści stare pluginy przed reinstalacją |
| `qt.conf` | Kieruje Qt do Homebrew zamiast bundled (styl macOS, pluginy) |
| `install_name_tool` | Naprawia ścieżki `.dylib` → `/opt/homebrew/lib/` |
| `find plugins` | Kopiuje 14 pluginów QLC+ do bundle |
| `codesign` | Podpisuje aplikację (wymagane przez macOS) |
| `open` | Uruchamia aplikację |

---

## Skrót — budowanie + instalacja razem

```bash
cd "/Users/filipolszewski/Documents/qlc projekty/qlcplus/build_cmake" && \
LIBRARY_PATH=/opt/homebrew/lib:$LIBRARY_PATH make -j$(sysctl -n hw.ncpu) && \
LIBRARY_PATH=/opt/homebrew/lib:$LIBRARY_PATH make install/fast && cd .. && \
rm -rf ~/QLC+.app/Contents/Frameworks/Qt*.framework ~/QLC+.app/Contents/PlugIns && \
cat > ~/QLC+.app/Contents/Resources/qt.conf << 'EOF'
[Paths]
Plugins = /opt/homebrew/share/qt/plugins
Libraries = /opt/homebrew/lib

[Qt]
QT_MAC_WANTS_LAYER = 1
QT_STYLE_OVERRIDE = macos
QT_AUTO_SCREEN_SCALE_FACTOR = 1
QT_ENABLE_HIGHDPI_SCALING = 1
QT_SCALE_FACTOR = 1.0
EOF
for lib in libqlcplusengine.dylib libqlcplusui.dylib libqlcpluswebaccess.dylib; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null || true
  done
done
for exe in qlcplus qlcplus-fixtureeditor qlcplus-launcher; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null || true
  done
done
mkdir -p ~/QLC+.app/Contents/PlugIns && \
find build_cmake/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \; && \
mkdir -p ~/QLC+.app/Contents/PlugIns/tls && \
cp /opt/homebrew/share/qt/plugins/tls/libqsecuretransportbackend.dylib ~/QLC+.app/Contents/PlugIns/tls/ && \
for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
  install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
    "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
    ~/QLC+.app/Contents/PlugIns/tls/libqsecuretransportbackend.dylib 2>/dev/null || true
done && \
codesign --force --deep --sign - ~/QLC+.app && \
echo "✅ Gotowe (pluginy: $(ls ~/QLC+.app/Contents/PlugIns/*.dylib | wc -l))" && \
open ~/QLC+.app
```

---

**Autor:** Filip Olszewski  
**Aplikacja:** `~/QLC+.app`
