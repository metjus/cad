# HybridCAD

A native Windows parametric CAD application combining Blender-like
interaction, Plasticity-inspired direct and NURBS modelling, and fully editable
parametric history, with mesh-to-CAD reverse engineering.

**Status: 0.1.x-alpha — Phase 1 (UI foundation, installer, in-app updates).**
Not a usable CAD application yet: it opens an example parametric project and
imports mesh references; there are no modelling tools or project saving yet.
See [docs/PHASE1_REPORT.md](docs/PHASE1_REPORT.md).

![HybridCAD 0.1.0-alpha](docs/images/p1_08_command_palette.png)

## Documentation

| Document | Content |
|---|---|
| [PHASE1_REPORT](docs/PHASE1_REPORT.md) | Results of Phase 1: UI shell, installer, updates |
| [UPDATES](docs/UPDATES.md) | How installation, updates and user data work |
| [PHASE0_REPORT](docs/PHASE0_REPORT.md) | Results of Phase 0, decisions, test results, risks |
| [PRODUCT_BRIEF](docs/PRODUCT_BRIEF.md) | Vision, principles, non-goals, open owner decisions |
| [ARCHITECTURE](docs/ARCHITECTURE.md) | Layers, document model, recompute, topological naming |
| [TECH_DECISIONS](docs/TECH_DECISIONS.md) | ADR log and risk register |
| [ROADMAP](docs/ROADMAP.md) | Phases 0–10 and Phase 1 plan |
| [LICENCES](docs/LICENCES.md) | Dependency and licence review |
| [TEST_STRATEGY](docs/TEST_STRATEGY.md) | Test levels, CI |
| [NURBS_ARCHITECTURE](docs/NURBS_ARCHITECTURE.md) | NURBS representation and editing |
| [MESH_TO_CAD](docs/MESH_TO_CAD.md) | Reverse-engineering pipeline |
| [UX_DESIGN](docs/UX_DESIGN.md) | Layout, interaction, visual design |

## Building

Requirements: CMake ≥ 3.21, a C++20 compiler, Open CASCADE Technology
(7.6–7.9 tested), Qt 6 (Widgets), GoogleTest.

### Linux (Ubuntu 24.04)

```bash
sudo apt install cmake ninja-build g++ libocct-*-dev qt6-base-dev libgtest-dev \
  libgl-dev libglu1-mesa-dev libfontconfig1-dev libtbb-dev xvfb
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure      # includes UI smoke test via xvfb-run
./build/src/HybridCAD
```

### Windows (MSVC 2022)

The CI recipe (`.github/workflows/ci.yml`) uses conda-forge binaries:

```powershell
micromamba create -n hcad -c conda-forge occt=7.9 qt6-main=6.8 gtest cmake ninja
micromamba activate hcad
# from a "x64 Native Tools Command Prompt for VS 2022"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="$env:CONDA_PREFIX\Library" -DHCAD_RUNTIME_DIRS="$env:CONDA_PREFIX\Library\bin" `
  -DCMAKE_INSTALL_PREFIX="$pwd\dist\HybridCAD"
cmake --build build
ctest --test-dir build -E ui_smoke
cmake --install build      # portable folder with required DLLs
```

Add `-DHCAD_WITH_VELOPACK=ON -DVELOPACK_DIR=<extracted velopack_libc zip>` to
enable installer hooks and in-app updates. The installer is produced with
`vpk pack` (see [docs/UPDATES.md](docs/UPDATES.md) and the Windows CI job).

### Installing a CI build

Each CI run attaches `HybridCAD-0.1.1-alpha-windows-x64-installer`. Run
`HybridCAD.App-stable-Setup.exe`: it installs per user (no administrator
rights) into `%LOCALAPPDATA%\HybridCAD.App` and adds a Start menu entry. The
installer is not code-signed yet, so Windows SmartScreen will warn
("More info" → "Run anyway").

## Using it

- F3 or Ctrl+K: search and run any command. Ctrl+F1 lists all shortcuts.
- 1 / 2 / 3 / 4: Object / Vertex / Edge / Face selection. Click to select.
- Blender-style navigation (default): middle mouse orbit, Shift+middle pan,
  wheel zoom. CAD-style (Preferences): middle mouse pan, Shift+middle orbit.
- View cube (top right): click a face, edge or corner. 0 iso, 7/8/9 top, front,
  right (Ctrl for the opposite side), 5 perspective, Home fit.
- Edit parameters (e.g. `Width` → `90 mm` or `Height` → `Width * 0.5`) in the
  Parameters table, or a feature's fields after selecting it in History.
- Right-click a History item: suppress, roll back, roll to end.
- File → Import Mesh Reference (STL, OBJ).
