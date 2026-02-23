# Instrukcja instalacji GRIDqlc na macOS (Apple Silicon)

## Wymagania systemowe

- **macOS:** 11.0+ (Big Sur lub nowszy)
- **Procesor:** Apple Silicon (M1/M2/M3/M4)
- **Narzędzia:** Homebrew, Xcode Command Line Tools
- **Czas instalacji:** ~10-15 minut

## Krok 1: Instalacja zależności przez Homebrew

```bash
# Instalacja Qt 6 i wszystkich wymaganych komponentów
brew install qt

# Instalacja bibliotek audio i innych zależności
brew install libsndfile fftw mad portaudio libftdi libusb
```

## Krok 2: Pobranie kodu źródłowego GRIDqlc

```bash
# Sklonuj repozytorium forka GRIDqlc
git clone https://github.com/filipolszewsk/qlcplus.git
cd qlcplus

# Przełącz na branch z licencjonowaniem
git checkout feature/license-protection
```

## Krok 3: Konfiguracja CMake

```bash
# Utwórz czysty katalog build
rm -rf build_cmake
mkdir build_cmake
cd build_cmake

# Konfiguracja CMake z Qt 6
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_BUILD_TYPE=Release \
      ..
```

**Parametry:**
- `CMAKE_PREFIX_PATH` - ścieżka do Qt 6 z Homebrew
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

## Krok 8: Naprawa ścieżek bibliotek dynamicznych

**WAŻNE:** Ten krok naprawia linkowanie Qt - bez tego aplikacja nie uruchomi się!

```bash
# Napraw ścieżki Qt w bibliotekach GRIDqlc
for lib in libqlcplusengine.dylib libqlcplusui.dylib libqlcpluswebaccess.dylib; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/Frameworks/$lib 2>/dev/null || true
  done
done

# Napraw ścieżki Qt w plikach wykonywalnych
for exe in qlcplus qlcplus-fixtureeditor qlcplus-launcher; do
  for qtfw in QtCore QtGui QtWidgets QtNetwork QtMultimedia QtMultimediaWidgets QtWebSockets QtQml QtSerialPort QtSvg QtPrintSupport; do
    install_name_tool -change "@executable_path/../Frameworks/$qtfw" \
      "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
      ~/QLC+.app/Contents/MacOS/$exe 2>/dev/null || true
  done
done
```

## Krok 9: Instalacja pluginów QLC+

```bash
# Utwórz katalog pluginów
mkdir -p ~/QLC+.app/Contents/PlugIns

# Skopiuj wszystkie pluginy
find build_cmake/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \;

# Sprawdź liczbę pluginów (powinno być 12):
ls ~/QLC+.app/Contents/PlugIns/*.dylib | wc -l
```

## Krok 10: Podpisanie aplikacji

```bash
codesign --force --deep --sign - ~/QLC+.app
```

## Krok 11: Uruchomienie

```bash
open ~/QLC+.app
```

Lub kliknij dwukrotnie na `QLC+.app` w katalogu domowym w Finderze.

---

## Aktywacja licencji premium (GRIDqlc)

Po zainstalowaniu możesz aktywować licencję premium żeby odblokować zaszyfrowane skrypty RGB.

### Krok 1: Pobierz swój Hardware ID

Uruchom GRIDqlc i wejdź w **Help → Premium License**. Kliknij **Copy Hardware ID** żeby skopiować unikalny identyfikator swojego komputera.

### Krok 2: Aktywacja online

Wejdź na **[gridqlc.com/activation](https://gridqlc.com/activation)** i:
1. Wklej klucz licencyjny z emaila po zakupie
2. Wklej swój Hardware ID
3. Kliknij **Activate & Download license.qlckey**

### Krok 3: Instalacja pliku licencji

Przenieś pobrany plik `license.qlckey` do:

```
~/Library/Application Support/QLC+/license.qlckey
```

Czyli w terminalu:
```bash
mv ~/Downloads/license.qlckey ~/Library/Application\ Support/QLC+/license.qlckey
```

### Krok 4: Weryfikacja

Uruchom GRIDqlc ponownie. W tytule okna powinno pojawić się:
```
Q Light Controller Plus [Twoje Imię - GRIDqlc] - New Workspace
```

W **Help → Premium License** status powinien pokazywać `✔  ACTIVE`.

### Instalacja skryptów premium

Umieść zaszyfrowane pliki `.qlcscript` w:

```
~/Library/Application Support/QLC+/RGBScripts/
```

GRIDqlc automatycznie je odczyta i odszyfruje przy starcie.

---

## Weryfikacja instalacji

Po uruchomieniu sprawdź:
- ✅ Okno GRIDqlc się otwiera z tytułem `Q Light Controller Plus`
- ✅ Interfejs ma natywny styl macOS
- ✅ Kropki w XY Pad są **szare** (nie różowe)
- ✅ Pluginy (DMX, ArtNet, MIDI) widoczne w Input/Output Manager
- ✅ Po aktywacji: tytuł zawiera `[Imię Nazwisko - GRIDqlc]`

## Rozwiązywanie problemów

### Problem 1: Różowe/fioletowe kropki w XY Pad

```bash
pkill -9 qlcplus
cat ~/QLC+.app/Contents/Resources/qt.conf
# Upewnij się że zawiera QT_STYLE_OVERRIDE = macos
```

### Problem 2: "Could not load the Qt platform plugin 'cocoa'"

```bash
cat ~/QLC+.app/Contents/Resources/qt.conf
# Plugins powinno wskazywać na /opt/homebrew/share/qt/plugins
```

### Problem 3: Aplikacja się crashuje przy starcie

```bash
otool -L ~/QLC+.app/Contents/MacOS/qlcplus | grep Qt
# Wszystkie Qt powinny wskazywać na /opt/homebrew/lib/
# Jeśli nie - powtórz Krok 8
```

### Problem 4: Licencja nie jest wykrywana

```bash
ls ~/Library/Application\ Support/QLC+/license.qlckey
# Plik musi być DOKŁADNIE w tej lokalizacji
# Sprawdź czy plik nie jest pusty:
wc -c ~/Library/Application\ Support/QLC+/license.qlckey
```

### Problem 5: Skrypty premium nie działają (brak animacji)

- Sprawdź czy licencja jest aktywna (Help → Premium License → status ACTIVE)
- Sprawdź czy pliki `.qlcscript` są w `~/Library/Application Support/QLC+/RGBScripts/`
- Sprawdź czy plik licencji pasuje do Twojego komputera (Hardware ID musi się zgadzać)

---

## Weryfikacja techniczna

```bash
# Architektura (powinno być arm64)
file ~/QLC+.app/Contents/MacOS/qlcplus

# Qt dependencies (wszystkie z /opt/homebrew/lib/)
otool -L ~/QLC+.app/Contents/MacOS/qlcplus | grep Qt

# Liczba pluginów (powinno być 12)
ls ~/QLC+.app/Contents/PlugIns/*.dylib | wc -l
```

---

## Automatyczny skrypt instalacji

```bash
#!/bin/bash
set -e

echo "GRIDqlc Installation Script for macOS Apple Silicon"
echo "===================================================="

cd "$(dirname "$0")"

# Build
rm -rf build_cmake
mkdir build_cmake
cd build_cmake

cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_BUILD_TYPE=Release \
      ..

make -j$(sysctl -n hw.ncpu)
make install/fast
cd ..

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

# Instalacja pluginów
mkdir -p ~/QLC+.app/Contents/PlugIns
find build_cmake/plugins -name "*.dylib" -type f -exec cp {} ~/QLC+.app/Contents/PlugIns/ \;

# Podpisanie
codesign --force --deep --sign - ~/QLC+.app

echo ""
echo "✅ Instalacja zakończona!"
echo "📍 Aplikacja: ~/QLC+.app"
echo "🔑 Aktywacja licencji: https://gridqlc.com/activation"
echo "🚀 Uruchom: open ~/QLC+.app"
```

Zapisz jako `install.sh`, nadaj uprawnienia i uruchom:
```bash
chmod +x install.sh
./install.sh
```

---

## Podsumowanie

Po wykonaniu wszystkich kroków otrzymasz:

✅ **GRIDqlc** - fork QLC+ z systemem licencjonowania  
✅ **Qt 6** - z Homebrew (bez duplikatów)  
✅ **ARM64 native** - pełne wsparcie Apple Silicon  
✅ **12 pluginów** - DMX, ArtNet, MIDI, OSC, E1.31, HID, etc.  
✅ **Natywny styl macOS** - szare kropki w XY Pad  
✅ **System licencji** - premium skrypty RGB przez `license.qlckey`  
✅ **Lokalizacja:** `~/QLC+.app`

---

**Autor:** Filip Olszewski  
**Strona:** [gridqlc.com](https://gridqlc.com)  
**Aktywacja:** [gridqlc.com/activation](https://gridqlc.com/activation)  
**Qt:** 6.x (Homebrew)  
**Testowane na:** macOS 26 (Tahoe) / Apple Silicon
