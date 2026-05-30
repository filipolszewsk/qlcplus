# Preset Table v2 ↔ EFX Engine — API i integracja

Dokument opisuje kontrakt między widgetem **Preset Table v2** (DMX, wiersze, outputy) a **EFX Engine** (bank presetów, global FX, inputy VC). EFX Engine **nie pisze DMX** — tylko udostępnia dane i steruje flagą spatial na tabeli.

## Podział odpowiedzialności

| Warstwa | Klasa | Odpowiedzialność |
|---------|--------|------------------|
| Tabela | `PresetTableV2Widget` | Wiersze presetów, binding kolumn→fixture, outputy FG, **całe DMX**, `efx_selector` / secondary per output, matrix / sweep / continuous / flash |
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
| `PresetTableV2ControlIface` | `org.qlcplus.PresetTableV2ControlIface/1.2` |

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
| `spatialEffectSettings()` / `set…()` | **Deprecated** | Legacy chase bez banku — unikać |

Engine **nie może**: pisać DMX, zmieniać `m_activeRow`, czytać wierszy tabeli (brak API).

---

## Tabela → Engine: `PresetTableV2TransitionProviderIface`

Implementacja: `PresetTableV2TransitionWidget`.

### Bank presetów (osobno Sweep / Continuous)

| Metoda | Użycie |
|--------|--------|
| `transitionPresetCount(PTTransitionMode mode)` | Liczba presetów w banku Sweep lub Continuous |
| `effectiveTransitionPreset(mode, index)` | Preset + live DMX (`mergePreset`) |

Engine VC: taby **Sweep** | **Continuous** (bez kolumny Mode). XML: `<SweepPresets>`, `<ContinuousPresets>`.

### Global FX (tylko Properties)

Speed, Intensity, Min/Max ms — **EFX Engine → Properties** (nie na VC). Na widgecie: read-only podsumowanie.

| Metoda | Zwraca |
|--------|--------|
| `globalEffectSettings()` | `PTGlobalEffectSettings` |

**Sweep tab:** `applySweepPresetConstraints` przy zapisie z UI (nie w `mergePreset`) — domyślnie width=360, level=255, fade max 50%. Live DMX/UI w Operate **nie** jest nadpisywane co klatkę.

**Continuous dimmer:** tabela pisze DMX z `fadeTime=0` na kanałach (jak QLC EFX DimmerWave). `preset.fadeMs` dotyczy tylko sweep/flash między wierszami.

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
| `m_liveSweepPreset[o]` | **selector_sweep** (64+o): -1 off, 0…N-1 |
| `m_liveContinuousPreset[o]` | **selector_continuous** (192+o): -1 off |
| `m_liveSecondaryRow[o]` | Secondary z DMX 128+o (Continuous; 0=Properties combo, 1=wiersz 1 jak primary) |
| `m_outputs[o].sweepPresetIndex` / `continuousPresetIndex` | Domyślne bez DMX |
| `m_matrixState[o]` | Sweep / flash |

### Input ID — tabela (`presettablev2inputids.h`)

| ID | Funkcja |
|----|---------|
| `0…63` | `rowSelector(output)` — **primary** 0–255 (0=off, 1…N=wiersze, 101–110=flash) |
| `64+o` | `transSweep(o)` — bank Sweep (0=off); sweep przy zmianie **primary** |
| `128+o` | `transSecondaryRow(o)` — **tylko Continuous**: 0=Properties, 1=wiersz 1, 2=wiersz 2… |
| `192+o` | `transContinuousBank(o)` — bank Continuous (0=off) |

ID 64/128/192 to **osobne kanały VC** (wewnętrzne ID), nie podział jednego zakresu 0–255.
| `255` | Crossfade global (direction-locked 0↔255) |

### Crossfade + EFX (oba włączone w Properties)

| Tryb | Suwak | Primary / selectory | DMX |
|------|-------|---------------------|-----|
| **Sweep** | `xfEffective` 0→255 lub 255→0 (od `m_crossfadeStartPos`) | Primary → `m_stagedRow` do ruchu suwaka | Matrix sweep **active→staged**; promote primary na skrajności (0/255) |
| **Continuous** | Pozycja **≤127** = edycja staged; **>127** = live | Przy ≤127: primary/secondary/bank → bufory `m_staged*`; przy >127 primary od razu live | **Zawsze live** na wyjściu do commitu; **commit** przy narastającym przejściu **127→128** (`promoteStagedToLive`) |

**Global speed / intensity** (engine): zawsze **live**, nigdy staged.

Bufory staged (Continuous): `m_stagedRow`, `m_stagedSecondaryRow`, `m_stagedSweepPreset`, `m_stagedContinuousPreset`, engine `m_stagedColumnOverrides`.

### Input ID — engine (`ptefxinputids.h`)

Stable IDs **32–51** (nie indeks kolumny UI): `InputAxis`, `InputDuration`, `InputGlobalSpeed`, …

---

## Przepływ DMX — Instant vs EFX

**Matrix engine** (`useMatrixEngineLocked`):

```
spatialEffects.enabled && linkedTransitionWidgetId valid && presetCount > 0
```

**Warstwy (selectory NIE wykluczają się):**

| Warstwa | Selector | Działanie |
|---------|----------|-----------|
| **Continuous** | `192+o` > 0 | Stała fala primary↔secondary (bank Continuous) |
| **Sweep** | `64+o` > 0 | Przejście przy **zmianie primary** tylko gdy Continuous **nie** działa (bank Sweep) |

Oba selectory mogą być **ON**: Continuous steruje falą primary↔secondary. **Zmiana primary przy aktywnym Continuous nie uruchamia Sweep** (nowy primary od razu w fali). Sweep przy zmianie primary tylko gdy Continuous wyłączony lub brak secondary.

**EFX aktywny:** `sweep >= 0 OR continuous >= 0`.

W `writeDMXFixtureGroup` (matrix):

1. `sweepRunning` → preset **Sweep** (old primary → new primary).
2. `contActive` + secondary → preset **Continuous** (`forceContinuousBlend`).
3. Tylko `sweepOn` → preset Sweep (hold primary).
4. Fallback: `writeContinuousSpatial` / spatial chase / instant row.

### Checklist użytkownika (Instant działa)

1. Properties outputu: combo Sweep/Continuous = **Instant** (-1).
2. DMX `selector_sweep` / `selector_continuous` = **0**.
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

Podgląd krzywej: widget `PTDimmerWaveCurveWidget` w EFX Engine (oś X = 0–360°, Y = dimmer).

Kolumna **Duration** w banku EFX została ukryta — czas cyklu z **global Speed** (`effectiveDurationMs`).

## Współdzielony kod (math)

Oba pluginy kompilują z `presettablev2/`:

| Plik | Rola |
|------|------|
| `presettablev2effectengine.*` | Typy, `mergePreset`, `blendValues`, `transitionPresetIndexFromInput` |
| `ptparammatrixengine.*` | Speed→duration, sweep, intensity |
| `ptdimmerwaveengine.*` | `calculateDimmerWave` |
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
