# QLC+ Fork - Szczegółowy Changelog

## 📋 Podsumowanie branchy i commitów

---

## 🔧 BUGFIXY

### `bugfix/dmx-dump-universe-overflow`
- **fix: resize DMX dump buffer to actual universes** - Naprawiono przepełnienie bufora DMX dump

### `bugfix/fixturegroup-autosave-crash`
- **fix: add NULL checks in Doc::saveXML to prevent crash during autosave** - Dodano sprawdzanie NULL w Doc::saveXML, aby zapobiec crashowi podczas autozapisu

### `bugfix/xypad-efx-zero-range`
- **Fix EFX preset launch with zero width/height range** - Naprawiono uruchamianie presetów EFX z zerowym zakresem szerokości/wysokości

### `fix/fixturegroup-savexml-race-condition`
- **Add mutex locks to prevent race conditions in FixtureGroup** - Dodano blokady mutex, aby zapobiec race condition w FixtureGroup

### `fix/vcanimation-parameter-sync`
- **Fix: Synchronize VC Animation parameter controls when function starts** - Synchronizacja kontrolek parametrów VC Animation przy starcie funkcji
- **Fix: Make VC Animation knob controls override function properties** - Kontrolki knob nadpisują właściwości funkcji

### `fix/vcxypad-efx-background-running`
- **Fix: VCXYPad EFX/Scene running in background after mode change** - Naprawiono działanie EFX/Scene w tle po zmianie trybu

### `fix/vcxypad-efx-qpointer-crash`
- **Fix VCXYPad crash: Use QPointer for EFX and Scene to prevent use-after-free** - Naprawiono crash VCXYPad używając QPointer
- **Add crash analysis: VCXYPad EFX race condition with deleted object** - Dodano analizę crasha

### `fix-center-to-sides-offset`
- **Fix EFX center-to-sides alignment** - Naprawiono wyrównanie center-to-sides w EFX

### `fix-column-mode-template`
- **Fix fixture group column template handling** - Naprawiono obsługę szablonów kolumn grup urządzeń
- **Apply stored column modes to fixture instances** - Zastosowanie zapisanych trybów kolumn do instancji urządzeń
- **Respect stored column mode when rebuilding EFX editor** - Respektowanie zapisanego trybu kolumny przy przebudowie edytora EFX

### `fix_rgbmatrix_sync_on_speed_change`
- **Fix: Eliminate cumulative rounding errors in RGBMatrix phase scaling** - Eliminacja kumulacyjnych błędów zaokrągleń w skalowaniu fazy RGBMatrix
- **Fix: Preserve phase when changing RGBMatrix speed in runtime** - Zachowanie fazy przy zmianie prędkości RGBMatrix w runtime

### `fix/animationknob-midi-scaling` (aktualny branch)
- **Fix: Scale MIDI/DMX input to AnimationKnob range in VCMatrix** - Skalowanie wejść MIDI/DMX do zakresu AnimationKnob w VCMatrix
- **Fix: Revert MIDI scaling, add ResetParamsOnOperate option to VCMatrix** - Cofnięto skalowanie MIDI; dodano opcję ResetParamsOnOperate do VCMatrix

### `fix/mastertimer-universe-threading-crash`
- **Fix threading race conditions causing SIGSEGV at 0x10 on file reload** - Naprawiono wyścigi wątków powodujące crash SIGSEGV przy przeładowaniu pliku

### `fix/soloframe-page-aware-stopping`
- **VCSoloFrame: do not stop functions on hidden/disabled pages** - VCSoloFrame nie zatrzymuje funkcji na ukrytych/wyłączonych stronach

### `fix/crossfade-initial-start`
- **VCCueList: crossfade start fix + Record/Overwrite/Delete external input fix** - Naprawiono start crossfade oraz obsługę zewnętrznych wejść Record/Overwrite/Delete

### `fix/cuelist-crossfade-reapply`
- **VCCueList: reapply step values in virtual crossfade for LTP priority** - Ponowne aplikowanie wartości kroków w wirtualnym crossfade dla priorytetu LTP
- **VCCueList: call adjustStepIntensity in virtual crossfade case** - Wywołanie adjustStepIntensity w przypadku wirtualnego crossfade

### `fix/vcbutton-monitoring-stability`
- **VCButton: stabilize channel monitoring with function ownership tracking** - Stabilizacja monitorowania kanałów VCButton ze śledzeniem własności funkcji

---

## ✨ NOWE FUNKCJE

### `feature/dimmerwave-efx`
Nowy algorytm EFX inspirowany GrandMA3:
- **Add DimmerWave EFX algorithm with GrandMA3-inspired parameters** - Dodano algorytm DimmerWave z parametrami inspirowanymi GrandMA3
- **Fix: DimmerWave output range from 0-1 to -1..1 for proper scaling** - Naprawiono zakres wyjściowy DimmerWave
- **Fix: DimmerWave algorithm validation in setAlgorithm** - Walidacja algorytmu DimmerWave
- **Fix: Add applyWaveShape method declaration to efx.h** - Dodano deklarację metody applyWaveShape
- **Fix UI: Move Color Background checkbox to row 17 to prevent overlap** - Poprawki UI

### `feature/dimmerwave-wavelength`
- **Add WaveLength parameter to DimmerWave EFX** - Dodano parametr WaveLength do DimmerWave EFX

### `feature/crossfade-to-selected-cue`
- **Add manual secondary selection control in Crossfade mode** - Dodano manualną kontrolę wtórnego wyboru w trybie Crossfade
- **Fix bidirectional crossfade slider behavior** - Naprawiono zachowanie dwukierunkowego slidera crossfade

### `feature/delete-cue-button`
- **Add Delete button to VC Cue List Recording tab** - Dodano przycisk Delete do zakładki nagrywania VC Cue List

### `feature/dmx-monitor-grid-view`
- **Change monitor level bar color from green to white** - Zmieniono kolor paska poziomu monitora z zielonego na biały

### `feature/efx-auto-rebuild`
- **Finalize fixture group auto rebuild** - Finalizacja automatycznej przebudowy grup urządzeń
- **Auto-rebuild EFX fixtures on group updates** - Automatyczna przebudowa urządzeń EFX przy aktualizacji grupy
- **Refresh EFX editor on fixture group updates** - Odświeżanie edytora EFX przy aktualizacjach grupy urządzeń

### `feature/efx-copy-paste-settings`
- **Add Copy/Paste buttons for EFX offsets and movement settings** - Dodano przyciski Kopiuj/Wklej dla offsetów EFX i ustawień ruchu
- **Fix DimmerWave 360° case to use normal fade logic instead of constant value** - Naprawiono przypadek DimmerWave 360°

### `feature/efx-group-refresh`
- **Persist column template offsets for EFX fixture groups** - Zachowanie offsetów szablonu kolumn dla grup urządzeń EFX
- **Persist reverse state for EFX group columns** - Zachowanie stanu reverse dla kolumn grup EFX

### `feature/efx-offset-helper`
Rozbudowany system offsetów dla EFX:
- **Add offset direction and step UI controls with 6 propagation patterns** - Dodano kontrolki UI kierunku offsetu i kroku z 6 wzorcami propagacji
- **Add offset direction and step controls to EFX backend** - Dodano kontrolki kierunku offsetu i kroku do backendu EFX
- **Add Wings mode and fix Symmetric for even-width grids** - Dodano tryb Wings i naprawiono Symmetric dla siatek o parzystej szerokości
- **Change Wings from mode to numeric parameter (1-10 blocks) like GrandMA** - Zmieniono Wings z trybu na parametr numeryczny (1-10 bloków) jak w GrandMA

### `feature/efx-spatial-fixturegroup`
Tryb Fixture Group dla EFX:
- **Add Fixture Group Mode UI controls to EFX editor layout** - Dodano kontrolki UI trybu Fixture Group do layoutu edytora EFX
- **Show all columns including empty ones with template offset values** - Pokazywanie wszystkich kolumn z wartościami offsetu szablonu
- **Add empty grid checks to prevent issues with 0-sized groups** - Dodano sprawdzanie pustych siatek
- **Add column index bounds validation in 3 slot methods** - Walidacja granic indeksu kolumny
- **Disable Add/Remove/Raise/Lower buttons in group mode to prevent crashes** - Wyłączenie przycisków w trybie grupy
- **Fix memory leak: delete EFXFixture if addFixture fails** - Naprawiono wyciek pamięci
- **Add ComboBox data validation to prevent invalid group ID usage** - Walidacja danych ComboBox
- **Add FixtureGroup deletion handler to prevent crashes when group is removed** - Handler usuwania FixtureGroup
- **Fix division by zero crash in EFX group mode columnOffset calculation** - Naprawiono dzielenie przez zero
- **EFX: Show all modes (PanTilt, Dimmer, RGB) for empty columns** - Pokazywanie wszystkich trybów dla pustych kolumn
- **EFX: Always show all modes for ALL columns (empty and non-empty)** - Zawsze pokazuj wszystkie tryby
- **Fix: Use correct mode name 'Position' instead of 'PanTilt'** - Użycie poprawnej nazwy trybu
- **Add row selection filter - choose which rows from grid are affected by EFX** - Dodano filtr wyboru wierszy
- **Fix row selection crash: prevent recursion and properly handle widget cleanup** - Naprawiono crash wyboru wierszy
- **Preserve fixture Mode and Direction settings when changing row selection** - Zachowanie ustawień Mode i Direction przy zmianie wyboru wierszy
- **Fix: Restore column Mode from backend for empty columns after project load** - Przywracanie Mode kolumny z backendu

### `feature/fixture-address-remap-vc-inputs`
- **feat: Auto-remap VC external inputs when changing fixture DMX address** - Automatyczne przemapowanie wejść zewnętrznych VC przy zmianie adresu DMX urządzenia

### `feature/fixture-group-grid-multiselect`
- **Fixture Group Editor: Add multi-selection and group drag & drop** - Dodano wielokrotny wybór i grupowe drag & drop
- **Fixture Group Editor: Use arrow keys for multi-selection movement** - Użycie klawiszy strzałek do przesuwania wielokrotnego wyboru

### `feature/fixture-group-grid-multiselect-dragdrop`
- **Fixture Group Editor: Add mouse drag & drop and preserve selection** - Dodano przeciąganie myszą z zachowaniem selekcji
- **Fixture Group Editor: Fix mouse drag - install eventFilter on viewport** - Naprawiono przeciąganie myszą
- **Fixture Group Editor: Live preview drag & drop, single fixture support** - Podgląd na żywo drag & drop
- **Fixture Group Editor: Fix swap logic to preserve position correspondence** - Naprawiono logikę zamiany
- **Fixture Group Editor: Fix swap logic to never delete fixtures** - Naprawiono logikę zamiany - nigdy nie usuwa urządzeń
- **Fixture Group Editor: Preserve column/row when swapping fixtures** - Zachowanie kolumny/wiersza przy zamianie urządzeń

### `feature/flash-shift-parameter-matrix`
- **Add preset editing and improve animation preset behavior** - Dodano edycję presetów i poprawiono zachowanie presetów animacji

### `feature/multipatch-slider-output-channel`
- **Add output channel assignment for Level sliders in Multi-Patch editor** - Dodano przypisanie kanału wyjściowego dla sliderów Level w edytorze Multi-Patch

### `feature/reverse-pan-tilt`
- **Add reverse pan/tilt functionality to channel modifiers** - Dodano funkcję odwróconego pan/tilt do modyfikatorów kanałów

### `feature/rgb-matrix-improvements`
Ulepszenia RGB Matrix:
- **Add ControlModeDimmerFullRange to RGB Matrix** - Dodano tryb ControlModeDimmerFullRange
- **Add collapsible and scrollable UI for multi-channel mapping** - Dodano zwijane i przewijalne UI dla mapowania wielu kanałów
- **Implement multi-channel mapping per fixture type** - Implementacja mapowania wielu kanałów per typ urządzenia

### `feature/rgb-matrix-script-knobs`
- **Feature: Add knob control for RGB Matrix script properties** - Dodano kontrolę knob dla właściwości skryptów RGB Matrix
- **Feature: Add Soft Patch to VCMatrix and Animation Knobs** - Dodano Soft Patch do VCMatrix i Animation Knobs

### `feature/rgb-script-parameter`
Zaawansowane parametry skryptów RGB:
- **Implement scriptHeight property for flexible row count** - Implementacja właściwości scriptHeight dla elastycznej liczby wierszy
- **Add scriptHeight() support to both RGBScript and RGBScriptV4** - Dodano wsparcie scriptHeight()
- **UI: Dynamic value index count based on scriptHeight()** - Dynamiczna liczba indeksów wartości
- **UI: Dynamic scriptHeight refresh on pattern change + default Row 0** - Dynamiczne odświeżanie scriptHeight

### `feature/soft-patch-and-animation-knobs`
- **Feature: Add Soft Patch to VCMatrix and Animation Knobs** - Dodano Soft Patch do VCMatrix i Animation Knobs
- **Feature: Add knob control for RGB Matrix script properties** - Dodano kontrolę knob dla właściwości skryptów RGB Matrix
- **feat: Add multi-patch support for VCMatrix** - Dodano wsparcie multi-patch dla VCMatrix

### `feature/step-index-output-channel`
- **Add step index output feature to VCCueList** - Dodano funkcję wyjścia indeksu kroku do VCCueList
- **Improve step index output UI with fixture/channel tree** - Poprawiono UI wyjścia indeksu kroku z drzewem urządzeń/kanałów
- **Use separate dialog for step index output channel selection** - Użycie oddzielnego dialogu dla wyboru kanału wyjścia indeksu kroku

### `feature/step-index-output-with-installer`
- Wszystko z `feature/step-index-output-channel` plus:
- **Add comprehensive Windows build instructions and lessons learned** - Dodano instrukcje budowania Windows
- **Add Windows installer workflow for QLC+ 4.14** - Dodano workflow instalatora Windows

### `feature/submaster-ltp-canfade`
- **Add Intensity flag to forced HTP channels for submaster support** - Dodano flagę Intensity dla wymuszonych kanałów HTP dla wsparcia submaster
- **Fix RGB to grey conversion precision and grayscale handling** - Naprawiono precyzję konwersji RGB na szarość

### `feature/vc-cue-list-load`
- Funkcje ładowania Cue List

### `feature/vc-cue-list-load-and-release`
- Funkcje ładowania i zwalniania Cue List

### `feature/vc-cue-list-overwrite`
- **Add overwrite feature to VCCueList widget** - Dodano funkcję nadpisywania do widgetu VCCueList

### `feature/vc-cue-list-recorder`
- **Add record feature to VCCueList widget with robustness improvements** - Dodano funkcję nagrywania do widgetu VCCueList

### `feature/vc-slider-dual-input`
- Podwójne wejście dla VC Slider

### `feature/vc-slider-one-shot`
Tryb One-Shot dla VC Slider:
- **Add One-Shot mode for VC Slider** - Dodano tryb One-Shot dla VC Slider
- **Fix One-Shot mode: slider stays in position, sends value only once** - Slider pozostaje w pozycji, wysyła wartość tylko raz
- **Fix One-Shot mode: send values during drag when position changes** - Wysyłanie wartości podczas przeciągania
- **Fix One-Shot mode: compare with actual channel value to prevent continuous sending** - Porównanie z faktyczną wartością kanału
- **Fix One-Shot mode: use universe->preGMValue() instead of fc->current()** - Użycie preGMValue()
- **Fix One-Shot mode: remove FadeChannel when value unchanged to prevent continuous sending** - Usunięcie FadeChannel gdy wartość niezmieniona

### `feature/vcmatrix-preset-exclude-properties`
- Właściwości wykluczenia presetów VCMatrix

### `feature/vcmatrix-softpatch`
- **Feature: Add knob control for RGB Matrix script properties** - Kontrola knob dla właściwości skryptów RGB Matrix

### `feature/vcmultipatch-fixes`
- **Fix(VC): Naprawiono błąd w edytorze multi-patch, który uniemożliwiał przypisanie wielu kanałów jednocześnie**
- **feat(VC): Add incremental option to multi-patch editor** - Dodano opcję inkrementalną
- **feat(VC): Improve multi-patch editor auto-increment logic** - Poprawiono logikę auto-incrementu
- **Fix compilation issues in VCMultiPatchEditor and add softpatch functionality** - Naprawiono problemy kompilacji

### `feature/windows-installer-workflow`
- **Add Windows installer workflow for QLC+ 4.14** - Dodano workflow instalatora Windows dla QLC+ 4.14
- **Fix Windows installer workflow - add QTDIR export and NSIS script path handling** - Naprawiono workflow instalatora
- **Add comprehensive Windows build instructions and lessons learned** - Dodano instrukcje budowania

### `feature/license-protection`
System licencji premium GRIDqlc:
- **Add GRIDqlc premium license system with RGB script encryption** - Dodano system licencji premium z szyfrowaniem skryptów RGB
- **Rename license file from .qlckey to license.qlckey and polish license UI** - Zmieniono nazwę pliku licencji; poprawki UI
- **Add EFX JSON settings copy/paste, VCCueList rename action, and hide buttons option** - Kopiowanie ustawień EFX przez JSON, zmiana nazwy kroku CueList, opcja ukrywania przycisków
- **Update macOS installation guide for GRIDqlc fork** - Zaktualizowano instrukcję instalacji macOS

### `feature/rgbw-engine-support`
Wsparcie silnika RGBW w RGB Matrix:
- **Add ControlModeRgbw to RGB Matrix engine for RGBW fixture support** - Dodano tryb ControlModeRgbw do silnika RGB Matrix
- **Fix RGBW preview: blend W channel additively into RGB display color** - Naprawiono podgląd RGBW: kanał W blendowany addytywnie
- **Improve RGBW preview: normalize instead of clamp for perceptual accuracy** - Normalizacja zamiast clamp dla perceptualnej dokładności podglądu
- **VCCueList: column settings copy and channel assignment** - Kopiowanie ustawień kolumn i zmiana kanału kolumny w VCCueList

### `feature/rgbmatrix-control-mode-none`
- **Add ControlModeNone to RGBMatrix for disabling channel output** - Dodano tryb ControlModeNone wyłączający wyjście kanałów RGB Matrix

### `feature/rgbmatrix-copy-paste`
- **RGBMatrixEditor: add copy/paste for settings with selection dialog** - Dodano kopiowanie/wklejanie ustawień RGB Matrix z dialogiem wyboru
- **Add batch paste of RGB Matrix settings via Ctrl+V in Function Manager** - Wsadowe wklejanie ustawień RGB Matrix przez Ctrl+V w Function Manager

### `feature/efx-copy-paste-dialog`
- **Add batch paste of RGB Matrix settings via Ctrl+V in Function Manager** - Wsadowe wklejanie ustawień przez dialog (wersja CI z workflow)

### `feature/vc-frame-scale-contents`
- **VCFrame: add persistent scale-contents feature with font scaling** - Dodano trwałe skalowanie zawartości VCFrame ze skalowaniem czcionek

### `feature/vccuelist-channel-columns`
Kolumny wartości kanałów w VCCueList:
- **Add channel value columns to VCCueList widget** - Dodano kolumny z wartościami kanałów do widgetu VCCueList
- **Add channel column editor dialog** - Dodano dialog edytora kolumn kanałów
- **Add double-click header rename for channel columns** - Zmiana nazwy kolumny przez dwuklik w nagłówku
- **Add hide option for channel columns** - Opcja ukrywania kolumn kanałów
- **Add scaled display mode and scene change detection** - Skalowany tryb wyświetlania i wykrywanie zmiany sceny

### `feature/vcframe-detach-window`
Odłączanie VCFrame do osobnego okna:
- **Add detach button to VCFrame for separating frames into floating windows** - Dodano przycisk odłączania VCFrame do pływającego okna
- **Fix VCFrame reattachment and add persistence for detached state** - Naprawiono ponowne dołączanie; persystencja stanu odłączenia
- **Fix save/load of detached VCFrame windows** - Naprawiono zapis/odczyt stanu odłączonych okien

### `feature/universe-grid-view`
Widok siatki universum z drag & drop:
- **Add Universe Grid View with drag-drop address changing and UX improvements** - Dodano widok siatki universum ze zmianą adresów przez drag & drop
- **Fix right panel width stability using QStackedWidget** - Stabilizacja szerokości prawego panelu
- **Fix drag-drop address change for all universes and add VC input remap** - Naprawiono zmianę adresów drag & drop dla wszystkich universów

### `feature/vc_input_remap`
- **Feature: Automatically remap VC input sources when fixture address changes** - Automatyczne przemapowanie wejść VC przy zmianie adresu urządzenia (standalone branch)

### `feature/vccuelist-autostart-operate`
- **Enable VCCueList overwrite during active playback** - Umożliwiono nadpisywanie VCCueList podczas aktywnego odtwarzania

### `feature/vcbutton-channel-monitoring`
- **VCButton: optional Monitor Channel Values and deactivate when overridden** - Opcjonalne monitorowanie wartości kanałów; dezaktywacja gdy kanał nadpisany

### `feature/per-page-frame-size`
- **VCFrame: wire per-page size checkbox in properties dialog** - Konfiguracja rozmiaru VCFrame per strona w dialogu właściwości
- **VCButton: only setState(Inactive) on channel override, don't stop scene** - VCButton ustawia tylko stan Inactive przy nadpisaniu kanału, nie zatrzymuje sceny

### `feature/vcframe-pages-hide-header`
- **WIP: hide header option for VCFrame pages** - (w trakcie) Opcja ukrywania nagłówka stron VCFrame

### `feature/scribble-enhancements`
- **VCButton: only setState(Inactive) on channel override, don't stop scene** - Ulepszenia zachowania VCButton przy nadpisaniu kanału

### `feature/import-functions-dialog`
- **Fix Fixture Mapping dialog UI: column layout and widget overlap** - Naprawiono UI dialogu mapowania urządzeń: układ kolumn i nakładanie widgetów

### `feature/universe-address-editor`
- **feat: Auto-remap VC external inputs when changing fixture DMX address** - Automatyczne przemapowanie wejść zewnętrznych VC przy zmianie adresu DMX

---

## 🔬 BRANCHY EKSPERYMENTALNE

### `multi_value_matrix`
System wielowartościowej matrycy:
- **Implement multi-value matrix with per-fixture value index mapping** - Implementacja matrycy wielowartościowej z mapowaniem indeksu wartości per urządzenie
- **Fix: Allow fixtures to read from any row in script output, not just physical position** - Urządzenia mogą czytać z dowolnego wiersza
- **Fix: Always use valueIndex as source row** - Zawsze używaj valueIndex jako wiersza źródłowego
- **Fix: Use fixed 32 rows in value index dropdown instead of physical group height** - Użycie stałych 32 wierszy

### `per_definition_channel_mapping`
- **Implement per-fixture-definition channel mapping for RGB Matrix** - Mapowanie kanałów per definicja urządzenia dla RGB Matrix
- **Fix: Move per-fixture channel mapping widget to scrollArea** - Przeniesienie widgetu do scrollArea
- **Fix: Add forward declarations for QLCFixtureDef and QLCFixtureMode** - Dodano forward declarations

### `fixture_range`
Zakresy Pan/Tilt:
- **Add Pan/Tilt range configuration with absolute ranges (0-540°)** - Dodano konfigurację zakresów Pan/Tilt z zakresami absolutnymi
- **Fix Pan/Tilt range scaling in EFX relative mode** - Naprawiono skalowanie zakresu Pan/Tilt w trybie relative EFX
- **Fix crash when loading project with Pan/Tilt ranges** - Naprawiono crash przy ładowaniu projektu z zakresami
- **Refactor: Move Pan/Tilt scaling to post-modifier stage** - Przeniesienie skalowania Pan/Tilt do etapu post-modifier

### `vcxypad-preset-softpatch`
- **Add Soft Patch support for XY Pad Presets** - Dodano wsparcie Soft Patch dla presetów XY Pad
- **Enable multi-selection for EFX and Scene presets in XY Pad** - Wielokrotny wybór dla presetów EFX i Scene

### `multi_value_thru_multiply`
Alternatywne podejście do Multi-Value Mapping:
- **Implement multiply-height approach for Multi-Value Mapping** - Implementacja podejścia multiply-height dla mapowania wielu wartości
- **Add Multi-Value Mapping toggle and Row Filter for RGB Matrix** - Przełącznik Multi-Value Mapping i filtr wierszy dla RGB Matrix

### `multipatch-level-parameter`
- **Add LEVEL parameter to multipatch editor for slider widgets** - Dodano parametr LEVEL do edytora multipatch dla sliderów
- **Fix LEVEL parameter conversion between Fixture and display formats** - Naprawiono konwersję parametru LEVEL między formatami Fixture a wyświetlania
- **Fix LEVEL parameter handling in multipatch editor** - Naprawiono obsługę parametru LEVEL

### `rgbscprit_widget_exlude_and_edit`
Edycja i wykluczanie presetów w widgecie RGB Script:
- **Add preset editing and improve animation preset behavior** - Dodano edycję presetów i poprawiono zachowanie presetów animacji
- **Fix: Make VC Animation knob controls override function properties** - Kontrolki knob nadpisują właściwości funkcji animacji
- **Fix: Synchronize VC Animation parameter controls when function starts** - Synchronizacja kontrolek parametrów przy starcie funkcji

### `optimize-parameter-matrix-reduced-channels`
Optymalizacja logiki flash/restore w Parameter Matrix:
- **Handle flash press during restore before first-press logic** - Obsługa ponownego wciśnięcia flash podczas restore
- **Fix: restore ALL params from backup, not just changed ones** - Przywracanie wszystkich parametrów z backupu
- **Fix flash re-press during restore: restore to backup before starting new flash** - Naprawiono ponowny flash podczas restore
- **Remove problematic backup recalculation on Release during restore** - Usunięto problematyczne przeliczanie backupu przy Release

### `optimize-parameter-matrix-staging-buffer`
- Optymalizacja bufora staging w Parameter Matrix (buduje na `multipatch-level-parameter`)

### `dimmer-full-range-control-mode`
- **Add ControlModeDimmerFullRange to RGB Matrix** - Dodano tryb ControlModeDimmerFullRange do RGB Matrix

### `efx-empty-column-all-modes`
- **EFX: Show all modes (PanTilt, Dimmer, RGB) for empty columns** - Pokazywanie wszystkich trybów dla pustych kolumn
- **EFX: Always show all modes for ALL columns** - Zawsze pokazuj wszystkie tryby

### `row_mapping_fixed`
- Naprawione mapowanie wierszy

### `rgb_matrix_row_filter`
- Filtr wierszy dla RGB Matrix

### `layout_reorganization`
- **Improve RGB Matrix Editor layout organization** - Reorganizacja layoutu edytora RGB Matrix

---

## 🏗️ INFRASTRUKTURA I NARZĘDZIA

### Workflow Windows
- Dodano kompleksowy workflow GitHub Actions dla budowania instalatora Windows
- Instrukcje budowania dla Windows z Qt6 i MSYS2
- Obsługa NSIS dla tworzenia instalatorów

### Czyszczenie projektu
- Usunięto pliki CMakeFiles z indeksu git
- Dodano odpowiednie wpisy do .gitignore
- Usunięto pliki historii edytora (.history)

---

## 📊 Statystyki

- **Łączna liczba branchy lokalnych:** ~85
- **Branchy bugfix:** 13
- **Branchy feature:** 53+
- **Branchy eksperymentalne:** 15+

---

*Wygenerowano: 2026-01-17 | Zaktualizowano: 2026-03-03*
