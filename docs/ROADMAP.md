# Roadmap

Strict phase gates: every phase ends with a review by the owner, and the next
phase starts only after approval (brief §12, §16).

Each phase delivers: objectives met, working build, tests, documentation,
known limitations, screenshots, build/installation instructions, version.

| Phase | Version | Goal | Exit criteria (summary) |
|---|---|---|---|
| **0** | 0.0.1 | Architecture + proof of concept | Docs complete; PoC shows B-Rep box, viewport, V/E/F selection, editable dimensions, recompute, NURBS surface, mesh import; tests green on Linux + Windows CI. **This deliverable.** |
| 1 | 0.1.0-alpha | Professional UI shell, installer, updater | Install on Windows, navigate, select real geometry, switch modes, understandable UI, persisted layout, receive and apply an update without reinstall. |
| 2 | 0.2.x | Primitives, transforms, document system | Box/Cylinder/Sphere/Cone/Torus/Plane, G/R/S with axis + numeric input, New/Open/Save/Save As, undo/redo, recovery, measurement, mesh references positioned and measured. |
| 3 | 0.3.x | Parametric sketcher | Sketch tools, geometric + dimensional constraints, DOF status (under/fully/over), dependent geometry updates. |
| 4 | 0.4.x | Parametric solid modelling | Extrude, Revolve, Sweep, Loft, Booleans, Fillet, Chamfer, Shell, Draft, Face Offset; feature tree with edit/suppress/rollback; TNP test suite. |
| 5 | 0.5.x | Advanced parametrics | Parameter manager, expressions everywhere, reference geometry, patterns, mirror, robust dependency resolution (DAG, partial recompute). |
| 6 | 0.6.x | Reverse engineering / Mesh-to-CAD | Analysis/repair, primitive recognition, Modes A/B/C, reconstruction history, deviation reports, save/reopen with editability. |
| 7 | 0.7.x | Advanced NURBS and surfacing | Curves/surfaces tools, control point editing, degree/knots/weights, trim/extend/join/offset/match, curvature + zebra + G0/G1/G2 inspection. |
| 8 | 0.8.x | Assemblies and kinematics | Components, instances, joints, motion study, over-constraint detection. |
| 9 | 0.9.x | Import/export and manufacturing | STEP/IGES/STL/OBJ/3MF/DXF both ways, tessellation settings, watertight/export validation. |
| 10 | 1.0.0 | Stabilisation and distribution | Performance, crash logging, update reliability, Windows 10/11 test matrix, production channel. |

## Phase 1 plan (approved 2026-11-27, delivered — see PHASE1_REPORT.md)

1. **Foundation:** vcpkg manifest, MSVC release build, spdlog logging, crash
   handler, `CommandRegistry`, settings service.
2. **Shell:** main window layout per UX_DESIGN.md, dark theme, original icon set
   (normal/hover/active/disabled), menus generated from commands, command
   palette (F3), tooltips with shortcuts, status bar, project browser, history
   panel, context-sensitive properties.
3. **Viewport:** orbit/pan/zoom (Blender-style + alternatives), fit, standard
   and isometric views, grid, axes, view cube/orientation indicator,
   hover/selection highlight, Object/Vertex/Edge/Face modes (already proven).
4. **Onboarding:** first-launch welcome, example project, skippable hints.
5. **Distribution:** Velopack packaging in CI, stable/beta channels, HTTPS
   feed, delta updates, update UI (check, download progress, restart, rollback),
   separate app and user-data directories, update tests.
6. **Exit:** installable `0.1.0-alpha` + a second build `0.1.1-alpha` delivered
   through the updater to prove the update path.

Not in Phase 1: modelling tools beyond the demo, file save/open (Phase 2).
