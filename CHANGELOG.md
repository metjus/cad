# Changelog

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
