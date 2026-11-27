# Mesh-to-CAD (Reverse Engineering)

Phase 6 feature; Phase 0 provides import, analysis and the architecture.

## 1. Principles

1. OBJ/STL/3MF contain polygons only. Original sketches, constraints, history
   and design intent **cannot be recovered**; we reconstruct a *new*
   editable model and say so in the UI.
2. The source mesh is a non-destructive `MeshReference` that is never modified
   or deleted by reconstruction **[PoC]**.
3. Every reconstruction stage is a feature with stored parameters, so the user
   can reopen and change settings, and results recompute.
4. Results are approximations within a stated tolerance, with deviation
   reports. Failures are localised and explained; partial results are kept.

## 2. Import (Phase 0 status)

| Capability | Status |
|---|---|
| STL ASCII + binary (binary detected by size, robust to "solid" headers) | **[PoC]** tested |
| OBJ (v/f, polygons fan-triangulated, `v/vt/vn` and negative indices) | **[PoC]** tested |
| 3MF | Phase 6/9 via lib3mf (BSD-2) |
| Unit confirmation (mm, cm, m, in) | **[PoC]** dialog + scale |
| Vertex welding | **[PoC]** grid hash, tolerance 1e-6 mm |
| Analysis report | **[PoC]** vertices, triangles, components, open boundary edges + loops (holes), non-manifold edges, inconsistent orientation, degenerate and duplicate triangles, size, area, enclosed volume, watertightness, inverted normals |
| Display as reference (distinct colour, not CAD-selectable) | **[PoC]** |
| Self-intersection detection | Phase 6 (BVH triangle-triangle tests) |
| Noise estimation | Phase 6 (local plane-fit residuals) |
| Position/orientation, auto-align to principal axes | Phase 2/6 |

## 3. Preprocessing (Phase 6)

Repair (fill small holes, fix orientation, remove degenerates/duplicates,
split components), decimation (quadric error), refinement, smoothing
(feature-preserving, bilateral), normal recomputation. Every step previews
the change and reports what will be removed; nothing destroys detail silently.
Implemented as features (`MeshCleanup{params}`) producing a *derived* mesh; the
original stays untouched.

Libraries (permissive): geogram (BSD-3) for remeshing/decimation/repair,
OpenMesh (BSD-3) or own half-edge structure, Eigen (MPL-2.0). CGAL excluded
(GPL) unless a commercial licence is bought.

## 4. Mode A — Parametric reconstruction (mechanical parts)

Pipeline:

1. **Normals and curvature** per vertex (PCA / quadric fitting).
2. **Segmentation** into regions: region growing on normal/curvature
   similarity, seeded by **RANSAC primitive detection** (planes, cylinders,
   cones, spheres, tori — Efficient-RANSAC style, own implementation).
3. **Fitting**: least-squares refinement of each primitive (Levenberg–Marquardt
   on Eigen), with constraints snapping (axis-aligned normals, common axes,
   round radii if within tolerance — user-configurable).
4. **Feature recognition** on the region adjacency graph: holes (cylinder
   regions bounded by circles with inward normals), bosses, pockets, extrusion
   profiles (parallel planar caps + ruled sides), revolved parts (shared axis),
   symmetry planes, linear/circular repetitions.
5. **Model construction**: generate *native features* — Sketch + Extrude,
   Hole, Revolve, Pattern — whose dimensions come from the fit. Regions not
   explained by features become face-replacement or freeform patches (Mode C).
6. **Validation**: build B-Rep, check validity, compute deviation to mesh.

Review UI: list of recognised features with confidence and deviation;
accept/reject/edit before committing. Output is a normal parametric history.

## 5. Mode B — NURBS reconstruction (organic shapes)

Pipeline:

1. **Quad-dominant remeshing** with field alignment (Instant Meshes, BSD-3;
   QuadriFlow licence to verify) at a user-chosen density.
2. **Patch layout**: extract a coarse quad layout from the singularities and
   separatrices of the cross field (or user-guided patch boundaries drawn on
   the mesh — important for quality).
3. **Per-patch fitting**: parameterise each patch (harmonic/mean-value map to
   a square), fit a B-spline surface with `GeomAPI_PointsToBSplineSurface` or
   own least-squares with smoothing (thin-plate energy), starting single-span
   and adding knots only where deviation exceeds tolerance.
4. **Boundary continuity**: shared boundary curves, then G1 (and G2 where
   requested) enforced by constraining the first pole rows across patches.
5. **Stitching** into a shell, solidify if closed.

Expectations: high-density scans of smooth shapes produce good results;
thin features, sharp creases mixed with freeform, and noisy scans need user
guidance. We prioritise few clean patches over reproducing every triangle.

## 6. Mode C — Hybrid

Run Mode A segmentation; regions recognised as analytic become analytic
faces; remaining regions are fitted with NURBS patches whose boundaries are
the intersection curves with neighbouring analytic faces (trimmed surfaces).
Sew, heal, solidify; if not closed, keep the sheets and report open edges.

## 7. Reconstruction history (example, Mode C)

```
MeshRef1           (source file, units, placement)
MeshAnalysis1      (report; no geometry change)
MeshCleanup1       (hole fill ≤ 2 mm², decimate to 200k tris, smoothing 0)
Segmentation1      (sensitivity, min region area, RANSAC tolerance)
Recognition1       (accepted primitive list, snapping rules)
AnalyticFaces1     (planes/cylinders built from Recognition1)
SurfaceFit1        (patch layout, degree, max deviation 0.05 mm, G1)
Stitch1            (sewing tolerance)
Solidify1          (validity check)
```

Each stage stores parameters and the ids of the inputs it uses. Editing an
upstream parameter recomputes downstream stages. User edits after
reconstruction (move control points, change a hole diameter) are ordinary
features appended after the reconstruction stages.

## 8. Accuracy reporting

Per result: max and RMS deviation (point-to-surface distances from mesh
vertices, and surface samples to mesh), patch/face count, recognition report,
open boundary count, solid validity, quality score, warnings with locations.
Visual deviation map (colour on the mesh) and the original mesh as an overlay.

## 9. Presets

| Preset | Decimation | Tolerance | Patch density | Continuity |
|---|---|---|---|---|
| Fast | aggressive | 0.2 mm | low | G1 |
| Balanced | moderate | 0.05 mm | medium | G1 |
| High Quality | light | 0.01 mm | adaptive | G2 target |

All detailed parameters remain available under "Advanced".

## 10. Test corpus (Phase 6)

Mechanical box; part with holes and cylinders; housing; organic freeform;
high-density scan; mesh with defects; non-manifold mesh. Each test checks
deviation, validity, patch quality, continuity, solid validity, boolean
compatibility, STEP export, save/reopen editability. Phase 0 already includes
small fixtures for watertight, open (hole) and non-manifold meshes.
