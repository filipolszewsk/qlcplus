# Preset Table v2 Performance Audit

## Summary

This note tracks the first CPU/performance pass across the three linked VC widgets:

- `presettablev2`
- `presettablev2transition` / Preset Table Engine
- `multibutton`

The current architecture is much safer than the earlier live lookup model because playback mostly reads immutable provider snapshots. The remaining performance risks are mostly repeated UI/link discovery and per-fixture recomputation in hot DMX paths.

## Progress

- ~~Priority 1: MultiButton effective-action cache~~ Done in `46b280ba7`.
- ~~Priority 2: Precompute `PTSpatialFixturePlan` in Position DMX~~ First pass done in `a5f20ee14`.
- Priority 3: Tick-local cache for `effectiveValuesForPoint`.
- Priority 4: Coalesce Preset Table Engine snapshot/rebuild.

## ~~Priority 1: MultiButton Effective-Action Cache~~

Status: done in `46b280ba7`.

`MultiButtonWidget` can be called from paint, UI highlight, shared-bus recall, and `writeDMX()`. Before this pass, those paths could repeatedly rebuild the effective widget action list, including auto-slave discovery.

The implemented cache avoids repeating these expensive calls in frequent paths:

- ~~full VC tree lookup via `findChildren<VCWidget*>`;~~
- ~~`PresetTableV2VCLookup::allTables()`;~~
- ~~`multiButtonLinkedSlaveActions(...)`.~~

Implemented behavior:

- ~~cache `effectiveWidgetActions()`;~~
- ~~cache the leader action used for labels/count/highlight;~~
- ~~cache whether any action supports staged selection;~~
- ~~invalidate on link/config/load/properties/target-destroyed;~~
- ~~use a short TTL/revision check for external source/slave changes.~~

This keeps staged/live behavior unchanged while avoiding repeated global lookup work during normal playback.

## ~~Priority 2: Precompute `PTSpatialFixturePlan` In Position DMX~~

Status: first pass done in `a5f20ee14` with a tick-local spatial-plan cache in Position DMX.

`PresetTableV2Widget::writeDMXPositionFixtureGroup()` still builds some spatial plans inside the per-fixture loop. That makes cost grow as:

`outputs x fixtures x active layers`

The implemented optimization avoids rebuilding identical plans inside the fixture loop. Plans are cached once per output/tick and reused for matching spatial preset signatures:

- ~~transition sweep;~~
- ~~live/staged interpolation;~~
- ~~live/staged 2D FX;~~
- ~~legacy sweep motion;~~
- ~~MultiFX.~~

The fixture loop now reuses the cached plan and only reads `indexByPoint` / applies values for matching signatures.

## Priority 3: Tick-Local Cache For `effectiveValuesForPoint`

Fixture Group grid overrides make `effectiveValuesForPoint(row, output, point)` flexible, but it can be called several times per point in one tick for active/staged/secondary rows.

The next safe improvement is a tick-local cache keyed by:

- row;
- output;
- point.

It should only live for the current DMX write and should not change XML or preset semantics.

## Priority 4: Coalesce Preset Table Engine Snapshot/Rebuild

Preset Table Engine publishes provider snapshots and notifies slave engines. In large projects, a master bank edit can cascade into multiple slave snapshot publishes, table rebuilds, and preview refreshes.

The intended follow-up is revision-based coalescing:

- publish snapshot immediately for playback safety;
- defer table rebuild/preview refresh through one queued UI update;
- skip slave rebuild if the relevant source revision did not change.

## Notes

Global VC lookup is acceptable in properties dialogs, one-shot relinking, and diagnostics. It should not happen repeatedly from:

- DMX tick;
- paint/highlight paths;
- shared-bus recall loops;
- high-frequency input handling.

Diagnostics should stay rate-limited in hot paths. Full breadcrumb flushes should remain reserved for important commits, link changes, startup/shutdown, and crash dumps.
