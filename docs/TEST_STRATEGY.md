# Test Strategy

Rule from the brief: **never claim a feature works unless it is implemented
and tested.** Every phase ships with automated tests for its acceptance
criteria and the results are reported at review.

## 1. Test levels

| Level | Tooling | Runs where | Phase 0 status |
|---|---|---|---|
| Unit (core: expressions, units, parameters) | GoogleTest | every push, Linux + Windows | 6 tests |
| Geometry (kernel facade, features) | GoogleTest + OCCT checks (`BRepCheck_Analyzer`, volume/area/topology counts, centroids) | every push | 6 tests (features, history, suppression, rollback) |
| Topological reference stability | GoogleTest; upstream parameter sweeps; resolve references after each recompute | every push | 6 tests |
| NURBS | GoogleTest; degree/knots/rationality/evaluation, history edits | every push | 6 tests |
| Mesh import/analysis | GoogleTest with fixtures (ASCII/binary STL, OBJ quads, open box, non-manifold, generated defects) | every push | 8 tests |
| UI smoke | App `--smoke-test`: real window, real OCCT picking at projected points, parameter edit, persistent re-selection, mesh import, screenshots | Linux (Xvfb) every push; Windows with Mesa llvmpipe (non-blocking until stable) | 22 checks |
| Integration (save/open, import/export round trips) | GoogleTest + golden files | Phase 2+ | – |
| Update tests | Velopack: install N, publish N+1 delta, update, verify files/settings/projects untouched, forced failure → rollback | Windows CI, Phase 1 | – |
| Performance | Benchmarks (recompute time, tessellation, open time) with thresholds | nightly, Phase 4+ | per-feature timing recorded |
| Mesh reconstruction | Corpus + metrics (max/RMS deviation, validity, patch count, continuity, STEP export, reopen) | nightly, Phase 6 | fixtures started |

Current count: **32 unit/geometry tests + 1 UI smoke test (22 checks)**, all
passing on Linux (OCCT 7.6.3, Qt 6.4.2) and on Windows (MSVC 2022, OCCT 7.9,
Qt 6.8, Mesa llvmpipe for the smoke test).

## 2. Geometry regression approach

Assert on *properties*, not on raw topology indices:

- validity (`BRepCheck_Analyzer`), solid count, face/edge/vertex counts;
- volume, area, bounding box with tolerances (1e-6 mm for analytic cases);
- centroid of named faces (e.g. chamfer face follows the edge while the box
  width sweeps 60/120/25/300 mm);
- persistent names exist and resolve uniquely (every box edge and vertex).

Phase 4 adds a golden-file corpus (STEP + expected properties) and a
"known-bad" corpus of kernel failures to track OCCT upgrades (R1, R9).

## 3. Parametric tests (from brief §14)

Edit upstream dimensions; recompute dependants; save/reopen (Phase 2);
reference stability; undo/redo (Phase 2); recovery after failure (error in a
feature propagates and clears when fixed — partially covered now).

## 4. UI smoke test

`HybridCAD --smoke-test <dir> --mesh <file>` runs a script inside the real
application:

1. Demo history computes.
2. Face mode: pick the projected centroid of `Box1.Top` → selection reads
   `Fillet1 : Box1.Top`.
3. Set `Width = 90 mm` → recompute → the same face is re-selected by
   persistent reference; chamfer still valid.
4. Edge mode: pick `Edge(Box1.Front|Box1.Left)`; Vertex mode: pick
   `Vertex(Box1.Bottom|Box1.Front|Box1.Left)`.
5. Face mode on the NURBS sheet → `CPEdit2 : Surface1.Surface`.
6. Front view command; clicking the view cube changes the camera and does
   not select model geometry; grid toggle updates viewport and check mark.
7. Command palette: "view" lists the view commands, "fit al" ranks Fit All
   first, Enter runs it.
8. Welcome and Preferences dialogs open (screenshots).
9. Import `scan_sphere.obj` → watertight report.
10. Screenshots after each step (`docs/images/`).

The smoke test runs in its own settings scope (`HybridCAD-SmokeTest`), so it
never changes a user's settings.

## 4a. UI unit tests (Phase 1)

`hcad_ui_tests` runs with Qt's offscreen platform (no display, no OpenGL):
command registry (enabled/checked state, triggering, search ranking), command
palette filtering and recent commands, icon states, version macros, user data
outside the install directory, preferences round trip, the null update
service, and the main window: every menu and toolbar entry is a registered
command with a description, every command is reachable from a menu, and no
two commands share a shortcut.

## 4b. Installer and update round trip (Phase 1, Windows CI)

`packaging/windows/update-roundtrip.ps1`, run on every push:

1. Build and `vpk pack` 0.1.0-alpha; rebuild the same commit as 0.1.1-alpha
   and pack it into the same feed (vpk produces a delta package).
2. Silent install of 0.1.0-alpha with its own `Setup.exe`.
3. Write a user data file and a settings value.
4. Start the installed app with `--update-test <feed>`: it checks the feed,
   must be offered a **delta** update, downloads it, applies it and restarts
   itself.
5. The restarted app must report version 0.1.1-alpha and still see the
   settings value; the user data file must still exist.
6. Uninstall; the program is gone, the user data file is still there.

Screenshots composite Qt widget grabs with OCCT's own offscreen render of the
viewport because screen grabs of GL windows on virtual X servers are
unreliable.

## 5. CI

`.github/workflows/ci.yml`:

- **Linux**: Ubuntu 24.04 packages (OCCT 7.6, Qt 6.4), build, all tests
  including UI smoke under Xvfb, screenshots uploaded as artifacts.
- **Windows**: MSVC 2022 + conda-forge OCCT 7.9 / Qt 6.8 with Velopack,
  build, unit/geometry/UI tests, UI smoke test with Mesa llvmpipe (marked
  non-blocking: hosted runners have no GPU), installer packaging and the
  update round trip above. The installer and release feed are uploaded as
  an artifact.

Still planned: sanitizers (ASan/UBSan) on Linux, clang-tidy, tests on real
GPUs (owner's PC).
