# Preset Table v2 ↔ EFX Engine — API i integracja

Dokument opisuje kontrakt między widgetem **Preset Table v2** (DMX, wiersze, outputy) a **EFX Engine** (bank presetów, global FX, inputy VC). EFX Engine **nie pisze DMX** — tylko udostępnia dane i steruje flagą spatial na tabeli.

Model użytkowy ma dwie warstwy:

- **Transitions** — sposób wejścia nowego primary row na scenę: instant albo preset przejścia.
- **Continuous FX** — ciągły efekt interpolujący primary row ↔ secondary row na fixture group.

Crossfade/staged/commit dotyczy tylko wyborów: primary row, secondary row, Transition preset i Continuous FX preset. Parametry EFX (`Duration`, `WaveWidth`, `OffsetDir`, `Shape`, itd.) są zawsze **live** i zmieniają aktualny efekt od razu.

Preset Table Properties → Crossfade ma globalny tryb **Continuous FX selector mode**:

- **Live** — `transContinuousBank(o)` zmienia preset natychmiast.
- **Staged commit** — wybór presetu trafia do staged i wchodzi live dopiero przy commicie crossfade.
- **Smooth morph** — wybór presetu trafia do staged, a parametry Continuous FX morphują się live→staged zgodnie z progressem crossfade.

## Podział odpowiedzialności

| Warstwa | Klasa | Odpowiedzialność |
|---------|--------|------------------|
| Tabela | `PresetTableV2Widget` | Wiersze presetów, binding kolumn→fixture, outputy FG, **całe DMX**, Transition selector / Continuous FX selector / secondary per output, matrix / transitions / continuous FX / flash |
| Engine | `PresetTableV2TransitionWidget` | Bank `PTTransitionPreset`, global speed + min/max ms + intensity, mapowanie inputów DMX, **brak DMX** |

Pluginy łączą się przez **Qt interfaces** + **ID widgetów** w Virtual Console (osobne `.dylib`).

## Linkowanie (handles VC)

| Kierunek | XML | Pole |
|----------|-----|------|
| Tabela → Engine | `LinkedTransitionWidgetId` | `m_linkedTransitionWidgetId` |
| Engine → Tabela | `TargetTableId` | `m_targetTableId` |

**Design:** Tabela wybiera engine w Properties → Transition panel. Engine w Properties wybiera tabelę i woła `table->setLinkedTransitionWidgetId(engine->id())`.

**Lookup:** `VirtualConsole::instance()->widget(id)` + `qobject_cast<…Iface*>()`.

**Discovery:** `PresetTableV2VCLookup::allTables()`, `allTransitionWidgets()`.

### Wersje interfejsów (IID)

| Interface | IID |
|-----------|-----|
| `PresetTableV2TransitionProviderIface` | `org.qlcplus.PresetTableV2TransitionProvider/2.3` |
| `PresetTableV2ControlIface` | `org.qlcplus.PresetTableV2ControlIface/1.4` |

Pliki: `presettablev2transitionprovideriface.h`, `presettablev2controliface.h`.

---

## Engine → Tabela: `PresetTableV2ControlIface`

Implementacja: `PresetTableV2Widget`.

| Metoda | Kiedy | Znaczenie |
|--------|-------|-----------|
| `spatialEffectsEnabled()` | Sync UI engine | `m_spatialEffects.enabled` — wymagane m.in. do matrix engine |
| `setSpatialEffectsEnabled(bool)` | Checkbox „Enable EFX” | Włącza spatial; przy wyłączeniu resetuje stan matrix |
| `linkedTransitionWidgetId()` / `setLinkedTransitionWidgetId(quint32)` | Properties | ID widgetu engine |
| `refreshTransitionPresetCache()` | Zmiana presetów / DMX override | Cache `m_cachedTransitionPresetCount` |
| `requestTableFlash(tableRow, transitionPresetIndex)` | Flash | Ustawia flash matrix na outputach z aktywnym wierszem |
| `continuousCrossfadeStagedEditing()` | Legacy compat | Historycznie sterowało staged parametrami engine; parametry EFX są teraz zawsze live |
| `fixtureGroupSpanAlongAxis(preset, global)` | EFX podgląd | Span osi FG; 0 = brak |
| `spatialGridPreview(preset, global, out)` | EFX podgląd | Siatka kolejności/offsetów |
| `spatialEffectSettings()` / `set…()` | **Deprecated** | Legacy chase bez banku — unikać |

Engine **nie może**: pisać DMX, zmieniać `m_activeRow`, czytać wierszy tabeli (brak API).

---

## Tabela → Engine: `PresetTableV2TransitionProviderIface`

Implementacja: `PresetTableV2TransitionWidget`.

### Bank presetów (Transitions / Continuous FX)

| Metoda | Użycie |
|--------|--------|
| `transitionPresetCount(PTTransitionMode mode)` | Liczba presetów w banku Transitions lub Continuous FX |
| `effectiveTransitionPreset(mode, index)` | Preset + live DMX (`mergePreset`) |

Engine VC: taby **Transitions** | **Continuous FX** (bez kolumny Mode). XML pozostaje kompatybilny: `<SweepPresets>`, `<ContinuousPresets>`.

### Global FX (tylko Properties)

Speed, Intensity, Min/Max ms — **EFX Engine → Properties** (nie na VC). Na widgecie: read-only podsumowanie.

| Metoda | Zwraca |
|--------|--------|
| `globalEffectSettings()` | `PTGlobalEffectSettings` |

**Transitions tab:** `applySweepPresetConstraints` — width=360, level=255, fade in max 50%; **fade out wyłączony** na osi transition (`dimmerSweepAttack01`). Live DMX/UI w Operate **nie** jest nadpisywane co klatkę.

**Continuous FX dimmer:** tabela pisze DMX z `fadeTime=0` na kanałach (jak QLC EFX DimmerWave). `preset.fadeMs` dotyczy tylko transition/flash między wierszami.

### Persistencja global Min/Max (workspace)

Min/Max ms zapisują się w **workspace** (`.qxw`), nie w dokumencie fixture (`.qxcf`):

| XML | Gdzie |
|-----|--------|
| `GlobalMinMs` / `GlobalMaxMs` | Atrybuty root `<PluginWidget>` (backup) |
| `MinMs` / `MaxMs` | `<GlobalEffect>` |

### External input — global Speed / Intensity

**Properties** widgetu EFX Engine → sekcja „External inputs (global)”.

XML: `<GlobalSpeedInput>`, `<GlobalIntensityInput>`. Etykieta ze `*` gdy input jest zmapowany.

Kolumny banku: dwuklik nagłówka kolumny (Design), jak wcześniej.

### Live override

| Metoda | Znaczenie |
|--------|-----------|
| `hasLiveColumnOverride(quint8 inputId)` | DMX trzyma parametr (`PTEfxCol::Input*`) — np. Duration dla `effectiveDurationMs` |

### Flash

`requestFlash(row, presetIndex)` → delegacja do `requestTableFlash` na tabeli.

### Nieużywane przez tabelę

`transitionMode()` — tabela bierze `playbackMode` z wiersza banku, nie tę metodę.

---

## Stan tylko na tabeli (engine nie widzi)

| Stan | Znaczenie |
|------|-----------|
| `m_rows[]` | Wartości kanałów |
| `m_activeRow[o]` | Primary row (-1 = off) |
| `m_liveSweepPreset[o]` | **Transition selector** / legacy `selector_sweep` (64+o): -1 off, 0…N-1 |
| `m_liveContinuousPreset[o]` | **Continuous FX selector** / legacy `selector_continuous` (192+o): -1 off |
| `m_liveSecondaryRow[o]` | Secondary z DMX 128+o (Continuous FX; 0=Properties combo, 1=wiersz 1 jak primary) |
| `m_outputs[o].sweepPresetIndex` / `continuousPresetIndex` | Domyślne bez DMX |
| `m_matrixState[o]` | Sweep / flash |

### Input ID — tabela (`presettablev2inputids.h`)

| ID | Funkcja |
|----|---------|
| `0…63` | `rowSelector(output)` — **primary** 0–255 (0=off, 1…N=wiersze, 101–110=flash) |
| `64+o` | `transSweep(o)` — Transition selector (0=instant); transition przy zmianie **primary** |
| `128+o` | `transSecondaryRow(o)` — **tylko Continuous FX**: 0=Properties, 1=wiersz 1, 2=wiersz 2… |
| `192+o` | `transContinuousBank(o)` — Continuous FX selector (0=off) |

ID 64/128/192 to **osobne kanały VC** (wewnętrzne ID), nie podział jednego zakresu 0–255.
| `255` | Crossfade global (direction-locked 0↔255) |

### Crossfade + Transitions (matrix)

- Wymaga **linku do EFX Engine** + **Transition selector** (64+o) > 0. **Nie** wymaga checkboxa „Enable spatial transition”.
- Suwak = **`globalProgress` 0…1** (lub zegar — patrz `InputCrossfadeManual`) wzdłuż planu spatial. Blend: `sweepBlend01` + **tylko atak** (bez fade-out). **Continuous** = osobno, pełny cykl 360°.

**Crossfade manual** (EFX Engine Properties → `InputCrossfadeManual`, ID 52): **>127** = suwak tabeli (255) steruje postępem; **≤127** = **XF clock** — postęp 0→1 w czasie `effectiveDurationMs(global, …, honorPreset=false)` (global speed + min/max ms), **jeden** tick MasterTimer na klatkę `writeDMX` (`m_crossfadeClockElapsedMs`). Zegar resetuje się przy nowym staged / nowym sweep crossfade. Domyślnie bez inputu: manual ON.
- Gdy **Continuous FX** jest aktywny (192+o + secondary), transition crossfade **nie** przejmuje warstwy.

### Continuous FX (matrix)

- Wymaga linku EFX + **Continuous FX selector** (192+o) + secondary (128+o lub Properties). Matrix działa **bez** checkboxa spatial.
- **Zegar 360°:** `m_continuousElapsedMs` + min/max + speed → [`matrixDimmerAtPoint`](presettablev2widget.cpp) (fala primary↔secondary). Suwak crossfade **nie** zmienia fazy — tylko staged vs live presetów (patrz niżej).
- **Nie** używa globalnego `applyFadeValue` na wszystkich kanałach (to dawałoby jednoczesne A→B).

### Crossfade + staged selections

| Suwak | Wybory | DMX |
|-------|--------|-----|
| ≤127 staged | `m_staged*` | Zamrożone **live** |
| narastające >128 | commit | `promoteStagedToLive` |

**Parametry EFX** (global i per-column): zawsze **live**, nigdy staged. Dotyczy to Transitions i Continuous FX.

Bufory staged selection: `m_stagedRow`, `m_stagedSecondaryRow`, `m_stagedSweepPreset`, `m_stagedContinuousPreset`. Engine nie trzyma już staged overrides parametrów. Tryb Continuous FX selector decyduje, czy `m_stagedContinuousPreset` jest używany jako commit-only, morph target, czy omijany przez tryb live.

### Input ID — engine (`ptefxinputids.h`)

Stable IDs **32–51** (nie indeks kolumny UI): `InputAxis`, `InputDuration`, `InputGlobalSpeed`, …

---

## Przepływ DMX — Instant vs EFX

**Matrix engine**:

```
linkedTransitionWidgetId valid && presetCount > 0
```

Per-output decyzję zbiera `resolveOutputPlaybackStateLocked()`: matrix jest używany, gdy jest aktywny spatial checkbox, crossfade, Transition selector albo Continuous FX selector. Dzięki temu Continuous FX i crossfade mogą działać bez legacy checkboxa spatial.

**Warstwy (selectory NIE wykluczają się):**

| Warstwa | Selector | Działanie |
|---------|----------|-----------|
| **Continuous FX** | `192+o` > 0 | Stała fala primary↔secondary (bank Continuous FX) |
| **Transitions** | `64+o` > 0 | Przejście przy **zmianie primary** tylko gdy Continuous FX **nie** działa (bank Transitions) |

Oba selectory mogą być **ON**: Continuous FX steruje falą primary↔secondary. **Zmiana primary przy aktywnym Continuous FX nie uruchamia Transition** (nowy primary od razu w fali). Transition przy zmianie primary tylko gdy Continuous FX wyłączony lub brak secondary.

**EFX aktywny:** `sweep >= 0 OR continuous >= 0`.

W `writeDMXFixtureGroup` (matrix):

1. `sweepRunning` → preset **Transitions** (old primary → new primary).
2. `contActive` + secondary → preset **Continuous FX** (`forceContinuousBlend`).
3. Tylko `sweepOn` → preset Transitions (hold primary).
4. Fallback: `writeContinuousSpatial` / spatial chase / instant row.

### Checklist użytkownika (Instant działa)

1. Properties outputu: combo Transitions/Continuous FX = **Instant** / **Off** (-1).
2. DMX Transition selector / Continuous FX selector = **0**.
3. DMX **row selector** ≥ 1 (wybrany wiersz tabeli).
4. Przy problemach: wyłącz **Enable EFX** na engine albo odlinkuj — wtedy zawsze tryb tabeli.

**Uwaga:** Stary projekt z XML `TransitionPreset="0"` = zawsze bank preset 0, nie Instant. Ustaw **Instant** w Properties i zapisz projekt.

---

## Fade In / Out (jak QLC `EFX::DimmerWave`)

- **0%** = brak strefy fade → ostre przejście (nie „blokuje” outputu).
- **100%** = cała szerokość pakietu (`waveWidth`) to ta strefa.
- Implementacja: `PTDimmerWaveEngine::dimmerAtPhaseInWidth` (zgodna z `engine/src/efx.cpp`).
- **Square** (`waveShape=1`): twardy prostokąt w pakiecie; fade % ignorowane na krawędziach.
- **Wave width = 0** → dimmer zawsze 0 (to nie bug fade).

Podgląd w EFX Engine (Operate / Design):

| Widget | Zawartość |
|--------|-----------|
| `PTDimmerWaveCurveWidget` | Krzywa dimmer: oś X = 0–360°, Y = dimmer |
| `PTSpatialFixtureGridWidget` | Siatka FG: **#kolejność**, **offset°**, **faza %**; wymaga linku do tabeli w trybie Fixture Group |

Limit **Offset step**: `max = 360 / effectiveSlots`, gdzie `effectiveSlots = wings × ceil((span/wings) / blocks)`; spinbox w banku ma dynamiczny max; kolizje offsetów (duplikaty °) — czerwona obwódka komórki.

Kolumna **Duration** w banku EFX została ukryta — czas cyklu z **global Speed** (`effectiveDurationMs`).

## Współdzielony kod (math)

Oba pluginy kompilują z `presettablev2/`:

| Plik | Rola |
|------|------|
| `presettablev2effectengine.*` | Typy, `mergePreset`, `blendValues`, `transitionPresetIndexFromInput` |
| `ptparammatrixengine.*` | Speed→duration, sweep, intensity |
| `ptdimmerwaveengine.*` | `calculateDimmerWave`, `maxOffsetStepForGrid` |
| `ptspatialfixtureplan.*` | Kolejka spatial, `buildGridPreview` |
| `ptspatialfixturegridwidget.*` | Podgląd siatki FG w EFX Engine |
| `ptefxinputids.h` | Stable input IDs |

`mergePreset(base, liveOverrides)` — mapowanie bajtów DMX na pola `PTTransitionPreset` (patrz `presettablev2effectengine.cpp`).

---

## Minimalny kontrakt pod rewrite engine

### Engine musi

1. `PresetTableV2TransitionProviderIface` (IID 2.2+).
2. `QVector<PTTransitionPreset>` + `PTGlobalEffectSettings`.
3. `m_liveColumnOverrides` + `hasLiveColumnOverride`.
4. Po zmianach: `linkedTable()->refreshTransitionPresetCache()`.
5. Enable EFX: `setSpatialEffectsEnabled(checked)`.
6. Link + `setLinkedTransitionWidgetId` przy Properties.
7. Inputy na `PTEfxCol::Input*` (nie index kolumny).

### Engine nie powinien

- Być `DMXSource`.
- Dotykać faderów / `writeDMX`.

### Tabela (bez zmian przy samym engine)

- Jedyny `DMXSource`.
- `livePri < 0` → zawsze direct row; `resetMatrixStateLocked` przy przejściu na Instant.

---

## Pliki referencyjne

| Temat | Ścieżka |
|-------|---------|
| Provider API | `plugins/vcwidgets/presettablev2/presettablev2transitionprovideriface.h` |
| Control API | `plugins/vcwidgets/presettablev2/presettablev2controliface.h` |
| DMX | `plugins/vcwidgets/presettablev2/presettablev2widget.cpp` |
| Engine | `plugins/vcwidgets/presettablev2transition/presettablev2transitionwidget.cpp` |
| Input IDs | `ptefxinputids.h`, `presettablev2inputids.h` |
