# Workflow pracy z forkiem QLC+ — przewodnik krok po kroku

> **Dla kogo:** Ty (Filip). Utrzymujesz własną wersję QLC+ we własnym forku, robisz swoje release'y, ale raz na jakiś czas chcesz wziąć łatki z oryginału.
>
> **Twoje cele:**
> 1. Wszystko trzymać u siebie (`filipolszewsk/qlcplus`)
> 2. Nic nie pushować do `mcallegari/qlcplus` (upstream)
> 3. Co jakiś czas wciągać patche z oryginału
> 4. Robić własne release'y (Windows `.exe` + macOS `.dmg`)
> 5. Innym dać możliwość robienia PR do Twojego forka

---

## 1. Na początek: zrozum strukturę

### Co to jest "remote"

**Remote** to zdalne repozytorium na GitHubie. Twój lokalny folder ma kilka "łącz" do różnych miejsc:

```
Lokalnie:  /Users/filipolszewski/Documents/qlc projekty/qlcplus
                │
                ├── fork     → git@github.com:filipolszewsk/qlcplus.git  (TWOJE REPO — tu pushujemy)
                ├── upstream → https://github.com/mcallegari/qlcplus.git (ORYGINAŁ — tylko czytamy)
                └── origin   → https://github.com/mcallegari/qlcplus.git (LEGACY — zignoruj)
```

**Zasada:** 
- `git push fork ...` = wysyła do Twojego repo
- `git fetch upstream` = pobiera zmiany z oryginału (bez wysyłania)
- `origin` ignoruj. Nigdy nie rób `git push origin`.

### Sprawdzenie czy remote jest dobrze ustawiony

```bash
git remote -v
```

Powinno pokazać:
```
fork      git@github-filip:filipolszewsk/qlcplus.git (fetch)
fork      git@github-filip:filipolszewsk/qlcplus.git (push)
upstream  https://github.com/mcallegari/qlcplus.git (fetch)
upstream  https://github.com/mcallegari/qlcplus.git (push)
origin    ...
```

---

## 2. Codzienna praca — dodanie nowej funkcji lub naprawa buga

### Krok 1: Zacznij od czystego `master`

```bash
git checkout master           # przełącz się na master
git pull                      # ściągnij najnowsze zmiany z Twojego forka
```

### Krok 2: Stwórz nowy branch

**Konwencja nazw:**
- `feature/nazwa-funkcji` — nowa funkcja (np. `feature/vcslider-animation`)
- `fix/nazwa-buga` — poprawka buga (np. `fix/crash-on-startup`)
- `docs/nazwa` — zmiany w dokumentacji
- `ci/nazwa` — zmiany w GitHub Actions

```bash
git checkout -b feature/moja-nowa-funkcja
```

### Krok 3: Pracuj, commituj

```bash
# edytujesz pliki...

git add <plik1> <plik2>        # dodaj konkretne pliki (lepsze niż git add .)
git commit -m "feat: krótki opis co zmieniasz"
```

**Konwencja commitów (Conventional Commits):**
- `feat:` — nowa funkcja
- `fix:` — naprawa buga
- `docs:` — dokumentacja
- `ci:` — GitHub Actions / build
- `refactor:` — refactor bez zmiany funkcjonalności
- `style:` — formatowanie
- `test:` — testy

Przykłady:
```bash
git commit -m "feat(vcslider): add custom color picker"
git commit -m "fix(efx): correct member initializer order"
git commit -m "ci: disable -Werror for Windows GCC 15"
```

### Krok 4: Push na fork

```bash
git push -u fork feature/moja-nowa-funkcja
# `-u` tylko przy pierwszym pushu — ustawia tracking
# kolejne pushe: po prostu `git push`
```

### Krok 5: Testuj (opcjonalnie buduj CI)

Jeśli branch jest na liście w `.github/workflows/build-windows-v4.yml` lub `build-macos-v4.yml` (`branches: [ main, master, ... ]`), buildy ruszą automatycznie.

Alternatywnie: **Actions → wybierz workflow → Run workflow → wybierz branch**.

### Krok 6: Merge do mastera

**Opcja A — z linii poleceń (szybciej, gdy pracujesz sam):**

```bash
git checkout master
git merge --no-ff feature/moja-nowa-funkcja -m "Merge feature: moja nowa funkcja"
git push
```

**`--no-ff`** (no fast-forward) tworzy commit merge, co ułatwia cofnięcie całej funkcji jedną zmianą.

**Opcja B — przez GitHub PR (gdy chcesz historyczny zapis / review):**

1. Wejdź: https://github.com/filipolszewsk/qlcplus
2. Kliknij "Compare & pull request" przy pushu
3. `base: master` ← `compare: feature/moja-nowa-funkcja`
4. Opisz, kliknij "Create pull request" → potem "Merge pull request"

### Krok 7: Sprzątanie

```bash
git branch -d feature/moja-nowa-funkcja              # usuń lokalnie
git push fork --delete feature/moja-nowa-funkcja     # usuń z forka
```

---

## 3. Łapanie łatek z oryginału (`mcallegari/qlcplus`)

### Scenariusz: raz w miesiącu chcesz mieć najnowsze fixy z upstreama

### Krok 1: Pobierz zmiany (bez mergowania)

```bash
git fetch upstream              # ściąga commity z oryginału, ale nic nie mergeuje
```

### Krok 2: Zobacz co jest nowego

```bash
git log HEAD..upstream/master --oneline        # co ma upstream czego Ty nie masz
git log upstream/master..HEAD --oneline        # co Ty masz czego nie ma upstream
```

### Krok 3: Dwie strategie wciągania zmian

#### Strategia A — **Merge wszystkiego** (proste, ale konflikty)

Kiedy stosować: upstream nie ma mnóstwa zmian, chcesz wszystko.

```bash
git checkout master
git merge upstream/master -m "Merge upstream changes"
# rozwiąż konflikty jeśli są
git push
```

#### Strategia B — **Cherry-pick konkretnych commitów** (bezpieczne)

Kiedy stosować: chcesz tylko konkretnego fixa, nie całych zmian.

```bash
git log upstream/master --oneline | head -20   # znajdź commit-hash
git checkout master
git cherry-pick <hash>                          # wciąga jeden commit
git push
```

Możesz też cherry-pick kilka naraz:
```bash
git cherry-pick <hash1> <hash2> <hash3>
```

Gdy jest konflikt: edytuj pliki, `git add <plik>`, `git cherry-pick --continue`.

#### Strategia C — **Rebase swoich feature'ów na upstream** (zaawansowane)

Kiedy stosować: rzadko, gdy chcesz mieć czyste drzewo jakby Twoje zmiany były od zawsze na upstream.

```bash
git checkout feature/moja-funkcja
git rebase upstream/master
# rozwiąż konflikty
git push -f fork feature/moja-funkcja          # UWAGA: force-push!
```

### ⚠️ Częste pułapki przy upstream

1. **Konflikty w CMakeLists.txt** — upstream może dodać te same pliki co Ty (np. `multiselectchannelcombo.cpp`). Akceptujesz obie wersje.
2. **Upstream usuwa plik który Ty dodałeś** — git zapyta czy zachować. Zachowaj Twoją wersję.
3. **`-Werror`** — upstream może dodać warningi których nie ma GCC 15. Twój workflow już to obchodzi, ale jeśli zmergujesz nowy `variables.cmake`, sprawdź czy sed w workflow nadal działa (`-Werror"` musi być w pliku).

### Jak NIE wysłać przypadkiem zmian do upstream

**Nigdy** nie używaj `git push upstream`. 
Nawet jakby się udało (nie masz praw), GitHub odrzuci z `403 Permission denied`.

Dla bezpieczeństwa możesz wyłączyć push do upstream lokalnie:
```bash
git remote set-url --push upstream "DISABLED"
```
Wtedy `git push upstream` da jasny błąd.

---

## 4. Robienie własnego release'u — AUTOMATYCZNIE

Masz workflow `.github/workflows/release.yml`, który robi WSZYSTKO sam: buduje Windows `.exe`, macOS `.dmg`, tworzy GitHub Release i dodaje pliki do pobrania.

### Metoda A — Ręcznie z GitHub UI (ZALECANE, najłatwiejsze)

1. Wejdź: https://github.com/filipolszewsk/qlcplus/actions/workflows/release.yml
2. Kliknij **Run workflow** (po prawej)
3. Wypełnij formularz:
   - **Tag:** `v4.14.4-filip.1` (lub inna wersja)
   - **Release name:** `QLC+ 4.14.4 (Filip Edition 1)` — opcjonalnie, puste = użyj tagu
   - **Draft:** zazwyczaj `true` (szkic do edycji przed publikacją)
   - **Prerelease:** `false` (lub `true` jeśli to wersja testowa)
4. Kliknij **Run workflow**
5. Czekaj ~15-20 min (build Windows + macOS + utworzenie release)
6. Wejdź: https://github.com/filipolszewsk/qlcplus/releases
7. Zobaczysz swój release z `.exe` i `.dmg` jako załącznikami

**Co się dzieje pod spodem:**
1. Jeśli tag nie istnieje — tworzy go na obecnym HEAD mastera
2. Buduje Windows (build-windows-v4.yml)
3. Buduje macOS (build-macos-v4.yml) równolegle
4. Pobiera oba artefakty
5. Tworzy GitHub Release z załącznikami i automatycznym changelogiem

### Metoda B — Przez push tagu (dla power userów)

```bash
git checkout master
git pull
git tag -a v4.14.4-filip.1 -m "Release opis zmian"
git push fork v4.14.4-filip.1
```

Reszta dzieje się sama. Po ~15-20 min release jest gotowy (automatycznie jako NIE-draft, ale możesz edytować).

### Konwencja nazw tagów

| Tag | Znaczenie |
|---|---|
| `v4.14.4-filip.1` | Twój build bazujący na upstream 4.14.4, iteracja 1 |
| `v4.14.4-filip.2` | Kolejna iteracja (bugfix) |
| `v4.14.5-filip.1` | Po wciągnięciu upstream 4.14.5 |
| `v4.14.4-beta.1` | Wersja testowa (ustaw `prerelease: true`) |

### Edycja release'u po utworzeniu

Jeśli chcesz poprawić opis / dodać screenshoty:

1. https://github.com/filipolszewsk/qlcplus/releases
2. Wybierz release
3. **Edit** (ikona ołówka)
4. Zmień co chcesz, **Update release**

### Jak inni pobierają Twój release

Podaj link:
```
https://github.com/filipolszewsk/qlcplus/releases/latest
```

Albo konkretny:
```
https://github.com/filipolszewsk/qlcplus/releases/tag/v4.14.4-filip.1
```

### Usuwanie release'u

1. https://github.com/filipolszewsk/qlcplus/releases → **Edit** → **Delete this release**
2. Usuń tag lokalnie i na forku:
   ```bash
   git tag -d v4.14.4-filip.1
   git push fork --delete v4.14.4-filip.1
   ```

### Pułapki

1. **Tag musi zaczynać się od `v`** — inaczej workflow się nie uruchomi
2. **Nie pushuj taga dwa razy** — GitHub da błąd; jeśli musisz, najpierw usuń stary
3. **Draft zawiera pliki** — nawet jako draft, release ma `.exe`/`.dmg` (ale niepubliczne dopóki nie klikniesz Publish)

---

## 5. Przyjmowanie Pull Requestów od innych

### Scenariusz: ktoś chce dodać do Twojego QLC+ funkcję

### Co robi osoba zewnętrzna (np. kolega)

1. Wchodzi na https://github.com/filipolszewsk/qlcplus
2. Klika **Fork** (tworzy swój fork Twojego forka)
3. Klonuje u siebie:
   ```bash
   git clone https://github.com/kolega/qlcplus.git
   ```
4. Dodaje Twój fork jako upstream:
   ```bash
   git remote add upstream https://github.com/filipolszewsk/qlcplus.git
   ```
5. Pracuje na swoim branchu, pushuje do swojego forka
6. Tworzy PR: `kolega/qlcplus:feature-x` → `filipolszewsk/qlcplus:master`

### Co robisz Ty

Dostajesz email/notyfikację na GitHubie. Potem:

#### A) Review kodu

1. Wejdź w PR: https://github.com/filipolszewsk/qlcplus/pulls
2. Zakładka **Files changed** → zobacz co zmienia
3. Komentuj przy linijkach jeśli chcesz poprawki
4. Jeśli OK → **Approve**

#### B) Testuj lokalnie (opcjonalnie)

```bash
git fetch fork pull/<NUMER-PR>/head:pr-<NUMER>
git checkout pr-<NUMER>
# buduj, testuj...
git checkout master
git branch -D pr-<NUMER>                  # sprzątanie
```

#### C) Merge

Na stronie PR kliknij **Merge pull request**. Trzy opcje:
- **Create a merge commit** — zachowuje wszystkie commity + merge commit (polecane)
- **Squash and merge** — łączy wszystkie commity w jeden (dobre dla małych zmian)
- **Rebase and merge** — dodaje commity jakby były Twoje (bez merge commita)

#### D) CI automatycznie zbuduje po merge'u

### Pierwszy raz? Dodaj sekcję "Contributing" w README

Możesz utworzyć plik `CONTRIBUTING.md` w repo z instrukcją dla kontrybutorów. Daj znać jeśli chcesz, zrobię szablon.

---

## 6. Uwaga o prywatności repo

### Obecny stan

Twój fork `filipolszewsk/qlcplus` jest **PUBLICZNY** (sprawdzone przez `gh repo view`).

### Jeśli chcesz go zmienić na prywatny

**Zalety prywatnego:**
- Nikt nie widzi kodu
- Możesz mieć konfiguracje/klucze bez obaw

**Wady prywatnego:**
- GitHub Actions limit: **2000 min/miesiąc** dla Free (vs. unlimited na public)
- **macOS kosztuje 10× więcej minut** (czyli ~200 min miesięcznie)
- Inni muszą dostać dostęp żeby widzieć/klonować

**Jak zmienić:**
1. https://github.com/filipolszewsk/qlcplus/settings
2. Scroll na dół → **Danger Zone**
3. **Change repository visibility** → Private

### Kompromis

Jeśli chcesz CI za darmo ale nie chcesz publikować kodu — **zostaw public, ale nie publikuj rzeczy sekretnych (kluczy, haseł)**.

---

## 7. Ściąga — najczęstsze komendy

### Codziennie

```bash
git status                             # co zmieniłeś
git diff                               # szczegóły zmian
git add <plik>                         # dodaj plik do commita
git commit -m "fix: coś"               # zapisz zmianę
git push                               # wyślij na fork
```

### Zarządzanie branchami

```bash
git checkout -b feature/x              # nowy branch
git checkout master                    # przełącz na master
git branch                             # lista lokalnych branchy
git branch -d feature/x                # usuń lokalnie
git push fork --delete feature/x       # usuń z forka
```

### Synchronizacja

```bash
git fetch --all                        # ściągnij ze wszystkich remote'ów
git pull                               # ściągnij + zmerguj bieżący branch
git fetch upstream                     # tylko z oryginału
```

### Wciąganie z upstream

```bash
git fetch upstream
git checkout master
git merge upstream/master              # wszystko
# LUB
git cherry-pick <hash>                 # pojedynczy commit
git push
```

### Release

```bash
git tag -a v4.14.4-filip.1 -m "..."
git push fork v4.14.4-filip.1
```

### Cofanie zmian

```bash
git restore <plik>                     # cofnij niezcommitowane zmiany
git reset HEAD~1                       # cofnij ostatni commit (zachowując zmiany)
git reset --hard HEAD~1                # cofnij ostatni commit (wyrzuć zmiany)
git revert <hash>                      # zrób commit cofający inny commit
```

### CI / Actions

```bash
# Status ostatnich runów
gh run list --repo filipolszewsk/qlcplus --limit 5

# Szczegóły konkretnego runa
gh run view <id> --repo filipolszewsk/qlcplus

# Anuluj run
gh run cancel <id> --repo filipolszewsk/qlcplus

# Pobierz artefakt
gh run download <id> --repo filipolszewsk/qlcplus
```

---

## 8. Najczęstsze problemy i ich rozwiązania

### "Permission denied" przy push

**Przyczyna:** Próbujesz pushować do `upstream` / `origin` (które wskazują na mcallegari).

**Fix:** 
```bash
git push fork <branch>                 # eksplicytnie na fork
```

### "non-fast-forward" przy push

**Przyczyna:** Ktoś (lub Ty z innego kompa) pushnął coś wcześniej.

**Fix:**
```bash
git pull --rebase                      # wciągnij i ustaw swoje commity na górze
git push
```

### Merge conflict

**Przyczyna:** Twoje zmiany kłócą się ze zmianami z innego brancha.

**Fix:**
1. Otwórz plik w edytorze
2. Znajdź `<<<<<<< HEAD`, `=======`, `>>>>>>>` — wybierz co zachować
3. Usuń markery
4. `git add <plik>` → `git commit` (lub `git cherry-pick --continue`)

### CI nie startuje po pushu

**Przyczyna:** Branch nie jest na liście `branches:` w workflow.

**Fix:** Dodaj branch do `.github/workflows/build-*.yml`:
```yaml
on:
  push:
    branches: [ main, master, <twój-branch> ]
```

### macOS build w queued wiecznie

Patrz sekcja w `WINDOWS_BUILD_INSTRUCTIONS.md` → "Znane pułapki" → pułapka #1/#5.

---

## 9. Kiedy pytać o pomoc

Jeśli:
- Nie jesteś pewny co zrobić → **zrób `git status` i zapytaj** zanim wpiszesz coś destrukcyjnego
- Ktoś zrobił PR którego nie rozumiesz → **poproś o opis/video**
- Konflikt jest skomplikowany → **zrób screenshot `git status` i zapytaj**
- Build pada → skopiuj **ostatnie ~50 linii loga** z Actions i wklej

**Nigdy nie pushuj w panice z `--force`**. Najpierw pytaj.

---

## 10. Przydatne linki

- Twój fork: https://github.com/filipolszewsk/qlcplus
- Twoje Actions: https://github.com/filipolszewsk/qlcplus/actions
- Twoje PRs: https://github.com/filipolszewsk/qlcplus/pulls
- Twoje Releases: https://github.com/filipolszewsk/qlcplus/releases
- Upstream: https://github.com/mcallegari/qlcplus
- Changelog upstream: https://github.com/mcallegari/qlcplus/blob/master/ChangeLog
- Dokumentacja CI: [WINDOWS_BUILD_INSTRUCTIONS.md](./WINDOWS_BUILD_INSTRUCTIONS.md)

---

**Ostatnia aktualizacja:** 2026-04-23  
**Autor:** Filip + asystent AI
