# GRIDqlc macOS DMG — Notatki techniczne

Dokument opisuje jak działa build DMG, jakie problemy wystąpiły i jak je naprawiono.
Aktualizuj go przy każdej zmianie procesu budowania.

---

## Jak zbudować DMG

```bash
cd "/Users/filipolszewski/Documents/qlc projekty/qlcplus"
./create-dmg-release.sh
```

Skrypt robi wszystko od zera: cmake → build → install → fix_dylib_deps → macdeployqt → purge Homebrew → signing → DMG.

**Wymagania wstępne:**
- Xcode Command Line Tools (`xcode-select --install`)
- Homebrew Qt (`brew install qt`) lub Qt zainstalowane w `~/Qt/`
- `create-dmg` w `platforms/macos/dmg/` (już jest w repo)

**Opcje:**
```bash
# Ad-hoc signing (domyślne, na własny użytek)
./create-dmg-release.sh

# Z prawdziwym certyfikatem Apple (do dystrybucji poza App Store)
SIGNATURE="Developer ID Application: Twoje Imię (TEAMID)" ./create-dmg-release.sh

# Inna lokalizacja Qt
QTDIR=~/Qt/6.9.1/macos ./create-dmg-release.sh
```

Gotowy plik: `GRIDqlc_<wersja>.dmg` w katalogu projektu.

---

## Architektura bundlu

```
QLC+.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   ├── qlcplus-launcher     ← główny executable (uruchamia Finder)
│   │   ├── qlcplus              ← właściwa aplikacja
│   │   └── qlcplus-fixtureeditor
│   ├── Frameworks/
│   │   ├── QtCore.framework/    ← Qt zbundlowany przez macdeployqt
│   │   ├── QtWidgets.framework/
│   │   ├── libqlcplusengine.dylib
│   │   ├── libsndfile.1.0.37.dylib
│   │   └── ... (inne dyliby)
│   ├── PlugIns/
│   │   ├── platforms/libqcocoa.dylib   ← Qt Cocoa plugin (krytyczny!)
│   │   └── ...
│   └── Resources/
```

---

## Krytyczny fix: Duplicate Qt Load

### Problem (symptom)
Aplikacja nie uruchamia się po instalacji z DMG. W konsoli:
```
objc: Class QNSApplication is implemented in both
  /Applications/QLC+.app/Contents/Frameworks/QtCore.framework/...
  /opt/homebrew/Cellar/qtbase/6.x/lib/QtCore.framework/...
qt.qpa.plugin: Could not find the Qt platform plugin "cocoa"
```

### Przyczyna
dyld ładował **dwa zestawy Qt jednocześnie** — z bundla i z Homebrew, bo:

1. **Homebrew RPATH był zapisany w binarnych** — dyld widząc `@rpath/QtCore.framework/...`
   sprawdza RPATHy po kolei; jeśli `/opt/homebrew/opt/qt/lib` był pierwszy, wygrywał Homebrew:
   ```
   # BAD (przed naprawą):
   qlcplus-launcher LC_RPATH: /opt/homebrew/opt/qt/lib   ← Homebrew wygrywa!
   qlcplus-launcher LC_RPATH: @executable_path/../Frameworks
   ```

2. **Cztery frameworki Qt miały `install_name` wskazujący na Homebrew** — macdeployqt
   kopiował je do bundla ale zapominał przepisać ich własny identyfikator:
   ```
   # BAD (przed naprawą):
   QtDBus.framework/Versions/A/QtDBus install_name:
     /opt/homebrew/opt/qtbase/lib/QtDBus.framework/...   ← Homebrew!
   ```

### Rozwiązanie (zaimplementowane w `create-dmg-release.sh`)

Blok **"Purging Homebrew references from bundle"** po `macdeployqt`:

**A. Usuń Homebrew RPATHy z każdego Mach-O:**
```bash
otool -l "$f" | awk '/LC_RPATH/{r=1;next} r && /path/{print $2; r=0}' | \
    grep -E "^(/opt/homebrew|/usr/local)" | while read -r rp; do
    install_name_tool -delete_rpath "$rp" "$f"
done
```

**B. Napraw `install_name` frameworków Qt które macdeployqt pominął:**
```bash
# Dla każdego .framework w bundlu który ma Homebrew install_name:
install_name_tool -id \
    "@executable_path/../Frameworks/$name.framework/Versions/A/$name" \
    "$fw/Versions/A/$name"
```

**C. Przepisz pozostałe `/opt/homebrew/...` dependencje:**
```bash
otool -L "$f" | awk 'NR>1{print $1}' | grep "^/opt/homebrew" | \
while read -r dep; do
    install_name_tool -change "$dep" "@executable_path/../Frameworks/..." "$f"
done
```

### Weryfikacja po buildzie

```bash
# 1. Brak Homebrew RPATHów (pusta odpowiedź = OK)
find ~/QLC+.app -type f | while read -r f; do
    otool -l "$f" 2>/dev/null | awk '/LC_RPATH/{r=1;next} r && /path/{print $2; r=0}' | \
        grep -E "/opt/homebrew|/usr/local" | while read -r rp; do
        echo "BAD RPATH: $f -> $rp"
    done
done

# 2. Brak Homebrew dependencies (pusta odpowiedź = OK)
find ~/QLC+.app -type f | while read -r f; do
    otool -L "$f" 2>/dev/null | awk 'NR>1{print $1}' | \
        grep -E "/opt/homebrew|/usr/local" | while read -r dep; do
        echo "BAD DEP: $f -> $dep"
    done
done

# 3. Test uruchomienia w czystym env (brak "implemented in both" = OK)
env -i HOME="$HOME" PATH="/usr/bin:/bin" ~/QLC+.app/Contents/MacOS/qlcplus-launcher 2>&1 &
sleep 5
kill %1
```

**Uwaga:** Kilka audio dylibs (`libmpg123`, `libFLAC`, `libvorbisenc`, `libopus`) będzie miało
swój własny `install_name` wskazujący na Homebrew — to normalne i nieszkodliwe, bo żaden
inny binary nie odwołuje się do nich przez ścieżkę Homebrew (używają `@executable_path/...`).

---

## Signing (podpisywanie kodu)

Kolejność jest krytyczna:

1. Strip starych podpisów Qt Company z `.framework` dirs (`codesign --remove-signature`)
2. Podpisz luźne `.dylib` (libsndfile, libfftw, itp.)
3. Podpisz PlugIns `.dylib`
4. Podpisz całe `.framework` bundle directories
5. Podpisz executables w `MacOS/` (z `--options runtime` i `--entitlements`)
6. Usuń temp pliki `*-e` które codesign tworzy przy `--entitlements`
7. Podpisz cały `.app` bundle z `--deep`

**Entitlements** (`platforms/macos/qlcplus.entitlements`) musi zawierać:
```xml
<key>com.apple.security.cs.disable-library-validation</key>
<true/>
```
Bez tego hardened runtime blokuje ładowanie dylibs z niezgodnymi Team ID.

---

## Kluczowe pliki

| Plik | Opis |
|------|------|
| `create-dmg-release.sh` | Główny skrypt budowania DMG |
| `platforms/macos/fix_dylib_deps.sh` | Naprawia non-Qt Homebrew deps (libsndfile, fftw itd.) |
| `platforms/macos/qlcplus.entitlements` | Entitlements macOS dla hardened runtime |
| `platforms/macos/dmg/Install.command` | Skrypt w DMG, usuwa quarantine po instalacji |
| `platforms/macos/dmg/background.png` | Tło okna DMG |
| `DMG_BUILD_NOTES.md` | Ten dokument |

---

## Typowe błędy i rozwiązania

### `make: *** No targets specified`
Stary styl buildu. Skrypt używa teraz `cmake -S . -B build_release` i `cmake --build`.

### `rm: no matches found: *.dmg`
zsh globbing na pustym katalogu. Naprawione przez `find ... -name "*.dmg" -delete`.

### `code object is not signed at all / Info.plist-e`
`codesign --entitlements` tworzy temp pliki `Info.plist-e`. Skrypt usuwa je przed
podpisaniem bundle: `find "$APP_DIR" -name "*-e" -delete`.

### `invalid Info.plist` podczas `codesign --verify --deep --strict`
macdeployqt modyfikuje Info.plist po podpisaniu. Rozwiązanie: podpisz bundle z `--deep`
NA KOŃCU, po wszystkich innych operacjach.

### `Library not loaded: different Team IDs`
Qt frameworks były podpisane przez Qt Company — inny Team ID niż ad-hoc.
Rozwiązanie: `codesign --remove-signature` na każdym `.framework` przed re-signing.

### `objc: Class X is implemented in both` + cocoa crash
Duplicate Qt load — patrz sekcja "Krytyczny fix" powyżej.

---

## Performance fix: VCSliderProperties

Otwieranie Widget Properties dla sliderów było bardzo wolne przy dużej liczbie fixture.

**Przyczyna:** `QTreeWidget` robił repaint i sort po każdym dodanym elemencie.

**Fix** (`ui/src/virtualconsole/vcsliderproperties.cpp`):
- Wyłącz sorting i updates podczas wypełniania listy:
  ```cpp
  m_levelList->setSortingEnabled(false);
  m_levelList->setUpdatesEnabled(false);
  // ... dodaj elementy ...
  m_levelList->setUpdatesEnabled(true);
  m_levelList->setSortingEnabled(true);
  ```
- Cache ikon fixture i kanałów (`QHash<int, QIcon>`) — unika wielokrotnego ładowania SVG.
