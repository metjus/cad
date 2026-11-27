# Dependency and Licence Review

Status: Phase 0 review, 2026-11-27. **Owner decision (2026-11-27): the
product is commercial and closed-source.** Only LGPL and permissive licences
are allowed in shipped binaries. GPL code is listed only to explain why it is
excluded.

This is an engineering review, not legal advice. Licence texts must be
re-checked against the exact versions pinned in vcpkg before each release.

## Shipped with the application

| Component | Used for | Licence | Conditions that affect us | Status |
|---|---|---|---|---|
| Open CASCADE Technology | Geometry kernel, visualization, STEP/IGES | LGPL-2.1 + OCCT exception | Dynamic linking; ship licence; provide source of any OCCT modifications; user must be able to replace the DLLs | **In use (PoC)** |
| Qt 6 (Core, Gui, Widgets, OpenGL) | UI | LGPL-3.0 | Dynamic linking; no static linking; user must be able to replace Qt DLLs (satisfied by per-user install in a writable folder); ship licence + source offer; no tivoization (n/a on Windows) | **In use (PoC)** |
| oneTBB | OCCT parallelism | Apache-2.0 | Ship licence/NOTICE | In use via OCCT |
| FreeType | OCCT text rendering | FreeType License (FTL) or GPLv2 — we use FTL | Attribution in docs/about | In use via OCCT |
| Velopack | Installer/updater | MIT | Ship licence | Planned (Phase 1) |
| Eigen | Linear algebra (solver, fitting) | MPL-2.0 | File-level copyleft only if Eigen files are modified | Planned (Phase 3/6) |
| PlaneGCS | Sketch solver | LGPL-2.1+ | Build as a separate DLL; publish modifications | Proposed (Phase 3) |
| nlohmann/json | Native format | MIT | Ship licence | Planned (Phase 2) |
| miniz or zlib/minizip | ZIP container | MIT / zlib | Ship licence | Planned (Phase 2) |
| lib3mf | 3MF import/export | BSD-2-Clause | Ship licence | Planned (Phase 9) |
| spdlog / fmt | Logging | MIT | Ship licence | Planned (Phase 1) |
| geogram | Remeshing, mesh repair | BSD-3-Clause | Ship licence | Candidate (Phase 6) |
| libigl | Mesh processing utilities | MPL-2.0 (core); some modules use GPL/CGAL — exclude those | Use only MPL-2.0 core headers | Candidate (Phase 6) |
| OpenMesh | Half-edge mesh structure | BSD-3-Clause | Ship licence | Candidate (Phase 6) |
| Instant Meshes | Field-aligned quad remeshing | BSD-3-Clause | Ship licence | Candidate (Phase 6) |
| QuadriFlow | Quad remeshing | Licence to be verified before use | – | Candidate, **verify** |
| Inter (font) | UI typeface | SIL OFL 1.1 | Ship licence; no renaming conflicts | Candidate (Phase 1) |

## Build / test only (not shipped)

| Component | Licence | Notes |
|---|---|---|
| GoogleTest | BSD-3-Clause | Tests |
| CMake, Ninja | BSD-3 / Apache-2.0 | Build |
| Mesa (llvmpipe, mesa-dist-win) | MIT | Software OpenGL for Windows CI smoke test only |
| conda-forge packages | per package | CI bootstrap only |
| vcpkg | MIT | Dependency manager (Phase 1) |

## Excluded or restricted

| Component | Licence | Reason |
|---|---|---|
| SolveSpace / libslvs | GPL-3.0 | Would force the whole application under GPL. Excluded (product is commercial). |
| CGAL (most packages) | GPL-3.0 / commercial | Same reason; commercial licence possible if a specific algorithm is essential (identify before Phase 6). |
| FreeImage | GPL-2/3 or FIPL | OCCT optional; not needed (Qt handles images). Build OCCT without it. |
| FFmpeg (OCCT optional video) | LGPL/GPL | Not needed; build OCCT without it. |
| Blender code or assets | GPL | Inspiration only; no code, icons, themes or keymaps copied verbatim. |
| Plasticity code, assets, branding | Proprietary | Public documentation used for workflow research only. No copying. |
| Parasolid/ACIS/D-Cubed | Commercial | Not used; would be a budget decision. |

## Compliance checklist for every release (Phase 1+)

1. `THIRD_PARTY_NOTICES.txt` generated from vcpkg `copyright` files, shipped in
   the install folder and shown in Help → About → Licences.
2. Qt and OCCT linked dynamically; DLLs in the per-user install folder.
3. Written offer / link to the exact source of LGPL components and any patches.
4. No GPL binaries in the package (CI check on the dependency list).
5. Icons and fonts are original or under OFL/MIT/ISC with attribution.
