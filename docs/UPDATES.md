# Installation and updates

HybridCAD uses [Velopack](https://velopack.io) (MIT licence) for the Windows
installer and in-app updates. The application talks to it through Velopack's
C library (`velopack_libc`); packages are produced with the `vpk` tool.

## Where things live

| What | Where | Touched by updates? |
|---|---|---|
| Program files | `%LOCALAPPDATA%\HybridCAD.App\current\` | Replaced |
| Updater | `%LOCALAPPDATA%\HybridCAD.App\Update.exe` | Replaced |
| User data (logs, recovery, caches) | `%LOCALAPPDATA%\HybridCAD\HybridCAD\` | Never |
| Settings | Registry `HKCU\Software\HybridCAD\HybridCAD` | Never |
| Projects (from Phase 2) | `Documents\HybridCAD Projects\` (user's choice) | Never |

The package id `HybridCAD.App` is deliberately different from the user data
folder name, so uninstalling (which deletes `%LOCALAPPDATA%\HybridCAD.App`)
cannot remove user data. This is verified in CI.

Installation is per user and needs no administrator rights.

## How an update works

1. On start (if enabled in Preferences, 4 s after launch) or from
   **Help → Check for Updates**, the app downloads the small release index
   `releases.<channel>.json` from the feed.
2. If a newer version exists, the status bar shows "Update X available".
   The update dialog shows whether it is a **delta** (only the changed bytes)
   or a full package, and its size.
3. **Download** fetches the delta(s) in the background with progress. Velopack
   checks each file's SHA-256 hash. If anything fails, nothing is changed.
4. **Restart and Install** closes the app; `Update.exe` rebuilds the new
   version from the deltas, swaps `current\`, and restarts HybridCAD. If the
   swap fails, the previous version stays in place.

Updates are never a full reinstall, and they never run without the user's
click.

## Channels and feed

- Channels: `stable` (default) and `beta`, chosen in Preferences.
- Default feed: `https://github.com/metjus/cad/releases/latest/download`
  (GitHub Releases over HTTPS). **Not final**: where production releases are
  hosted is an open owner decision; the feed can be changed in Preferences
  (Release feed) or at build time with `-DHCAD_UPDATE_FEED=...`.
- No release has been published to the feed yet. CI uses a local folder feed.

## Building an installer

```powershell
# after building with -DHCAD_WITH_VELOPACK=ON and staging the files
./packaging/windows/stage.ps1 -BuildDir build -StageDir stage -Prefix $env:CONDA_PREFIX\Library
vpk pack --packId HybridCAD.App --packVersion 0.1.0-alpha --packDir stage `
  --mainExe HybridCAD.exe --packTitle HybridCAD --channel stable --outputDir releases
```

`releases/` then contains `HybridCAD.App-stable-Setup.exe`, the full package,
delta packages (when an older release is in the folder) and the release index.
Upload the whole folder content to the feed.

## Open items

- **Code signing**: the installer and executables are not signed, so
  SmartScreen warns on first install. A code-signing certificate is an open
  owner decision; `vpk pack` signs automatically once one is configured.
- **Production feed hosting** (see above).
- User-triggered rollback to the previous version is not implemented; Velopack
  only keeps the previous version if applying an update fails.
