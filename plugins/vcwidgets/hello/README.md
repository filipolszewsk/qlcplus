# HelloVCWidget — Example VC Widget Plugin for QLC+

This is the minimal reference implementation of a QLC+ VC widget plugin.
Copy this directory and adapt it to create your own widget.

## What it shows

- How to implement `VCWidgetPluginInterface` (factory / metadata).
- How to subclass `VCWidget` and implement `loadXML` / `saveXML`.
- How to handle Design vs. Operate mode via `slotModeChanged`.
- How to connect to `MasterTimer::beat` for beat-synchronized animations.
- How to draw a custom `paintEvent`.

## Files

| File | Role |
|------|------|
| `helloplugin.h/.cpp` | Plugin factory — implements `VCWidgetPluginInterface`, carries `Q_PLUGIN_METADATA` |
| `hellowidget.h/.cpp` | The actual widget — inherits `VCWidget` |
| `CMakeLists.txt` | Build script |

## Building

### Prerequisites

- Qt 5.x — **must match the exact version used by the QLC+ binary you are targeting**
  (check the QLC+ release notes or run `qlcplus --version`)
- C++14 compatible compiler
- QLC+ source tree (for headers) and build directory (for linking)

### Steps

```bash
# 1. Clone or download the QLC+ source (for headers only, no need to rebuild)
git clone https://github.com/mcallegari/qlcplus.git /path/to/qlcplus-src

# 2. Configure your plugin — point at the QLC+ source and build trees
cmake -B build \
  -DQLCPLUS_SRC_DIR=/path/to/qlcplus-src \
  -DQLCPLUS_BUILD_DIR=/path/to/qlcplus-build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/gcc_64

# 3. Build
cmake --build build

# 4. Install
#   macOS:   cp build/libhello_vcwidget.dylib ~/Library/Application\ Support/QLC+/VCWidgets/
#   Linux:   cp build/libhello_vcwidget.so ~/.qlcplus/vcwidgets/
#   Windows: copy build\hello_vcwidget.dll %APPDATA%\QLC+\VCWidgets\
```

After restarting QLC+, the widget appears in:
**Virtual Console → Add → Hello Widget**

## Distributing your plugin

Package your `.dll/.so/.dylib` as a `.qlcvcw` file (ZIP archive) with:

```
myplugin-1.0.0.qlcvcw
├── manifest.json
├── icon.png
├── README.md
└── platforms/
    ├── windows-x64/myplugin.dll
    ├── macos-universal/myplugin.dylib
    └── linux-x64/myplugin.so
```

Users install it via **Tools → VC Widget Plugins… → Install from file…**
or by placing the binary in their vcwidgets folder.

## License

This example is released under CC0 1.0 (public domain). No attribution required.
