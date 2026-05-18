# QLC+ — Quick Install (macOS)

Instrukcja budowania i uruchamiania QLC+ ze źródeł na macOS,
łącznie z brancha `feature/vc-widget-plugin-sdk`.

Testowane na: **macOS 15 Sequoia / Apple Silicon + Intel**, Xcode 16, Qt 5.15.

---

## 1. Wymagania wstępne

### Xcode Command Line Tools

```bash
xcode-select --install
```

Sprawdź instalację:

```bash
clang --version   # Apple clang 15+ OK
```

### Homebrew

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Zależności przez Homebrew

```bash
brew install cmake qt@5 pkg-config fftw libsndfile libusb portaudio
```

> **Qt 5.15 jest wymagane.** Qt 6 nie jest wspierane w QLC+ 4.x.
> Homebrew instaluje Qt 5 do `/opt/homebrew/opt/qt@5`.

Upewnij się że Qt 5 jest widoczne dla cmake:

```bash
# Dodaj do ~/.zshrc lub ~/.bash_profile:
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
export Qt5_DIR="/opt/homebrew/opt/qt@5/lib/cmake/Qt5"
```

Załaduj zmiany:

```bash
source ~/.zshrc
```

Sprawdź:

```bash
qmake --version   # powinno pokazać Qt 5.15.x
```

---

## 2. Pobranie kodu źródłowego

```bash
git clone https://github.com/mcallegari/qlcplus.git
cd qlcplus
```

### Przełączenie na branch VC Widget Plugin SDK

```bash
git checkout feature/vc-widget-plugin-sdk
```

---

## 3. Konfiguracja cmake

Utwórz katalog `build/` poza drzewem źródłowym:

```bash
mkdir build && cd build
```

Konfiguracja dla macOS app bundle (`~/QLC+.app`):

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/QLC+.app/Contents" \
  -DINSTALL_ROOT="/" \
  -DQt5_DIR="/opt/homebrew/opt/qt@5/lib/cmake/Qt5" \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@5"
```

> **Debug build** (wolniejszy, ale z symbolami debugowania):
> Zamień `-DCMAKE_BUILD_TYPE=Release` na `-DCMAKE_BUILD_TYPE=Debug`.

Cmake powinien wypisać:
```
Found Qt version 5: /opt/homebrew/opt/qt@5/...
Building QLC+ 4 QtWidget UI
```

---

## 4. Budowanie

```bash
# Wszystkie targety (silnik + UI + pluginy I/O)
cmake --build . --parallel $(sysctl -n hw.logicalcpu)
```

Czas budowania:
- Apple Silicon M-series: **~2–4 min** (Release), ~4–7 min (Debug)
- Intel: **~5–10 min**

---

## 5. Instalacja do app bundle

```bash
cmake --install .
```

Instaluje do `~/QLC+.app/`:
```
~/QLC+.app/
├── Contents/
│   ├── MacOS/
│   │   ├── qlcplus                 ← główny executable
│   │   ├── qlcplus-fixtureeditor
│   │   └── qlcplus-launcher
│   ├── PlugIns/                    ← pluginy I/O (MIDI, ArtNet, HID...)
│   │   ├── libmidi.dylib
│   │   └── ...
│   └── Resources/
│       ├── Fixtures/
│       ├── InputProfiles/
│       └── ...
```

---

## 6. Uruchomienie

### Przez Launcher (zalecane)

```bash
open ~/QLC+.app
```

Lub bezpośrednio:

```bash
~/QLC+.app/Contents/MacOS/qlcplus-launcher
```

### Bezpośrednio (debug / CLI)

```bash
~/QLC+.app/Contents/MacOS/qlcplus
```

---

## 7. Instalacja pluginów VC Widget

Pluginy z brancha (`dmxnumeric`, `hello`) kompilują się osobno —
potrzebują nagłówków z drzewa źródłowego i bibliotek z katalogu build.

### Budowanie pluginów (ze źródeł QLC+)

```bash
cd /ścieżka/do/qlcplus/plugins/vcwidgets/dmxnumeric

mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DQLCPLUS_SRC_DIR="/ścieżka/do/qlcplus" \
  -DQLCPLUS_BUILD_DIR="/ścieżka/do/qlcplus/build" \
  -DQt5_DIR="/opt/homebrew/opt/qt@5/lib/cmake/Qt5" \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@5"

cmake --build .
```

### Instalacja pluginu do app bundle

```bash
# Utwórz folder jeśli nie istnieje
mkdir -p ~/QLC+.app/Contents/PlugIns/VCWidgets

# Skopiuj plugin
cp build/libdmxnumeric_vcwidget.dylib \
   ~/QLC+.app/Contents/PlugIns/VCWidgets/
```

### Instalacja pluginu do folderu użytkownika

Alternatywnie (nie wymaga uprawnień do bundle):

```bash
mkdir -p ~/Library/Application\ Support/QLC+/VCWidgets
cp build/libdmxnumeric_vcwidget.dylib \
   ~/Library/Application\ Support/QLC+/VCWidgets/
```

Oba foldery są skanowane przy każdym starcie QLC+.

### Instalacja przez GUI QLC+

1. Uruchom QLC+
2. Przejdź do `Virtual Console → Add → Get more widgets...`
   (lub `Tools → VC Widget Plugins...`)
3. Kliknij **Install from file...**
4. Wybierz plik `.dylib` lub `.qlcvcw`
5. Uruchom QLC+ ponownie

---

## 8. Weryfikacja

Po restarcie QLC+:

1. Otwórz zakładkę **Virtual Console**
2. Kliknij menu **Add** (lub prawy klik na płótnie)
3. Na końcu listy powinny pojawić się nowe pozycje:
   - `New DMX Numeric` (plugin dmxnumeric)
   - `New Hello Widget` (plugin hello)
4. `Get more widgets...` otwiera okno zarządzania pluginami

---

## 9. Rozwiązywanie problemów

### Plugin nie ładuje się / nie widać w menu

Sprawdź log terminala przy starcie QLC+:

```bash
~/QLC+.app/Contents/MacOS/qlcplus 2>&1 | grep -i "vcwidget\|plugin"
```

Typowe przyczyny:
- **Qt ABI mismatch** — plugin skompilowany inną wersją Qt niż QLC+.
  Sprawdź: `otool -L libdmxnumeric_vcwidget.dylib | grep Qt`
  Wymagana wersja: `Qt5Core.framework/Versions/5/Qt5Core (compatibility version 5.15.0)`
- **Architektura** — arm64 vs x86_64. Sprawdź: `file libdmxnumeric_vcwidget.dylib`
- **Błąd linku** — brak symboli z `libqlcplusui.dylib`. Upewnij się że `QLCPLUS_BUILD_DIR` wskazuje na zbudowany projekt.

### cmake nie znajduje Qt5

```bash
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
# Lub bezpośrednio:
cmake .. -DQt5_DIR=/opt/homebrew/opt/qt@5/lib/cmake/Qt5
```

### Błąd "Permission denied" przy `make install`

Nie używaj `sudo`. Upewnij się że `CMAKE_INSTALL_PREFIX` wskazuje na folder w katalogu domowym (`~/QLC+.app/Contents`).

### Xcode Command Line Tools nieaktualne

```bash
sudo rm -rf /Library/Developer/CommandLineTools
xcode-select --install
```

---

## 10. Szybki rebuild po zmianach

```bash
cd build
cmake --build . --parallel $(sysctl -n hw.logicalcpu) && cmake --install .
```

Tylko UI (szybsze przy zmianach w VC):

```bash
cmake --build . --target qlcplusui --parallel $(sysctl -n hw.logicalcpu)
cmake --install . --component qlcplusui 2>/dev/null || cmake --install .
```

---

## Struktura katalogów po instalacji

```
~/QLC+.app/Contents/
├── MacOS/
│   └── qlcplus                                    ← uruchamiasz to
├── PlugIns/
│   ├── libmidi.dylib                              ← pluginy I/O
│   ├── libartnet.dylib
│   └── VCWidgets/                                 ← pluginy VC Widget (system)
│       ├── libdmxnumeric_vcwidget.dylib
│       └── libhello_vcwidget.dylib
└── Resources/
    ├── Fixtures/
    └── ...

~/Library/Application Support/QLC+/
└── VCWidgets/                                     ← pluginy VC Widget (user, bez sudo)
    └── mywidget.dylib
```
