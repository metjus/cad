# Technical Decisions (ADR log)

Each record: context → options → decision → consequences. Status values:
**Accepted** (validated by the Phase 0 PoC), **Proposed** (to confirm in the
named phase), **Needs owner input**.

Versions checked on 2026-11-27: OCCT 8.0.x is the newest major release
(8.0.0, then 8.0.1), 7.9.3 is the latest 7.x; Qt 6.11 is the current
minor, Qt 6.8 is the LTS line.

---

## ADR-001 Geometry kernel: Open CASCADE Technology (OCCT) — Accepted

**Options**

| Kernel | B-Rep / NURBS | Booleans / fillets | Licence / cost | Verdict |
|---|---|---|---|---|
| **OCCT** | Full B-Rep, analytic + B-spline curves/surfaces, trimming, sewing, healing | Good; fillets/booleans have known edge cases | LGPL-2.1 with linking exception, free | **Chosen** |
| Parasolid / ACIS / C3D | Industrial grade | Excellent | Commercial, per-seat royalties, NDA | Not viable without budget; can be revisited behind the kernel facade |
| CGAL | Meshes, arrangements, no NURBS B-Rep | – | GPL/commercial | Not a CAD kernel |
| Manifold | Mesh booleans only | – | Apache-2.0 | Not a CAD kernel |
| Truck (Rust) | Young B-Rep/NURBS | Immature | Apache/MIT | Too immature |

**Why OCCT:** the only free, mature kernel with real B-Rep, NURBS
(`Geom_BSplineSurface`, rational weights, knots), surface construction
(`GeomFill`, `GeomPlate`, `BRepOffsetAPI_ThruSections`), sewing and healing
(`BRepBuilderAPI_Sewing`, `ShapeFix`), STEP/IGES with names and colours
(XCAF), tessellation (`BRepMesh`) and a working visualization/selection stack
(`AIS`). Used in production by FreeCAD, KiCad, Salome, CadQuery, Mayo.

**Validated in PoC:** real B-Rep box with semantic faces, chamfer and fillet
with history propagation, kernel validity checks, rational B-spline surfaces,
viewport face/edge/vertex picking (OCCT 7.6 on Linux; Windows CI uses 7.9).

**Known weaknesses (risk register R1–R3):** fillet/chamfer failures on complex
blends, boolean robustness on tangent/coincident faces, no built-in
topological naming, single-threaded most algorithms, dated visualization.

**Version policy:** develop against OCCT 7.9.x for Phase 1 (well packaged in
vcpkg/conda), evaluate 8.0.x (API clean-up release) at the start of Phase 2 and
migrate when the ports are stable. The kernel facade (`hcad_kernel`) limits the
code that must change.

## ADR-002 Desktop UI: Qt 6 Widgets — Accepted

**Options:** Qt 6 Widgets, Qt Quick/QML, WinUI 3, wxWidgets, Dear ImGui,
Electron/web.

**Decision:** Qt 6 Widgets.

- Mature docking (`QDockWidget`, saved layouts — used in PoC), tree/table
  views, high-DPI support, native look under Fusion with a custom dark palette
  (PoC `Theme.cpp`).
- Proven combination with OCCT (FreeCAD, Mayo, OCCT samples).
- QML is better for animated touch UI but heavier for dense professional
  panels; WinUI 3 has awkward OpenGL interop and no cross-platform escape
  route; ImGui lacks accessibility and native text input; Electron is
  explicitly excluded by the brief.

**Licence:** LGPL-3.0 with dynamic linking (see LICENCES.md). Commercial Qt
not required.

**Version policy:** Qt 6.8 LTS for Phase 1 (open-source LTS patch releases
stop when the next LTS appears, so plan to move to the next LTS when it ships).

## ADR-003 Language and build — Accepted

C++20, CMake ≥ 3.21, MSVC 2022 (v143) for Windows releases, GCC/Clang on
Linux CI for faster feedback and sanitizers.

**Dependencies:** for release builds use **vcpkg manifest mode** with pinned
baseline (OCCT, Qt, GoogleTest, Eigen, spdlog, nlohmann-json, lib3mf) and
binary caching in CI. Phase 0 CI uses conda-forge on Windows (prebuilt OCCT +
Qt, minutes instead of hours) and Ubuntu packages on Linux; vcpkg is
introduced in Phase 1 together with the installer.

## ADR-004 Viewport integration — Accepted (revisit in Phase 1)

PoC: OCCT `V3d_View` bound to the native window of a Qt widget
(`WNT_Window` / `Xw_Window`). Works, picking verified. Lessons recorded in code:
display objects with selection mode 0 before switching modes (otherwise
sub-shape picking silently fails on OCCT 7.6); resync the camera aspect after
the native window is resized.

Phase 1 evaluates OCCT's `QOpenGLWidget` integration (OCCT renders into the
widget's FBO) for overlays, smoother docking and consistent high-DPI.

## ADR-005 Sketch constraint solver — Proposed (decide in Phase 3)

| Solver | Licence | Notes |
|---|---|---|
| **PlaneGCS** (FreeCAD Sketcher solver) | LGPL-2.1+ | Proven at scale, DOF/redundancy/conflict diagnostics, depends on Eigen (MPL-2.0) and Boost. Needs extraction from FreeCAD tree (standalone forks exist). |
| SolveSpace `libslvs` | **GPL-3.0** | Clean C API, robust, 3D constraints. Only usable if the product is GPL (owner decision P1). |
| D-Cubed 2D DCM (Siemens) | Commercial | Industry standard; licence cost. |
| Own Newton/LM solver | – | Not recommended (brief: evaluate established solutions first). |

**Recommendation:** PlaneGCS behind an `ISketchSolver` interface; build a
small benchmark (rectangle with constraints, slot, tangent arcs, 200-entity
sketch) in Phase 3 against libslvs. If the product becomes GPL, libslvs is a
valid alternative.

## ADR-006 NURBS strategy — Accepted

See NURBS_ARCHITECTURE.md. Editable definition (degree, poles, weights, knots)
is stored in the document and rebuilt into OCCT `Geom_BSplineSurface`; control
point moves are history features (PoC: `ControlPointEditFeature`, tested).

## ADR-007 Installer and updater: Velopack — Proposed (implement in Phase 1)

| Option | Delta updates | Per-user, no admin | Rollback | Notes |
|---|---|---|---|---|
| **Velopack** (MIT) | Yes (binary delta packages) | Yes (`%LOCALAPPDATA%`) | Keeps previous version; full package fallback | C++ SDK, CLI `vpk pack`, channels (stable/beta), static HTTPS feed, Authenticode signing integration |
| MSIX + App Installer | Block-level | Yes | OS-managed | Requires signing for every build, packaged-app restrictions, harder for portable/beta |
| WiX MSI + WinSparkle | No (full MSI) | Usually admin | Manual | Mature, but users re-run installers |
| Inno Setup + custom updater | DIY | Configurable | DIY | Most custom code to own |

**Decision:** Velopack. It matches the brief exactly: one-time install, in-app
updates with deltas, rollback, channels, no manual reinstall, user data
separate. Production feed over HTTPS (GitHub Releases for alpha, own bucket
later). Integrity: SHA-256 per package (built in) + Authenticode signature
verification once a certificate exists (owner decision P3). A full installer
(`Setup.exe`) is always published for first install and recovery.

## ADR-008 Native file format — Proposed (implement in Phase 2)

ZIP container with JSON document + optional BRep caches + mesh payloads (see
ARCHITECTURE §8). JSON keeps the format diffable and migratable; caches make
large models open fast. OCAF rejected as the primary model (ARCHITECTURE §8).

## ADR-009 Topological naming — Accepted (core), Proposed (extensions)

Generator-role face names + history propagation + face-set edge/vertex
references; ambiguity fails loudly. Implemented and regression-tested in
Phase 0. Extensions (sketch-element names, geometric fallback, repair UI) in
Phases 3–5.

## ADR-010 Mesh-to-CAD toolchain — Proposed (Phase 6)

See MESH_TO_CAD.md. Own pipeline on top of permissive libraries (geogram,
libigl, OpenMesh, Instant Meshes) and OCCT fitting
(`GeomAPI_PointsToBSplineSurface`, `GeomPlate`). CGAL avoided in the core
because of GPL.

## ADR-011 Testing — Accepted

GoogleTest for unit/geometry tests, CTest orchestration, scripted UI smoke test
inside the real app (`--smoke-test`), screenshots from OCCT offscreen render,
CI on Linux (Xvfb) and Windows (MSVC, Mesa llvmpipe for the GL smoke test).
See TEST_STRATEGY.md.

---

## Risk register

| ID | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| R1 | OCCT fillet/chamfer/boolean failures on real models | High | High | Validate every result (`BRepCheck`), clear error messages, `ShapeFix` healing, regression corpus of failing cases, keep kernel behind facade (commercial kernel possible later). |
| R2 | Topological naming breaks in complex histories (sketch edits that change curve count, booleans splitting faces) | High | High | Semantic naming from day one (done), geometric fallback matcher, repair UI, large reference-stability test suite. |
| R3 | Performance of recompute/tessellation on large models | Medium | High | Dirty-flag DAG, background recompute, cached results, parallel `BRepMesh`, LOD tessellation. |
| R4 | Mesh-to-NURBS quality is fundamentally hard; user expectations ("one click to perfect CAD") | High | High | Communicate approximations with deviation reports, semi-automatic workflow with review steps, staged features, scope Mode B to suitable inputs. |
| R5 | Sketch solver licensing/quality (GPL vs LGPL) | Medium | High | Decide product licence early (P1); PlaneGCS is the LGPL path. |
| R6 | Windows OpenGL driver variability (Intel iGPUs, remote desktop) | Medium | Medium | OCCT core profile ≥ 3.3, fallback flags, Mesa software renderer option, crash telemetry opt-in. |
| R7 | LGPL compliance with installer/updater (relinking/replacing Qt/OCCT DLLs) | Low | Medium | Dynamic linking only, per-user install in a writable folder, ship licence texts and source offer. |
| R8 | Code-signing not available → SmartScreen warnings, weaker update trust | Medium | Medium | Obtain certificate before public beta (P3). |
| R9 | OCCT 7.9 → 8.0 API migration cost | Medium | Low | Facade layer; migrate at start of Phase 2. |
| R10 | Scope: the full roadmap is multi-year | High | High | Strict phase gates (as required by the brief), each phase shippable. |
