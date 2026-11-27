# Product Brief

> Working title: **HybridCAD**. This is a placeholder; an original product name,
> logo and icon set are still to be created (Phase 1). No Blender or Plasticity
> branding, assets or code are used.

## Vision

A professional, native Windows CAD application that combines:

| Pillar | What we take as inspiration | What it means here |
|---|---|---|
| Blender-like interaction | Selection modes, keyboard-driven tools (G/R/S + axis + number), F3-style search, direct manipulation | Fast, learnable interaction model; every tool works with numeric input |
| Plasticity-inspired direct CAD and NURBS | Clean UI, control-point editing, continuity tools, mesh-to-NURBS workflow | Real NURBS curves/surfaces that stay editable and can become solids |
| Traditional parametric CAD | Sketches, constraints, feature history, named parameters, assemblies | Every operation is an editable feature with stored parameters and references |

**Philosophy:** Blender-like interaction + Plasticity-inspired direct CAD and
NURBS modelling + fully editable parametric CAD geometry.

## Target users and uses

- Blender users who need precise, manufacturable geometry.
- Engineers/makers designing mechanical parts, enclosures, functional objects.
- Product designers needing freeform surfaces with G1/G2 quality.
- Reverse engineering of scanned or downloaded meshes (STL/OBJ/3MF) into
  editable CAD.
- 3D printing and general manufacturing (STEP/IGES/STL/3MF/DXF exchange).

## Core product principles

1. **The B-Rep model is authoritative.** Tessellation is for display/export only.
   Imported meshes are labelled reference data and never presented as CAD solids.
2. **Everything is a feature.** Native operations, direct edits, control-point
   moves and reconstruction stages are recorded as editable history.
3. **Stable references.** Downstream features reference geometry by persistent
   semantic names, not by index (see ARCHITECTURE.md).
4. **Honest precision.** Millimetres by default, inches supported, explicit
   tolerances, clear failure messages. No claims of infinite precision.
5. **Clarity over complexity.** Context-sensitive panels, progressive disclosure,
   no decorative or unimplemented buttons.
6. **Install once, update in-app.** Per-user installer, signed delta updates,
   rollback, user data never touched.

## Explicit non-goals

Sculpting, character animation, rigging, UV/texture painting, game-engine
features, rendering studio, compositing, particles/VFX, general animation
timelines, a full slicer. Mechanical kinematics (joints, motion study) is in scope.

## Scope by phase (summary)

See [ROADMAP.md](ROADMAP.md). Phase 0 (this deliverable) validates the stack
with a working proof of concept; Phase 1 builds the professional UI shell,
installer and updater.

## Open product decisions (need owner input)

| # | Decision | Why it matters | Default if not decided |
|---|---|---|---|
| P1 | Will the product be closed-source/commercial or open-source? | Determines whether GPL components (SolveSpace solver, CGAL) are usable. LGPL (OCCT, Qt) works for both. | Assume closed-source; use only LGPL/permissive dependencies. |
| P2 | Final product name and visual identity | Needed for installer, update feed, file extension, icons. | Working title "HybridCAD", extension `.hcad`. |
| P3 | Code-signing certificate (OV / EV / Azure Trusted Signing) | Unsigned installers trigger SmartScreen warnings; updater signature checks rely on it. | Unsigned alpha builds with a clear warning until a certificate exists. |
| P4 | Update hosting (GitHub Releases vs own HTTPS bucket/CDN) | Affects privacy, bandwidth, beta channel management. | GitHub Releases for alpha, static HTTPS bucket later. |
