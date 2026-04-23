# Instrukcje budowania Windows Installera i macOS DMG dla QLC+ 4.14

> **Cel dokumentu:** krok po kroku jak skonfigurować GitHub Actions żeby od razu zbudowało się Windows `.exe` i macOS `.dmg` bez wielu iteracji.

## TL;DR — Szybka lista kontrolna dla nowego brancha

Żeby build przeszedł od razu, zrób **po kolei**:

1. **Dodaj branch do workflow triggerów** w `.github/workflows/build-windows-v4.yml` oraz `build-macos-v4.yml` (lista `branches:`)
2. **Sprawdź kolejność inicjalizacji zmiennych członkowskich** we wszystkich nowo dodanych `.cpp/.h` (musi odpowiadać kolejności w `.h`) — patrz sekcja "Typowe błędy kompilacji"
3. **Jeśli dodałeś nowe biblioteki zewnętrzne**, sprawdź czy są dostępne w MSYS2/brew lub dodaj krok budowania ze źródeł
4. **Push** — workflow uruchomi się automatycznie

Jeśli build się wywali, kroki naprawy są w sekcji "Znane pułapki".

---

## Workflow Windows (`build-windows-v4.yml`)

### Triggery

```yaml
on:
  push:
    branches: [ main, master, <lista twoich branchy> ]
  pull_request:
    branches: [ main, master ]
  workflow_dispatch:
```

**WAŻNE:** Jeśli pracujesz na własnym branchu, **dodaj go do listy** lub używaj `workflow_dispatch` do ręcznego uruchomienia.

### Co ten workflow robi (skrót)

1. Checkout kodu
2. Instalacja Qt 6.8.1 przez `jurplel/install-qt-action@v4`
3. Instalacja MSYS2 z pakietami (gcc, cmake, ninja, libsndfile, fftw, libusb, nsis, ...)
4. **Build libltc ze źródeł** (nie ma gotowego pakietu w MSYS2)
5. Pobranie i kompilacja FTDI D2XX SDK
6. Patche pre-build: `-Werror` off, `Debug→Release`, disable Velleman, fix ścieżek MSYS2/NSIS
7. CMake configure + Ninja build + install do `/c/qlcplus`
8. `windeployqt` (kopiuje DLL-e Qt)
9. NSIS build installera
10. Upload `.exe` jako artifact (retention: 30 dni)

Artifact: `QLC+-build-v4-{APPVERSION}-{BUILD_DATE}-{GIT_REV}.exe`

---

## Workflow macOS (`build-macos-v4.yml`)

### Triggery — analogicznie jak Windows

### Co robi

1. Checkout kodu (runner `macos-13`)
2. Zależności przez Homebrew: `fftw mad libsndfile libftdi libltc`
3. Qt 6.8.1 (`macos` / `clang_64`) przez `jurplel/install-qt-action@v4`
4. CMake configure + `make -j`
5. `make install/fast` (instaluje do `~/QLC+.app`)
6. Fix dylib dependencies + `macdeployqt`
7. Tworzenie DMG przez `platforms/macos/dmg/create-dmg`
8. Upload `.dmg` jako artifact

Artifact: `QLC+-{APPVERSION}-{BUILD_DATE}-{GIT_REV}.dmg`

---

## Typowe błędy kompilacji i jak im zapobiec

### 1. `-Werror=reorder` (kolejność inicjalizacji w C++)

**Objaw:**
```
error: 'Klasa::m_pole_B' will be initialized after [-Werror=reorder]
error:   'Typ Klasa::m_pole_A'
error:   when initialized here [-Werror=reorder]
```

**Przyczyna:** W konstruktorze `: m_pole_A(...), m_pole_B(...)` musi odpowiadać **kolejności deklaracji w `.h`**. GCC 15 (MSYS2) traktuje to jako błąd.

**Jak naprawić od razu:**
- Przy dodawaniu nowego pola do klasy, zawsze dodaj go w **takim samym miejscu** w `.h` i w liście inicjalizacyjnej `.cpp`
- Domyślnie nasz workflow Windows **wyłącza `-Werror`** (patrz niżej), więc nie powinno już blokować buildów — ale dobre praktyki ich zniechęcają

**Obejście w workflow (już wdrożone):**
```bash
sed -i -e 's/set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")/#&/' variables.cmake
```

### 2. `-Werror=dangling-else`

**Objaw:**
```
error: suggest explicit braces to avoid ambiguous 'else' [-Werror=dangling-else]
```

**Jak naprawić:** dodaj `{}` wokół `if` wewnątrz innego `if`:
```cpp
if (warunek1) {
    if (warunek2)
        akcja();
} else {
    inna_akcja();
}
```

**Workflow obejście:** `-Werror` jest wyłączony (patrz wyżej).

### 3. `libltc not found`

**Objaw:**
```
CMake Error: libltc not found. Install it with: brew install libltc
```

**Gdzie występuje:** `ui/src/CMakeLists.txt` — używane przez `ltctimecodeengine` i `ltctimecodewidget` (LTC Timecode w Show Manager).

**Rozwiązanie:**
- **macOS:** `brew install libltc` (zrobione w workflow)
- **Windows:** **nie ma w MSYS2** — budowane ze źródeł w workflow (`wget` → `./configure && make install` do `/mingw64`)
- **Linux:** `sudo apt install libltc-dev`

Plik `ui/src/CMakeLists.txt` ma już cross-platform detection przez `pkg-config` z fallback do `find_path`/`find_library`.

### 4. `git.exe failed with exit code 128` w "Post Checkout"

**Objaw:** W annotations na samym końcu workflow.

**Ignoruj to** — to post-cleanup step. Jeśli build przeszedł, artefakt jest OK.

---

## Biblioteki i ich źródła

| Biblioteka | Windows (MSYS2) | macOS (brew) | Linux (apt) |
|---|---|---|---|
| Qt 6.8.1 | `install-qt-action` | `install-qt-action` | `install-qt-action` |
| FFTW | `mingw-w64-x86_64-fftw` | `fftw` | `libfftw3-dev` |
| libsndfile | `mingw-w64-x86_64-libsndfile` | `libsndfile` | `libsndfile1-dev` |
| libmad | `mingw-w64-x86_64-libmad` | `mad` | `libmad0-dev` |
| libusb | `mingw-w64-x86_64-libusb` | (builtin) | `libusb-1.0-0-dev` |
| libftdi | (D2XX ze źródeł) | `libftdi` | `libftdi1-dev` |
| **libltc** | **ze źródeł** (brak pakietu) | `libltc` | `libltc-dev` |
| NSIS | `mingw-w64-x86_64-nsis` | — | — |

---

## Znane pułapki (nauki z prawdziwych buildów)

### Pułapka #1: GCC 15 vs upstream CI (GCC 11/12)

MSYS2 ma nowszy GCC (15.2) niż upstream QLC+ CI (`ubuntu-22.04` = GCC 11). **Więcej warningów** = więcej `-Werror` błędów.

**Rozwiązanie:** workflow wyłącza `-Werror` dla buildu Windows. Warningi dalej są widoczne w logach (`-Wall -Wextra`).

### Pułapka #2: Nowe biblioteki bez pakietów MSYS2

Jeśli dodajesz zależność przez `find_package`/`pkg_check_modules`:
1. Sprawdź https://packages.msys2.org/package/ czy jest pakiet `mingw-w64-x86_64-<nazwa>`
2. Jeśli **jest** — dodaj do listy `install:` w kroku MSYS2
3. Jeśli **nie ma** — dodaj krok buildu ze źródeł (jak dla libltc):
   ```yaml
   - name: Build and install libXXX from source
     shell: msys2 {0}
     run: |
       pacman -S --noconfirm autoconf automake libtool
       wget https://.../libXXX-x.y.z.tar.gz -O /tmp/lib.tar.gz
       cd /tmp && tar xzf lib.tar.gz && cd libXXX-x.y.z
       ./configure --prefix=/mingw64 && make -j$(nproc) && make install
   ```

### Pułapka #3: `.gitignore` blokuje pliki workflow

Reguła `build*` w `.gitignore` blokuje dodawanie nowych plików w `.github/workflows/build-*.yml`. Użyj:

```bash
git add -f .github/workflows/build-macos-v4.yml
```

### Pułapka #4: Kolejność inicjalizacji C++

Zawsze przy dodawaniu nowego pola członkowskiego do klasy:

```cpp
// foo.h
class Foo {
private:
    int m_a;           // linia 10
    int m_b;           // linia 11
    int m_c;           // linia 12   ← nowe pole
};

// foo.cpp
Foo::Foo()
    : m_a(0)           // ✅ taka sama kolejność
    , m_b(0)
    , m_c(0)           // ✅ nowe pole DOKŁADNIE na końcu
{}
```

---

## Szybkie naprawy gdy build się wywala

| Błąd | Plik do poprawy | Co zrobić |
|---|---|---|
| `-Werror=reorder` | `.cpp` z konstruktorem | Przenieś pole w liście init. zgodnie z kolejnością w `.h` |
| `-Werror=dangling-else` | wskazany plik | Dodaj `{}` wokół `if`/`else` |
| `libXXX not found` | workflow `yml` | Dodaj pakiet MSYS2/brew lub build ze źródeł |
| `git.exe failed (post)` | — | Ignoruj |
| Velleman plugin error | workflow | Już wyłączony (sed w "Fix build") |

---

## Struktura plików workflow

```
.github/
└── workflows/
    ├── build-windows-v4.yml    # Windows NSIS installer (.exe)
    └── build-macos-v4.yml      # macOS DMG installer (.dmg)
```

---

## Checklist przed pushem nowego feature branch

- [ ] Branch dodany do listy `branches:` w obu workflow
- [ ] Kolejność inicjalizacji w konstruktorach C++ zgodna z `.h`
- [ ] Nowe biblioteki mają źródło w workflow (MSYS2 pacman / brew / build ze źródeł)
- [ ] `git status` — żadnych zbędnych zmian (np. `build*`)
- [ ] Test lokalny (jeśli masz Windows/macOS pod ręką)
- [ ] `git push` → sprawdź https://github.com/filipolszewsk/qlcplus/actions

---

## Linki

- [Oficjalny workflow QLC+](https://github.com/mcallegari/qlcplus/blob/main/.github/workflows/build.yml)
- [Windows Build Wiki (Qt6 & CMake)](https://github.com/mcallegari/qlcplus/wiki/Windows-Build-(Qt6-&-cmake))
- [MSYS2 Packages](https://packages.msys2.org/package/)
- [jurplel/install-qt-action](https://github.com/jurplel/install-qt-action)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)

---

## Historia istotnych zmian w workflow

| Data | Zmiana | Powód |
|---|---|---|
| 2026-04-23 | Dodano build libltc ze źródeł | Brak pakietu w MSYS2, wymagane dla LTC Timecode |
| 2026-04-23 | Cross-platform `find_package` libltc | macOS wymaga brew, Linux apt |
| 2026-04-23 | Wyłączenie `-Werror` w Windows CI | GCC 15 w MSYS2 strikter niż upstream (GCC 11) |
| 2026-04-23 | Dodano `build-macos-v4.yml` | Potrzeba macOS DMG installera |
| 2026-04-23 | Naprawa kolejności init. w `doc.cpp` | `m_timeCodeSource` przed `m_monitorProps` |

---

**Ostatnia aktualizacja:** 2026-04-23  
**Wersja QLC+:** 4.14.4  
**Workflow:** `build-windows-v4.yml`, `build-macos-v4.yml`
