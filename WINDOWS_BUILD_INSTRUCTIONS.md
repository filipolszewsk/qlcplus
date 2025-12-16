# Instrukcje budowania Windows Installera dla QLC+ 4.14

## Przegląd

Ten dokument opisuje proces budowania Windows installera dla QLC+ 4.14 używając GitHub Actions. Workflow automatycznie buduje installer na Windows runnerze i udostępnia go jako artifact.

## Lokalizacja workflow

Workflow znajduje się w: `.github/workflows/build-windows-v4.yml`

## Jak uruchomić build

### Automatycznie
Workflow uruchamia się automatycznie przy:
- Push do gałęzi `main`, `master` lub `feature/windows-installer-workflow`
- Pull request do `main` lub `master`

### Ręcznie (workflow_dispatch)
1. Przejdź do: https://github.com/filipolszewsk/qlcplus/actions
2. Wybierz workflow "Build Windows Installer (QLC+ 4.14)"
3. Kliknij "Run workflow"
4. Wybierz gałąź i kliknij "Run workflow"

### Pobieranie artifactu
1. Po zakończeniu builda, przejdź do zakładki "Actions"
2. Kliknij na zakończony workflow run
3. Przewiń w dół do sekcji "Artifacts"
4. Pobierz plik `.exe` (nazwa: `QLC+-build-v4-{VERSION}-{DATE}-{GIT_REV}.exe`)

## Struktura workflow

### Główne kroki:
1. **Checkout code** - Pobiera kod z repozytorium
2. **Setup ccache** - Konfiguruje cache dla szybszych buildów
3. **Set environment variables** - Ustawia zmienne środowiskowe (QTDIR, BUILD_DATE, GIT_REV, etc.)
4. **Install Qt** - Instaluje Qt 6.8.1 używając `jurplel/install-qt-action`
5. **Install MSYS2** - Instaluje MSYS2 z wymaganymi pakietami (gcc, cmake, ninja, biblioteki)
6. **D2XX SDK** - Pobiera i kompiluje bibliotekę FTDI D2XX dla wsparcia DMX USB
7. **Fix build** - Modyfikuje pliki konfiguracyjne (CMakeLists.txt, NSIS script)
8. **Configure build** - Konfiguruje build używając CMake
9. **Build** - Kompiluje projekt używając Ninja
10. **Install** - Instaluje zbudowane pliki do `/c/qlcplus`
11. **windeployqt** - Kopiuje wymagane biblioteki Qt
12. **Build installer** - Tworzy installer używając NSIS
13. **Upload artifact** - Przesyła installer jako artifact

## Ważne zmienne środowiskowe

- `QT_VERSION`: "6.8.1"
- `QTDIR`: `/d/a/qlcplus/qlcplus/qt/Qt/6.8.1/mingw_64`
- `OUTFILE`: Nazwa pliku installera (np. `QLC+_4.14.4.exe`)
- `APPVERSION`: Wersja aplikacji (np. `4.14.4`)
- `NSIS_SCRIPT`: `qlcplus4Qt6.nsi`

## Wnioski i nauki z procesu

### 1. Używanie oficjalnego workflow jako wzorca
**Problem:** Początkowo próbowaliśmy uproszczonego workflow, który nie działał.

**Rozwiązanie:** Użyliśmy oficjalnego workflow QLC+ jako wzorca, co zapewniło kompatybilność i działanie wszystkich kroków.

**Wniosek:** Zawsze sprawdzaj oficjalne workflow/CI w repozytorium przed tworzeniem własnego.

### 2. Kolejność inicjalizacji zmiennych członkowskich w C++
**Problem:** Błąd kompilacji:
```
error: 'EFX::m_wings' will be initialized after [-Werror=reorder]
error:   'bool EFX::m_autoApplyOffsetTemplate' [-Werror=reorder]
```

**Przyczyna:** Kolejność inicjalizacji w konstruktorze nie odpowiadała kolejności deklaracji w klasie.

**Rozwiązanie:** Poprawiono kolejność inicjalizacji w `efx.cpp`:
```cpp
// Przed (błędne):
, m_fixtureGroupID(FixtureGroup::invalidId())
, m_offsetDirection(LeftToRight)
, m_offsetStep(90)
, m_wings(1)                    // ❌ Przed m_autoApplyOffsetTemplate
, m_autoApplyOffsetTemplate(false)
, m_offsetTemplateDirty(false)

// Po (poprawne):
, m_fixtureGroupID(FixtureGroup::invalidId())
, m_autoApplyOffsetTemplate(false)  // ✅ Zgodnie z kolejnością w klasie
, m_offsetTemplateDirty(false)
, m_offsetDirection(LeftToRight)
, m_offsetStep(90)
, m_wings(1)
```

**Wniosek:** Z flagą `-Werror=reorder` kompilator wymaga, aby kolejność inicjalizacji w konstruktorze odpowiadała kolejności deklaracji zmiennych członkowskich w klasie.

### 3. Używanie backticks vs $() w bash
**Różnica:** W oficjalnym workflow używane są backticks (`` ` ``) zamiast `$()` dla niektórych komend:
```bash
# Oficjalny workflow używa:
echo "BUILD_DATE=`date -u '+%Y%m%d'`" >> $GITHUB_ENV
echo "GIT_REV=`git rev-parse --short HEAD`" >> $GITHUB_ENV

# Zamiast:
echo "BUILD_DATE=$(date -u '+%Y%m%d')" >> $GITHUB_ENV
```

**Wniosek:** Oba podejścia działają, ale dla zgodności z oficjalnym workflow lepiej używać backticks.

### 4. Hardcoded ścieżki vs zmienne GitHub Actions
**Problem:** Próbowaliśmy użyć `${{ github.workspace }}` zamiast hardcoded ścieżek.

**Rozwiązanie:** Oficjalny workflow używa hardcoded ścieżek `/d/a/qlcplus/qlcplus/` które są standardowe dla GitHub Actions Windows runners.

**Wniosek:** W GitHub Actions Windows runners zawsze używają tych samych ścieżek, więc hardcoded ścieżki są bezpieczne i bardziej przewidywalne.

### 5. Podział zmiennych środowiskowych na osobne kroki
**Wzorzec:** Oficjalny workflow dzieli ustawianie zmiennych na osobne kroki:
- "Set ENV variables" - podstawowe zmienne
- "Set v4 ENV variables" - zmienne specyficzne dla wersji v4

**Wniosek:** To ułatwia debugowanie i utrzymanie workflow.

### 6. Instalacja Qt przez jurplel/install-qt-action
**Ważne:** Używamy `jurplel/install-qt-action@v4` zamiast instalacji Qt przez MSYS2, ponieważ:
- Zapewnia wszystkie wymagane narzędzia Qt (windeployqt, lrelease, etc.)
- Jest bardziej niezawodne i szybsze
- Automatycznie konfiguruje środowisko

**Konfiguracja:**
```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v4
  with:
    version: ${{ env.QT_VERSION }}
    host: 'windows'
    target: 'desktop'
    arch: 'win64_mingw'
    dir: "${{ github.workspace }}/qt/"
    modules: 'qt3d qtimageformats qtmultimedia qtserialport qtwebsockets'
```

### 7. Modyfikacje plików przed buildem
Workflow modyfikuje kilka plików przed buildem:
- `CMakeLists.txt`: Zmienia `Debug` na `Release`
- `plugins/CMakeLists.txt`: Wyłącza plugin Velleman
- `platforms/windows/CMakeLists.txt`: Poprawia ścieżki MSYS2
- `platforms/windows/qlcplus4Qt6.nsi`: Poprawia ścieżki projektu

**Wniosek:** Te modyfikacje są konieczne, ponieważ pliki są skonfigurowane dla lokalnego builda, a nie dla GitHub Actions.

### 8. Używanie MSYS2 shell
**Ważne:** Wszystkie kroki związane z buildem używają `shell: msys2 {0}`, co zapewnia:
- Dostęp do narzędzi MSYS2 (gcc, cmake, ninja)
- Poprawne ścieżki Unix-style (`/c/qlcplus` zamiast `C:\qlcplus`)
- Kompatybilność z narzędziami cross-compilation

## Troubleshooting

### Błąd: "git.exe failed with exit code 128"
**Przyczyna:** Problem z checkout lub submodułami.

**Rozwiązanie:** 
- Sprawdź czy workflow ma poprawne ustawienia checkout
- Upewnij się że `submodules: false` jest ustawione

### Błąd kompilacji: "will be initialized after [-Werror=reorder]"
**Przyczyna:** Nieprawidłowa kolejność inicjalizacji zmiennych członkowskich.

**Rozwiązanie:** 
1. Sprawdź kolejność deklaracji w pliku `.h`
2. Upewnij się że kolejność inicjalizacji w konstruktorze (`.cpp`) odpowiada kolejności deklaracji

### Błąd: "windeployqt: command not found"
**Przyczyna:** Qt nie został poprawnie zainstalowany lub QTDIR nie jest ustawione.

**Rozwiązanie:**
- Sprawdź czy krok "Install Qt" zakończył się sukcesem
- Sprawdź czy `QTDIR` jest poprawnie ustawione w zmiennych środowiskowych
- Upewnij się że używasz `jurplel/install-qt-action` zamiast instalacji przez MSYS2

### Błąd: "makensis: command not found"
**Przyczyna:** NSIS nie został zainstalowany.

**Rozwiązanie:**
- Sprawdź czy `mingw-w64-x86_64-nsis` jest w liście pakietów do instalacji w kroku MSYS2

### Build się nie uruchamia automatycznie
**Przyczyna:** Workflow może nie być wyzwalany przez push.

**Rozwiązanie:**
- Sprawdź sekcję `on:` w workflow
- Upewnij się że gałąź jest w liście `branches:`
- Użyj `workflow_dispatch` do ręcznego uruchomienia

## Struktura plików workflow

```
.github/
└── workflows/
    └── build-windows-v4.yml    # Workflow dla Windows installera
```

## Wymagane pakiety MSYS2

- `wget` - Pobieranie plików
- `unzip` - Rozpakowywanie archiwów
- `mingw-w64-x86_64-gcc` - Kompilator C/C++
- `mingw-w64-x86_64-gcc-libs` - Biblioteki GCC
- `mingw-w64-x86_64-cmake` - System buildowy
- `mingw-w64-x86_64-ninja` - Build system (szybszy niż make)
- `mingw-w64-x86_64-libmad` - Biblioteka audio
- `mingw-w64-x86_64-libsndfile` - Obsługa plików audio
- `mingw-w64-x86_64-flac` - Codec FLAC
- `mingw-w64-x86_64-fftw` - Biblioteka FFT
- `mingw-w64-x86_64-libusb` - Obsługa USB
- `mingw-w64-x86_64-python-lxml` - XML processing
- `mingw-w64-x86_64-nsis` - Tworzenie installera

## Wersje narzędzi

- **Qt:** 6.8.1
- **MSYS2:** Najnowsza wersja (release: true)
- **GCC:** Z pakietu mingw-w64-x86_64-gcc
- **CMake:** Z pakietu mingw-w64-x86_64-cmake
- **Ninja:** Z pakietu mingw-w64-x86_64-ninja

## Czas builda

Typowy czas builda: **~10-15 minut** (w zależności od obciążenia GitHub Actions)

## Retention artifacts

Artifacts są przechowywane przez **30 dni** (ustawione w workflow).

## Linki przydatne

- [Oficjalny workflow QLC+](https://github.com/mcallegari/qlcplus/blob/main/.github/workflows/build.yml)
- [Windows Build Wiki (Qt6 & CMake)](https://github.com/mcallegari/qlcplus/wiki/Windows-Build-(Qt6-&-cmake))
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [jurplel/install-qt-action](https://github.com/jurplel/install-qt-action)

## Notatki na przyszłość

1. **Zawsze sprawdzaj oficjalne workflow** przed tworzeniem własnego
2. **Testuj lokalnie** jeśli to możliwe przed pushowaniem
3. **Używaj ccache** dla szybszych buildów (już skonfigurowane)
4. **Sprawdzaj logi** w GitHub Actions jeśli build się nie powiedzie
5. **Kolejność inicjalizacji** w C++ musi odpowiadać kolejności deklaracji
6. **Hardcoded ścieżki** są OK w GitHub Actions (są standardowe)
7. **Backticks vs $()** - oba działają, ale backticks są używane w oficjalnym workflow
8. **MSYS2 shell** jest wymagany dla wszystkich kroków builda
9. **Qt przez install-qt-action** jest bardziej niezawodne niż przez MSYS2
10. **Workflow_dispatch** pozwala na ręczne uruchomienie builda

---

**Ostatnia aktualizacja:** 2025-12-16  
**Wersja QLC+:** 4.14.4  
**Workflow:** build-windows-v4.yml

