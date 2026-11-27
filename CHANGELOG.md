# Changelog

## 0.1.1-alpha — 2026-11-27

Same code as 0.1.0-alpha. Built by CI only to prove that an installed
0.1.0-alpha updates itself in place (delta package) without losing user data.

## 0.1.0-alpha — 2026-11-27

Phase 1: UI foundation, installer and in-app updates.

- Command registry: menus, toolbar, tooltips, shortcut sheet and the command
  palette (F3 / Ctrl+K, fuzzy search, recently used first) are generated from
  one list of commands. No menu item without an implemented command.
- Original vector icon set (24 px grid, normal / hover / checked / disabled).
- Viewport: orientation cube, grid, axes and perspective toggles, seven
  standard views, Blender-style and CAD-style navigation presets.
- First-launch welcome (navigation style, units, first steps), Preferences
  dialog, example project and empty project.
- Log file with rotation; program files and user data kept in separate places.
- Velopack installer (per-user, no admin) and in-app updates: check, delta
  download with progress, restart to install, status bar indicator.
- Executable renamed to `HybridCAD.exe`.
- Tests: 52 automated tests plus UI smoke test; Windows CI installs
  0.1.0-alpha, updates it to 0.1.1-alpha from inside the app and uninstalls.

## 0.0.1-phase0 — 2026-11-27

Phase 0 technical proof of concept (not a product release).

- Layered C++20/CMake skeleton: core, mesh, kernel (OCCT), document, app (Qt).
- Named parameters with unit-aware expressions and cycle detection.
- Parametric history: Box, Chamfer, Fillet, NURBS Surface, Move Control Point;
  recompute, suppression, rollback, error propagation.
- Persistent topological naming (generator-role face names, history
  propagation, face-set edge/vertex references).
- NURBS: rational B-spline surfaces from editable definitions, control cage.
- STL (ASCII/binary) and OBJ import as reference meshes with analysis report.
- Qt Widgets UI with OCCT viewport, Object/Vertex/Edge/Face selection,
  context-sensitive properties, history and project browser, dark theme.
- 32 unit/geometry tests, scripted UI smoke test, Linux and Windows CI.
- Phase 0 documentation set in `docs/`.
