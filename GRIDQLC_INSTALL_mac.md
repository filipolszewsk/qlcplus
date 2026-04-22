# GRIDqlc v1 - Instalacja na macOS

## Wymagania

- macOS 12.0 (Monterey) lub nowszy
- Apple Silicon (M1/M2/M3/M4) lub Intel x86_64
- Bez Homebrew, bez Qt - wszystko jest wbudowane w DMG

---

## Instalacja (3 kroki)

### Krok 1: Otwórz DMG

Kliknij dwukrotnie na plik `GRIDqlc_*.dmg`. Otworzy się okno z ikoną QLC+.app oraz plikiem **Install.command**.

### Krok 2: Kliknij dwukrotnie Install.command

> To jest **zalecany sposób** - automatycznie kopiuje aplikację do `/Applications` i usuwa ostrzeżenia Gatekeepera.

- Kliknij dwukrotnie **Install.command**
- Jeśli pojawi się okienko "Are you sure you want to open it?" kliknij **Open**
- Otworzy się Terminal, który zainstaluje aplikację i uruchomi ją automatycznie
- Gotowe!

### Krok 3 (alternatywa): Przeciągnij ręcznie

Jeśli wolisz zainstalować ręcznie:

1. Przeciągnij `QLC+.app` do folderu `Applications`
2. Przy pierwszym uruchomieniu pojawi się okienko:  
   **"QLC+ cannot be opened because Apple cannot check it for malicious software"**
3. Idź do **System Settings → Privacy & Security** → przewiń w dół → kliknij **"Open Anyway"**
4. Potwierdź klikając **Open** w kolejnym okienku
5. Od tej chwili aplikacja uruchamia się normalnie

---

## Aktywacja licencji premium (GRIDqlc)

### Krok 1: Pobierz Hardware ID

Uruchom GRIDqlc i wejdź w **Help → Premium License**. Kliknij **Copy Hardware ID**.

### Krok 2: Aktywacja online

Wejdź na [gridqlc.com/activation](https://gridqlc.com/activation):

1. Wklej klucz licencyjny z emaila po zakupie
2. Wklej swój Hardware ID
3. Kliknij **Activate & Download license.qlckey**

### Krok 3: Instalacja pliku licencji

Przenieś plik `license.qlckey` do:

```
~/Library/Application Support/QLC+/license.qlckey
```

W terminalu:

```bash
mv ~/Downloads/license.qlckey ~/Library/Application\ Support/QLC+/license.qlckey
```

### Krok 4: Weryfikacja

Uruchom GRIDqlc ponownie. W **Help → Premium License** status powinien pokazywać **ACTIVE**.

---

## Instalacja skryptów premium

Umieść zaszyfrowane pliki `.qlcscript` w:

```
~/Library/Application Support/QLC+/RGBScripts/
```

GRIDqlc automatycznie je odczyta i odszyfruje przy starcie.

---

## Rozwiązywanie problemów

### "App is damaged and can't be opened"

Uruchom w Terminalu:

```bash
xattr -dr com.apple.quarantine /Applications/QLC+.app
```

### "Could not load the Qt platform plugin 'cocoa'"

Sprawdź czy aplikacja jest zainstalowana w `/Applications` (nie uruchamiana wprost z DMG):

```bash
ls /Applications/QLC+.app
```

### Aplikacja nie widzi MIDI / USB urządzeń

Sprawdź uprawnienia w **System Settings → Privacy & Security → Input Monitoring** - dodaj QLC+.

### Licencja nie jest wykrywana

```bash
ls ~/Library/Application\ Support/QLC+/license.qlckey
```

Plik musi być dokładnie w tej lokalizacji. Sprawdź czy nie jest pusty:

```bash
wc -c ~/Library/Application\ Support/QLC+/license.qlckey
```

---

## Informacje techniczne

| Parametr | Wartość |
|---|---|
| Architektura | arm64 (Apple Silicon native) |
| Minimalna wersja macOS | 12.0 (Monterey) |
| Qt | 6.x (wbudowane w bundle) |
| Podpis | Ad-hoc (bez notaryzacji) |
| Autor | Filip Olszewski |
| Strona | [gridqlc.com](https://gridqlc.com) |

---

**Autor:** Filip Olszewski  
**Strona:** [gridqlc.com](https://gridqlc.com)  
**Aktywacja:** [gridqlc.com/activation](https://gridqlc.com/activation)
