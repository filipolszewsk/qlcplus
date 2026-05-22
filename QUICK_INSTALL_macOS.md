# QLC+ / GRIDqlc — Quick Install (macOS Apple Silicon)

Instrukcja budowania i uruchamiania GRIDqlc ze źródeł na macOS Apple Silicon (M-series).

**Qt pinned: 6.8.1 LTS** — identyczna wersja lokalnie i w GitHub Actions CI.

Testowane na: **macOS 14/15, Apple Silicon (arm64)**, Xcode 15/16.

---

## Wymagania wstępne

### Xcode Command Line Tools

```bash
xcode-select --install
```

### Homebrew (zależności systemowe — nie Qt)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake python fftw libsndfile libusb libftdi libltc pkg-config
```

> Qt NIE jest instalowane przez Homebrew — Homebrew ma rolling Qt (aktualnie 6.10+), który jest niekompatybilny z pluginami innych developerów. Qt instalujemy przez aqtinstall.

---

## 1. Instalacja Qt 6.8.1 LTS (jednorazowe)

```bash
pip3 install aqtinstall

aqt install-qt mac desktop 6.8.1 clang_64 \
    --outputdir ~/Qt \
    -m qt5compat qt3d qtimageformats qtmultimedia qtserialport qtwebsockets
```

Weryfikacja:

```bash
~/Qt/6.8.1/macos/bin/qmake --version
# Qt version 6.8.1 in /Users/.../Qt/6.8.1/macos
```

**Alternatywnie:** użyj wbudowanego setup:

```bash
./install.sh setup
```

---

## 2. Pobranie kodu źródłowego

```bash
git clone https://github.com/<twoja-org>/qlcplus.git
cd qlcplus
```

---

## 3. Build + Install (jednym poleceniem)

```bash
./install.sh
```

Skrypt automatycznie:
1. Weryfikuje Qt 6.8.1 w `~/Qt/6.8.1/macos`
2. Konfiguruje cmake (usuwa stary build dir jeśli Qt się zmieniło)
3. Buduje QLC+ (pełny build: host + I/O plugins)
4. Instaluje do `~/QLC+.app`
5. Uruchamia `macdeployqt` — bundluje Qt 6.8.1 do app (self-contained)
6. Codesign
7. Weryfikuje instalację

---

## 4. Uruchomienie

```bash
open ~/QLC+.app
```

---

## 5. Instalacja pluginów VC Widget

Pluginy VC Widget to oddzielne `.dylib` tworzone przez developerów zewnętrznych.
Instalacja przez GUI: **Virtual Console → Add → Get more widgets... → Install from file...**

Format dystrybucji: `.qlcvcw` (ZIP z `manifest.json` + binarią).

Katalog użytkownika (hot-reload bez restartu):

```
~/Library/Application Support/QLC+/VCWidgets/
```

> Pluginy muszą być zbudowane na **Qt 6.8.1** (identyczna wersja jak host).
> Patrz `plugins/vcwidgets/VC_WIDGET_PLUGIN_DEV_GUIDE.md` — pełna instrukcja SDK.

---

## 6. Weryfikacja istniejącej instalacji

```bash
./install.sh verify
```

---

## 7. Rebuild po zmianach

```bash
./install.sh
```

---

## Struktura katalogów po instalacji

```
~/QLC+.app/
└── Contents/
    ├── MacOS/
    │   ├── qlcplus                ← główny executable
    │   ├── qlcplus-fixtureeditor
    │   └── qlcplus-launcher
    ├── Frameworks/
    │   ├── QtCore.framework       ← Qt 6.8.1 (bundlowane przez macdeployqt)
    │   ├── QtWidgets.framework
    │   ├── libqlcplusui.dylib
    │   └── libqlcplusengine.dylib
    ├── PlugIns/
    │   ├── libmidi.dylib          ← I/O plugins
    │   ├── libartnet.dylib
    │   └── ...
    └── Resources/
        ├── Fixtures/
        └── ...

~/Library/Application Support/QLC+/
└── VCWidgets/                     ← user VC Widget plugins (hot-reload)
    └── libmyplugin_vcwidget.dylib
```

---

## Rozwiązywanie problemów

### `./install.sh setup` — nie znaleziono pip3

```bash
brew install python
# następnie:
./install.sh setup
```

### Plugin VC Widget: "incompatible Qt library"

Plugin zbudowany na innej wersji Qt. Sprawdź:
```bash
otool -L ~/Library/Application\ Support/QLC+/VCWidgets/libplugin.dylib | grep QtCore
# musi być: current version 6.8.x
```
Rozwiązanie: przebuduj plugin z `-DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.1/macos/lib/cmake"`.

### CMake nie widzi Qt

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.1/macos/lib/cmake"
```

### Stara wersja w `/Applications/QLC+.app`

Zawsze otwieraj `~/QLC+.app`, nie `/Applications`. Użyj `open ~/QLC+.app`.
