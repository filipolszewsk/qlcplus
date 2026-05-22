# QLC+ VC Widget Registry

This repository is the public index of VC Widget plugins available for
[QLC+](https://www.qlcplus.org/). Users can browse and install plugins
directly from **Virtual Console → Get more widgets... → Browse Library**.

## How to submit your plugin

1. **Build your plugin** for each platform you support (see the
   [VC Widget Plugin Dev Guide](https://github.com/Hpforfilm/qlcplus/blob/feature/vc-widget-plugin-hub/plugins/vcwidgets/VC_WIDGET_PLUGIN_DEV_GUIDE.md)).

2. **Create a GitHub Release** on your plugin's repository. Attach the
   `.qlcvcw` package file(s) as release assets.

3. **Open a Pull Request** to this repository. Add your plugin to `index.json`:

```json
{
  "plugin_id": "com.yourname.vcwidgets.yourplugin",
  "name": "Your Plugin",
  "version": "1.0.0",
  "author": "Your Name",
  "category": "DMX Control",
  "description": "What your plugin does in one or two sentences.",
  "homepage": "https://github.com/you/your-plugin",
  "min_qlc_version": "4.14.0",
  "platforms": {
    "macos_arm64": "https://github.com/you/your-plugin/releases/download/v1.0.0/yourplugin-1.0.0-macos-arm64.qlcvcw",
    "linux_x64":   "https://github.com/you/your-plugin/releases/download/v1.0.0/yourplugin-1.0.0-linux-x64.qlcvcw",
    "windows_x64": "https://github.com/you/your-plugin/releases/download/v1.0.0/yourplugin-1.0.0-windows-x64.qlcvcw"
  }
}
```

### Rules

- `plugin_id` must be globally unique and use reverse-domain notation
  (`com.yourname.vcwidgets.yourplugin`). It must never change between versions.
- `version` must use semver format (`MAJOR.MINOR.PATCH`).
- Each URL in `platforms` must be a direct download link to a `.qlcvcw` file.
- Platform keys: `macos_arm64`, `macos_x64`, `linux_x64`, `windows_x64`.
  Include only the platforms you have binaries for.
- You only need one platform entry to be accepted.

### Automatic validation

Every PR runs a GitHub Action that:
- Validates `index.json` against `schema.json`
- Checks for duplicate `plugin_id` values
- Sends HTTP HEAD requests to all platform URLs to verify they are reachable

## Updating an existing plugin

Submit a PR that bumps the `version` field and updates the download URLs.
QLC+ compares the installed version string with the registry version string;
if the registry has a higher version, it shows "Update available" in the UI.

## Categories

Suggested category values (others are accepted):
- `DMX Control` — widgets that write DMX directly
- `Functions` — widgets that trigger QLC+ functions (scenes, chasers, etc.)
- `Automation` — timers, macros, sequences
- `Custom` — anything else

## License

By submitting a PR you confirm that your plugin is compatible with the
Apache License 2.0 used by QLC+, or that you are distributing it under a
compatible open-source license. Closed-source plugins are also accepted
in the registry; the registry only indexes metadata and download URLs.
