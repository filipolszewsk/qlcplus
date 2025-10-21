# Instrukcja instalacji QLC+ na macOS (Apple Silicon)

## Wymagania systemowe

- **macOS:** 11.0+ (Big Sur lub nowszy)
- **Procesor:** Apple Silicon (M1/M2/M3/M4)
- **Narzędzia:** Homebrew, Xcode Command Line Tools
- **Czas instalacji:** ~10-15 minut

## Krok 1: Instalacja zależności przez Homebrew

```bash
# Instalacja Qt 6 i wszystkich wymaganych komponentów
brew install qt@6 qtmultimedia qtserialport qtwebsockets qtsvg

# Instalacja bibliotek audio i innych zależności
brew install libsndfile fftw mad portaudio libftdi libusb
```

## Krok 2: Pobranie kodu źródłowego QLC+

```bash
# Sklonuj repozytorium (jeśli jeszcze nie masz)
git clone https://github.com/mcallegari/qlcplus.git
cd qlcplus
```

## Krok 3: Konfiguracja CMake z Qt 6

```bash
# Utwórz czysty katalog build
rm -rf build
mkdir build
cd build

# Konfiguracja CMake z Qt 6
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qtbase/lib/cmake" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_BUILD_TYPE=Release \
      ..
```

**Parametry:**
- `CMAKE_PREFIX_PATH` - ścieżka do Qt 6
- `CMAKE_OSX_DEPLOYMENT_TARGET` - minimalna wersja macOS (11.0 = Big Sur)
- `CMAKE_BUILD_TYPE=Release` - zoptymalizowana wersja produkcyjna

## Krok 4: Kompilacja

```bash
# Kompilacja wielordzeniowa (automatycznie wykrywa liczbę rdzeni)
make -j$(sysctl -n hw.ncpu)
```

## Krok 5: Instalacja podstawowa

```bash
# Instalacja do ~/QLC+.app
make install/fast
```

## Krok 6: Usunięcie duplikatów Qt (KLUCZOWE!)

```bash
# Usuń Qt frameworks z bundle (zapobiega duplikatom i konfliktom)
rm -rf ~/QLC+.app/Contents/Frameworks/Qt*.framework

# Usuń Qt pluginy z bundle (będziemy używać z Homebrew)
rm -rf ~/QLC+.app/Contents/PlugIns
```

**WAŻNE:** Ten krok jest krytyczny! Bez tego będą duplikaty Qt i aplikacja nie będzie działać poprawnie.

## Krok 7: Konfiguracja Qt poprzez qt.conf

```bash
# Utwórz qt.conf wskazujący na Homebrew Qt
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
```

**Co to robi:**
- `Plugins` - wskazuje na pluginy Qt z Homebrew
- `Libraries` - wskazuje na biblioteki Qt z Homebrew
- `QT_MAC_WANTS_LAYER` - włącza hardware acceleration
- `QT_STYLE_OVERRIDE = macos` - wymusza natywny styl macOS (naprawia problemy z kolorami!)

## Krok 8: Naprawa ścieżek bibliotek dynamicznych

**WAŻNE:** Ten krok naprawia linkowanie Qt - bez tego aplikacja nie uruchomi się!

```bash
# Napraw ścieżki Qt w bibliotekach QLC+
for lib in libqlcplusengine.dylib libqlcplusui.dylib libqlcpluswebaccess.dylib; do
  install_name_tool -change "@executable_path/../Frameworks/QtCore" "/opt/homebrew/lib/QtCore.framework/Versions/A/QtCore" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtGui" "/opt/homebrew/lib/QtGui.framework/Versions/A/QtGui" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtWidgets" "/opt/homebrew/lib/QtWidgets.framework/Versions/A/QtWidgets" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtNetwork" "/opt/homebrew/lib/QtNetwork.framework/Versions/A/QtNetwork" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtMultimedia" "/opt/homebrew/lib/QtMultimedia.framework/Versions/A/QtMultimedia" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtMultimediaWidgets" "/opt/homebrew/lib/QtMultimediaWidgets.framework/Versions/A/QtMultimediaWidgets" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtWebSockets" "/opt/homebrew/lib/QtWebSockets.framework/Versions/A/QtWebSockets" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtQml" "/opt/homebrew/lib/QtQml.framework/Versions/A/QtQml" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtSerialPort" "/opt/homebrew/lib/QtSerialPort.framework/Versions/A/QtSerialPort" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtSvg" "/opt/homebrew/lib/QtSvg.framework/Versions/A/QtSvg" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtPrintSupport" "/opt/homebrew/lib/QtPrintSupport.framework/Versions/A/QtPrintSupport" ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null
done

# Napraw ścieżki Qt w plikach wykonywalnych
for exe in qlcplus qlcplus-fixtureeditor qlcplus-launcher; do
  install_name_tool -change "@executable_path/../Frameworks/QtCore" "/opt/homebrew/lib/QtCore.framework/Versions/A/QtCore" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtGui" "/opt/homebrew/lib/QtGui.framework/Versions/A/QtGui" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtWidgets" "/opt/homebrew/lib/QtWidgets.framework/Versions/A/QtWidgets" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtNetwork" "/opt/homebrew/lib/QtNetwork.framework/Versions/A/QtNetwork" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtMultimedia" "/opt/homebrew/lib/QtMultimedia.framework/Versions/A/QtMultimedia" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtMultimediaWidgets" "/opt/homebrew/lib/QtMultimediaWidgets.framework/Versions/A/QtMultimediaWidgets" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtWebSockets" "/opt/homebrew/lib/QtWebSockets.framework/Versions/A/QtWebSockets" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtQml" "/opt/homebrew/lib/QtQml.framework/Versions/A/QtQml" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtSerialPort" "/opt/homebrew/lib/QtSerialPort.framework/Versions/A/QtSerialPort" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtSvg" "/opt/homebrew/lib/QtSvg.framework/Versions/A/QtSvg" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
  install_name_tool -change "@executable_path/../Frameworks/QtPrintSupport" "/opt/homebrew/lib/QtPrintSupport.framework/Versions/A/QtPrintSupport" ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null
done
```

## Krok 9: Instalacja pluginów QLC+

```bash
# Utwórz katalog pluginów
mkdir -p ~/QLC+.app/Contents/PlugIns

# Skopiuj wszystkie pluginy QLC+
find build/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \;

# Lista zainstalowanych pluginów (powinno być 12):
# - libartnet.dylib (ArtNet)
# - libdmxusb.dylib (DMX USB)
# - libe131.dylib (E1.31 sACN)
# - libenttecwing.dylib (ENTTEC Wing)
# - libhidplugin.dylib (HID)
# - libloopback.dylib (Loopback)
# - libmidiplugin.dylib (MIDI)
# - libos2l.dylib (OS2L)
# - libosc.dylib (OSC)
# - libpeperoni.dylib (Peperoni)
# - libudmx.dylib (uDMX)
# - libvelleman.dylib (Velleman)
```

## Krok 10: Podpisanie aplikacji

```bash
# Podpisz aplikację (wymagane przez macOS)
codesign --force --deep --sign - ~/QLC+.app
```

## Krok 11: Uruchomienie

```bash
# Uruchom QLC+
open ~/QLC+.app
```

Lub kliknij dwukrotnie na `QLC+.app` w katalogu domowym przez Finder.

## Weryfikacja instalacji

Po uruchomieniu sprawdź:
- ✅ Okno QLC+ się otwiera
- ✅ Interfejs ma natywny styl macOS
- ✅ **Kropki w XY Pad są SZARE (nie różowe)**
- ✅ Pluginy (DMX, ArtNet, MIDI) są widoczne w Input/Output Manager
- ✅ Brak błędów w konsoli

## Rozwiązywanie problemów

### Problem 1: Różowe/fioletowe kropki w XY Pad

**Przyczyna:** Qt używa niewłaściwego stylu lub ma wyłączone natywne renderowanie

**Rozwiązanie:**
```bash
# Zamknij QLC+
pkill -9 qlcplus

# Sprawdź qt.conf
cat ~/QLC+.app/Contents/Resources/qt.conf

# Upewnij się że zawiera:
# QT_MAC_WANTS_LAYER = 1
# QT_STYLE_OVERRIDE = macos
```

### Problem 2: "Could not load the Qt platform plugin 'cocoa'"

**Przyczyna:** Brakujące lub nieprawidłowe pluginy Qt

**Rozwiązanie:**
```bash
# Sprawdź czy qt.conf wskazuje na właściwą ścieżkę
cat ~/QLC+.app/Contents/Resources/qt.conf

# Powinno być:
# [Paths]
# Plugins = /opt/homebrew/share/qt/plugins
```

### Problem 3: Duplikaty bibliotek Qt (warning o duplicate classes)

**Przyczyna:** Qt frameworks są zarówno w bundle jak i w Homebrew

**Rozwiązanie:**
```bash
# Usuń Qt frameworks z bundle
rm -rf ~/QLC+.app/Contents/Frameworks/Qt*.framework

# Podpisz ponownie
codesign --force --deep --sign - ~/QLC+.app
```

### Problem 4: Aplikacja się crashuje przy starcie

**Przyczyna:** Nieprawidłowe ścieżki bibliotek

**Rozwiązanie:**
```bash
# Sprawdź ścieżki bibliotek
otool -L ~/QLC+.app/Contents/MacOS/qlcplus | grep Qt

# Wszystkie Qt powinny wskazywać na /opt/homebrew/lib/
# Jeśli nie, powtórz Krok 8 (naprawa ścieżek)
```

### Problem 5: Pluginy QLC+ nie działają

**Przyczyna:** Pluginy nie zostały skopiowane lub mają złe ścieżki

**Rozwiązanie:**
```bash
# Sprawdź czy pluginy są zainstalowane
ls ~/QLC+.app/Contents/PlugIns/

# Powinno być 12 plików .dylib
# Jeśli brakuje, powtórz Krok 9
```

## Weryfikacja techniczna

### Sprawdzenie architektury
```bash
file ~/QLC+.app/Contents/MacOS/qlcplus
# Powinno pokazać: Mach-O 64-bit executable arm64
```

### Sprawdzenie Qt dependencies
```bash
otool -L ~/QLC+.app/Contents/MacOS/qlcplus | grep Qt
# Wszystkie Qt powinny być z /opt/homebrew/lib/
```

### Sprawdzenie zainstalowanych pluginów
```bash
ls -1 ~/QLC+.app/Contents/PlugIns/ | wc -l
# Powinno pokazać: 12
```

## Automatyczny skrypt instalacji

Możesz utworzyć skrypt `install_qlcplus.sh` który wykona wszystkie kroki automatycznie:

```bash
#!/bin/bash
set -e

echo "QLC+ Installation Script for macOS Apple Silicon"
echo "================================================"

# Przejdź do katalogu QLC+
cd "$(dirname "$0")"

# Build
rm -rf build
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qtbase/lib/cmake" \
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
find build/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \;

# Podpisanie
codesign --force --deep --sign - ~/QLC+.app

echo ""
echo "✅ Instalacja zakończona!"
echo "📍 Aplikacja: ~/QLC+.app"
echo "🚀 Uruchom: open ~/QLC+.app"
```

Zapisz jako `install_qlcplus.sh`, nadaj uprawnienia wykonywania (`chmod +x install_qlcplus.sh`) i uruchom: `./install_qlcplus.sh`

## Podsumowanie końcowe

Po wykonaniu wszystkich kroków otrzymasz:

✅ **QLC+ 4.14.4 GIT** - w pełni funkcjonalny  
✅ **Qt 6.9.3** - z Homebrew (bez duplikatów)  
✅ **ARM64 native** - pełne wsparcie Apple Silicon  
✅ **12 pluginów** - DMX, ArtNet, MIDI, OSC, E1.31, HID, etc.  
✅ **Natywny styl macOS** - szare kropki w XY Pad  
✅ **Rozmiar:** ~62 MB  
✅ **Lokalizacja:** `~/QLC+.app`  

## Uruchomienie

```bash
open ~/QLC+.app
```

Lub kliknij dwukrotnie na aplikację w Finderze.

## Uwagi końcowe

- **Bez wrapperów** - czysta instalacja macOS
- **Bez hacków** - wszystko poprawnie skonfigurowane
- **Pluginy działają** - pełne wsparcie DMX/ArtNet/MIDI
- **Retina ready** - obsługa wyświetlaczy high-DPI
- **Testowane na:** macOS 15.6.1 (Sequoia) / Apple Silicon M2

---

**Autor:** Filip Olszewski  
**Data:** Październik 2024  
**Wersja Qt:** 6.9.3  
**Wersja QLC+:** 4.14.4 GIT

