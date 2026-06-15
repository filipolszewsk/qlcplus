# Silent QLC+ Close / No Crash Report Runbook

This guide is for cases where QLC+ simply disappears/closes and macOS does not show a normal crash report.

## 1. First Check If macOS Created A Real Crash Report

Look in:

```text
~/Library/Logs/DiagnosticReports/
```

Search for files named like:

```text
qlcplus-YYYY-MM-DD-*.ips
```

If a new `.ips` file exists, keep it. This is the best report because it contains the native stack trace.

## 2. If There Is No `.ips`, Collect VC Widget Logs

Check these files:

```text
~/Library/Logs/QLC+/vcwidgets-crash.log
~/Library/Logs/QLC+/vcwidgets.log
~/QLC+.log
```

Send all three if possible.

`vcwidgets-crash.log` is the most important one for silent closes. It may contain the last VC widget actions and emergency dump.

`vcwidgets.log` contains normal breadcrumbs such as preset edits, spread changes, staged rows, transition commits, and session markers.

`~/QLC+.log` contains the standard QLC+ application log.

## 3. What “No Crash Report” Usually Means

If there is no `.ips`, the process may not have crashed in the normal macOS sense.

Common causes:

- QLC+ exited through `quit()` / `exit()` / event loop shutdown.
- The process was killed by `SIGTERM` or `SIGKILL`.
- A hot-reload/install-local action replaced or disturbed a loaded plugin.
- Qt or QLC+ hit a fatal path that closed the application without a normal CrashReporter report.
- A VC widget bug caused cleanup/shutdown instead of a classic `SIGSEGV`.

This is why `vcwidgets-crash.log` exists: it gives us application context even when macOS does not generate `.ips`.

## 4. What To Write Down Immediately

After QLC+ disappears, before reopening it if possible, note:

- Exact time of the close.
- What mode was active: Design / Operate.
- Which VC widgets were open.
- Which preset/output/selection was active.
- Last action: slider move, preset change, transition edit, MIDI input, crossfade, save/load, install-local.
- Whether QLC+ was running from the installed app or from a dev build.

Even a rough note like “16:06, Position mode, output 1, Spread Pan MIDI, row 3” is useful.

## 5. Do Not Overwrite Evidence

Avoid immediately doing many new actions in QLC+ before copying logs. The logs are append-only, but new sessions add noise.

Recommended quick copy:

```bash
mkdir -p ~/Desktop/qlc-crash-logs
cp ~/Library/Logs/QLC+/vcwidgets-crash.log ~/Desktop/qlc-crash-logs/ 2>/dev/null
cp ~/Library/Logs/QLC+/vcwidgets.log ~/Desktop/qlc-crash-logs/ 2>/dev/null
cp ~/QLC+.log ~/Desktop/qlc-crash-logs/ 2>/dev/null
cp ~/Library/Logs/DiagnosticReports/qlcplus-*.ips ~/Desktop/qlc-crash-logs/ 2>/dev/null
```

## 6. How To Read The New VC Diagnostics

Useful markers:

```text
session start
session normal quit
previous session ended unexpectedly
vcwidgets emergency dump
reason=signal N
reason=std::terminate
```

Interpretation:

- `session normal quit`: QLC+ closed normally from Qt/application point of view.
- `previous session ended unexpectedly`: previous run did not record a clean quit.
- `reason=signal 11`: likely segmentation fault.
- `reason=signal 6`: likely abort/assert/fatal.
- `reason=signal 15`: process was terminated, often no macOS `.ips`.
- `std::terminate`: uncaught exception or C++ fatal termination.

## 7. Known VC Crash Signatures

If `vcwidgets-crash.log` has a native backtrace, check the top plugin frames:

```text
PresetTableV2VCLookup::allVcWidgets()
QObject::findChildren<VCWidget*>
PresetTableV2Widget::transitionProviderLocked()
PresetTableV2Widget::writeDMX(...)
```

This means the DMX/render thread touched the live Qt widget tree. That is not a normal data problem in a preset; it is a thread-safety bug. The fix is to use a cached provider snapshot from the GUI thread.

```text
MultiButtonWidget::widgetLinkTarget()
qobject_cast<PresetTableV2MultiButtonTargetIface*>
MultiButtonWidget::entryCount()
MultiButtonWidget::syncDynamicEntryCountLayout()
```

This means a MultiButton linked-widget target was missing, stale, reloaded, or no longer had the expected interface. The fix is to clear/re-resolve the target and avoid casting stale `VCWidget*` pointers.

When there is no `.ips`, these signatures in `vcwidgets-crash.log` are still actionable. Use the timestamp in the emergency dump and compare it with the last lines of `vcwidgets.log`.

## 8. What To Send For Debugging

Send:

- Latest `.ips`, if it exists.
- `vcwidgets-crash.log`.
- `vcwidgets.log`.
- `~/QLC+.log`.
- The project file, if the issue is reproducible with it.
- Short reproduction steps.

Best format:

```text
Time:
Mode:
Widget:
Output:
Preset/row:
Last action:
MIDI/external input active:
Crash report .ips exists: yes/no
Logs attached: yes/no
```

## 9. Important Note For Clients

If QLC+ closes without a macOS crash report, that does not mean “there is no bug”.

It means macOS did not classify the shutdown as a normal application crash. The VC widget diagnostics are there specifically to catch those cases.
